#include "sketch/SketchController.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "sketch/SketchOverlay.h"

#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/nodes/SoCamera.h>

#include <QLineEdit>
#include <QApplication>
#include <QTimer>
#include <QWidget>

namespace freeshape::sketch {

SketchController::SketchController(
    App::Document* document,
    Gui::Document* guiDocument,
    Gui::View3DInventor* view,
    QWidget* viewportHost
)
    : document_(document)
    , guiDocument_(guiDocument)
    , view_(view)
    , viewportHost_(viewportHost)
{
    if (document_ == nullptr || guiDocument_ == nullptr || view_ == nullptr
        || viewportHost_ == nullptr) {
        throw std::invalid_argument("SketchController requires live document/view objects");
    }

    overlay_ = new SketchOverlay(viewportHost_);
    syncOverlayGeometry();

    numericInput_ = new QLineEdit(viewportHost_);
    numericInput_->setPlaceholderText(QStringLiteral("Diameter"));
    numericInput_->setFixedWidth(108);
    numericInput_->hide();
    numericInput_->setStyleSheet(QStringLiteral(
        "QLineEdit { background:#ffffff; border:1px solid #4c9bd4; "
        "border-radius:3px; padding:4px 6px; }"
    ));

    QObject::connect(numericInput_, &QLineEdit::returnPressed, viewportHost_, [this] {
        bool ok = false;
        const double diameter = numericInput_->text().toDouble(&ok);
        if (ok && diameter > 0.0 && pendingDiameterGeometry_ >= 0) {
            applyDiameterConstraint(diameter);
            emitMessage(
                QStringLiteral("Circle diameter constrained to %1 mm").arg(diameter)
            );
        }
        numericInput_->hide();
        pendingDiameterGeometry_ = -1;
        if (auto* gl = view_->getViewer()->getGLWidget(); gl != nullptr) {
            gl->setFocus();
        }
    });
}

SketchController::~SketchController()
{
    if (transactionOpen_) {
        finishSketch(false);
    }
}

void SketchController::beginSketch()
{
    if (transactionOpen_) {
        finishSketch(false);
    }

    state_ = State::AwaitPlane;
    tool_ = Tool::None;
    geometryCount_ = 0;
    circleCenter_.reset();
    firstPoint_.reset();
    overlay_->clearPreview();
    numericInput_->hide();

    emitMessage(QStringLiteral("Sketch · select Top, Front, Right, or a planar face"));
    emitChanged();
}

bool SketchController::selectPlane(const QString& planeObjectName)
{
    if (state_ != State::AwaitPlane) {
        return false;
    }

    if (!createSketchOnPlane(planeObjectName)) {
        return false;
    }

    state_ = State::Editing;
    tool_ = Tool::None;

    // Current interactive geometry pass supports the canonical Top/XY workflow.
    // Other base planes are created correctly, but their screen-to-sketch
    // transform lands in the next pass.
    if (planeObjectName == QStringLiteral("XY_Plane")) {
        guiDocument_->sendMsgToViews("ViewTop");
    }

    for (const char* plane : {"XY_Plane", "XZ_Plane", "YZ_Plane"}) {
        guiDocument_->setHide(plane);
    }
    guiDocument_->setHide("Origin");
    guiDocument_->setShow("Sketch");

    emitMessage(
        planeObjectName == QStringLiteral("XY_Plane")
            ? QStringLiteral("Sketch 1 · Top plane · press C for circle")
            : QStringLiteral(
                  "Sketch created · Top plane is the fully interactive plane in this pass"
              )
    );
    emitChanged();
    return true;
}

bool SketchController::isAwaitingPlane() const
{
    return state_ == State::AwaitPlane;
}

bool SketchController::isEditing() const
{
    return state_ == State::Editing;
}

bool SketchController::hasClosedProfile() const
{
    return geometryCount_ > 0;
}

void SketchController::activateCircle()
{
    if (state_ != State::Editing) {
        emitMessage(QStringLiteral("Circle requires an active sketch"));
        return;
    }

    tool_ = Tool::Circle;
    circleCenter_.reset();
    overlay_->clearPreview();
    numericInput_->hide();
    pendingDiameterGeometry_ = -1;

    emitMessage(QStringLiteral("Circle · click center, then click radius"));
}

void SketchController::activateLine()
{
    if (state_ != State::Editing) {
        emitMessage(QStringLiteral("Line requires an active sketch"));
        return;
    }

    tool_ = Tool::Line;
    firstPoint_.reset();
    overlay_->clearPreview();
    numericInput_->hide();
    emitMessage(QStringLiteral("Line · click start, then click end"));
}

void SketchController::activateCornerRectangle()
{
    if (state_ != State::Editing) {
        emitMessage(QStringLiteral("Rectangle requires an active sketch"));
        return;
    }

    tool_ = Tool::CornerRectangle;
    firstPoint_.reset();
    overlay_->clearPreview();
    numericInput_->hide();
    emitMessage(QStringLiteral("Corner rectangle · click first corner, then opposite corner"));
}

void SketchController::cancelActiveTool()
{
    circleCenter_.reset();
    firstPoint_.reset();
    overlay_->clearPreview();
    numericInput_->hide();
    pendingDiameterGeometry_ = -1;
    tool_ = Tool::None;
    emitMessage(QStringLiteral("Sketch tool cancelled"));
}

bool SketchController::handleMousePress(
    const QPoint& viewportPosition,
    Qt::MouseButton button
)
{
    if (state_ != State::Editing || tool_ == Tool::None
        || button != Qt::LeftButton) {
        return false;
    }

    syncOverlayGeometry();

    const auto point = screenToXY(viewportPosition);
    if (!point.has_value()) {
        return true;
    }

    bool snapX = false;
    bool snapY = false;
    const QPointF inferred = applyInference(*point, &snapX, &snapY);

    if (tool_ == Tool::Circle) {
        if (!circleCenter_.has_value()) {
            circleCenter_ = inferred;
            circleCenterScreen_ = viewportPosition;
            centerSnapX_ = snapX;
            centerSnapY_ = snapY;
            overlay_->setCirclePreview(
                circleCenterScreen_,
                circleCenterScreen_,
                centerSnapX_,
                centerSnapY_
            );
            emitMessage(
                snapX && snapY
                    ? QStringLiteral("Circle center · coincident with Origin")
                    : QStringLiteral("Circle center set · click radius")
            );
            return true;
        }

        const QPointF center = *circleCenter_;
        const double radius = std::hypot(inferred.x() - center.x(), inferred.y() - center.y());
        if (radius < 1.0e-6) {
            emitMessage(QStringLiteral("Circle radius is too small"));
            return true;
        }

        const int geoId = createCircle(center, radius);
        if (geoId >= 0) {
            ++geometryCount_;
            promptDiameter(geoId, radius * 2.0, viewportPosition);
            emitChanged();
        }
        circleCenter_.reset();
        overlay_->clearPreview();
        emitMessage(QStringLiteral("Circle created · type diameter + Enter, or draw another"));
        return true;
    }

    if (!firstPoint_.has_value()) {
        firstPoint_ = inferred;
        firstPointScreen_ = viewportPosition;
        centerSnapX_ = snapX;
        centerSnapY_ = snapY;
        emitMessage(
            tool_ == Tool::Line
                ? QStringLiteral("Line start set · click end")
                : QStringLiteral("Rectangle corner set · click opposite corner")
        );
        return true;
    }

    if (tool_ == Tool::Line) {
        if (createLine(*firstPoint_, inferred) >= 0) {
            ++geometryCount_;
            emitChanged();
        }
        // Continuous line mode: previous endpoint becomes next start.
        firstPoint_ = inferred;
        firstPointScreen_ = viewportPosition;
        overlay_->clearPreview();
        emitMessage(QStringLiteral("Line created · continue line, or Esc to stop"));
        return true;
    }

    if (tool_ == Tool::CornerRectangle) {
        const int created = createRectangle(*firstPoint_, inferred);
        if (created >= 0) {
            geometryCount_ += 4;
            emitChanged();
        }
        firstPoint_.reset();
        overlay_->clearPreview();
        emitMessage(QStringLiteral("Rectangle created · draw another, or Shift+E to extrude"));
        return true;
    }

    return true;
}

void SketchController::handleMouseMove(const QPoint& viewportPosition)
{
    if (state_ != State::Editing || tool_ == Tool::None) {
        return;
    }

    syncOverlayGeometry();

    if (tool_ == Tool::Circle && circleCenter_.has_value()) {
        overlay_->setCirclePreview(
            circleCenterScreen_,
            viewportPosition,
            centerSnapX_,
            centerSnapY_
        );
        return;
    }

    if (firstPoint_.has_value()) {
        if (tool_ == Tool::Line) {
            overlay_->setLinePreview(
                firstPointScreen_,
                viewportPosition,
                centerSnapX_,
                centerSnapY_
            );
        }
        else if (tool_ == Tool::CornerRectangle) {
            overlay_->setRectanglePreview(
                firstPointScreen_,
                viewportPosition,
                centerSnapX_,
                centerSnapY_
            );
        }
    }
}

bool SketchController::finishSketch(bool accept)
{
    if (state_ == State::Idle) {
        return false;
    }

    const bool hadProfile = geometryCount_ > 0;

    overlay_->clearPreview();
    numericInput_->hide();
    circleCenter_.reset();
    firstPoint_.reset();
    tool_ = Tool::None;
    pendingDiameterGeometry_ = -1;

    if (transactionOpen_) {
        if (accept) {
            document_->commitTransaction();
            document_->recompute();
            Base::Console().message(
                "FREESHAPE_SKETCH PASS geometry_count={}\n",
                geometryCount_
            );
        }
        else {
            document_->abortTransaction();
            document_->recompute();
            geometryCount_ = 0;
            Base::Console().message("FREESHAPE_SKETCH CANCEL\n");
        }
        transactionOpen_ = false;
    }

    state_ = State::Idle;
    emitChanged();
    return accept && hadProfile;
}

void SketchController::setChangedHandler(std::function<void()> handler)
{
    changed_ = std::move(handler);
}

void SketchController::setMessageHandler(
    std::function<void(const QString&)> handler
)
{
    message_ = std::move(handler);
}

std::optional<QPointF> SketchController::screenToXY(
    const QPoint& viewportPosition
) const
{
    auto* viewer = view_->getViewer();
    auto* gl = viewer->getGLWidget();
    auto* camera = viewer->getSoRenderManager()->getCamera();

    if (gl == nullptr || camera == nullptr || gl->width() <= 0 || gl->height() <= 0) {
        return std::nullopt;
    }

    const float aspect =
        static_cast<float>(gl->width()) / static_cast<float>(gl->height());

    SbViewVolume volume = camera->getViewVolume(aspect);
    SbLine ray;

    const float nx =
        static_cast<float>(viewportPosition.x()) / static_cast<float>(gl->width());
    const float ny =
        1.0F
        - static_cast<float>(viewportPosition.y()) / static_cast<float>(gl->height());

    volume.projectPointToLine(SbVec2f(nx, ny), ray);

    SbPlane sketchPlane(SbVec3f(0.0F, 0.0F, 1.0F), 0.0F);
    SbVec3f intersection;

    if (!sketchPlane.intersect(ray, intersection)) {
        return std::nullopt;
    }

    return QPointF(intersection[0], intersection[1]);
}

QPointF SketchController::applyInference(
    const QPointF& point,
    bool* snapX,
    bool* snapY
) const
{
    QPointF result = point;

    // Shift explicitly suppresses automatic inferencing, matching the
    // Onshape sketch interaction contract.
    if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
        if (snapX != nullptr) *snapX = false;
        if (snapY != nullptr) *snapY = false;
        return result;
    }

    // This pass centralizes the initial origin/axis inference policy here so
    // all geometry tools share it.  The next inference pass expands this into
    // a screen-space candidate graph for existing geometry/midpoints/tangency.
    constexpr double inferenceTolerance = 1.0;

    const bool x = std::abs(result.x()) <= inferenceTolerance;
    const bool y = std::abs(result.y()) <= inferenceTolerance;

    if (x) {
        result.setX(0.0);
    }
    if (y) {
        result.setY(0.0);
    }

    if (snapX != nullptr) {
        *snapX = x;
    }
    if (snapY != nullptr) {
        *snapY = y;
    }
    return result;
}

bool SketchController::createSketchOnPlane(const QString& planeObjectName)
{
    if (document_->getObject("Sketch") != nullptr) {
        emitMessage(QStringLiteral("Sketch 1 already exists"));
        state_ = State::Editing;
        geometryCount_ = 1;
        return true;
    }

    document_->openTransaction("FreeShape Sketch");
    transactionOpen_ = true;

    const QByteArray plane = planeObjectName.toUtf8();

    std::ostringstream script;
    script
        << "import FreeCAD as App\n"
        << "import Sketcher\n"
        << "doc = App.getDocument('" << document_->getName() << "')\n"
        << "body = doc.getObject('Body')\n"
        << "if body is None:\n"
        << "    body = doc.addObject('PartDesign::Body', 'Body')\n"
        << "sketch = body.newObject('Sketcher::SketchObject', 'Sketch')\n"
        << "sketch.Label = 'Sketch 1'\n"
        << "sketch.AttachmentSupport = (doc.getObject('" << plane.constData()
        << "'), [''])\n"
        << "sketch.MapMode = 'FlatFace'\n"
        << "doc.recompute()\n";

    Base::Interpreter().runString(script.str().c_str());
    document_->recompute();

    if (document_->getObject("Sketch") == nullptr) {
        document_->abortTransaction();
        transactionOpen_ = false;
        emitMessage(QStringLiteral("Failed to create Sketch 1"));
        return false;
    }

    geometryCount_ = 0;

    Base::Console().message(
        "FREESHAPE_SKETCH_BEGIN PASS plane={}\n",
        planeObjectName.toStdString()
    );
    return true;
}

int SketchController::createCircle(const QPointF& center, double radius)
{
    std::ostringstream script;
    script
        << "import FreeCAD as App\n"
        << "import Part\n"
        << "doc = App.getDocument('" << document_->getName() << "')\n"
        << "sketch = doc.getObject('Sketch')\n"
        << "geo = sketch.addGeometry("
        << "Part.Circle(App.Vector(" << center.x() << "," << center.y() << ",0),"
        << "App.Vector(0,0,1)," << radius << "), False)\n"
        << "doc.recompute()\n"
        << "App.__freeshape_last_geo = geo\n";

    Base::Interpreter().runString(script.str().c_str());
    document_->recompute();

    // The first circle is geometry 0, then monotonically increasing in this
    // first-pass controller because we do not delete entities yet.
    const int geoId = geometryCount_;

    Base::Console().message(
        "FREESHAPE_CIRCLE PASS geo={} cx={:.6f} cy={:.6f} radius={:.6f}\n",
        geoId,
        center.x(),
        center.y(),
        radius
    );
    return geoId;
}

int SketchController::createLine(const QPointF& start, const QPointF& end)
{
    if (std::hypot(end.x() - start.x(), end.y() - start.y()) < 1.0e-6) {
        return -1;
    }

    std::ostringstream script;
    script
        << "import FreeCAD as App\n"
        << "import Part\n"
        << "doc = App.getDocument('" << document_->getName() << "')\n"
        << "sketch = doc.getObject('Sketch')\n"
        << "geo = sketch.addGeometry(Part.LineSegment("
        << "App.Vector(" << start.x() << "," << start.y() << ",0),"
        << "App.Vector(" << end.x() << "," << end.y() << ",0)), False)\n"
        << "doc.recompute()\n";
    Base::Interpreter().runString(script.str().c_str());
    document_->recompute();

    const int geoId = geometryCount_;
    Base::Console().message(
        "FREESHAPE_LINE PASS geo={} x1={:.6f} y1={:.6f} x2={:.6f} y2={:.6f}\n",
        geoId,
        start.x(),
        start.y(),
        end.x(),
        end.y()
    );
    return geoId;
}

int SketchController::createRectangle(
    const QPointF& first,
    const QPointF& opposite
)
{
    if (std::abs(opposite.x() - first.x()) < 1.0e-6
        || std::abs(opposite.y() - first.y()) < 1.0e-6) {
        return -1;
    }

    std::ostringstream script;
    script
        << "import FreeCAD as App\n"
        << "import Part\n"
        << "import Sketcher\n"
        << "doc = App.getDocument('" << document_->getName() << "')\n"
        << "sketch = doc.getObject('Sketch')\n"
        << "x1,y1,x2,y2 = " << first.x() << "," << first.y() << ","
        << opposite.x() << "," << opposite.y() << "\n"
        << "ids = sketch.addGeometry([\n"
        << " Part.LineSegment(App.Vector(x1,y1,0),App.Vector(x2,y1,0)),\n"
        << " Part.LineSegment(App.Vector(x2,y1,0),App.Vector(x2,y2,0)),\n"
        << " Part.LineSegment(App.Vector(x2,y2,0),App.Vector(x1,y2,0)),\n"
        << " Part.LineSegment(App.Vector(x1,y2,0),App.Vector(x1,y1,0))], False)\n"
        << "sketch.addConstraint(Sketcher.Constraint('Horizontal', ids[0]))\n"
        << "sketch.addConstraint(Sketcher.Constraint('Vertical', ids[1]))\n"
        << "sketch.addConstraint(Sketcher.Constraint('Horizontal', ids[2]))\n"
        << "sketch.addConstraint(Sketcher.Constraint('Vertical', ids[3]))\n"
        << "doc.recompute()\n";

    Base::Interpreter().runString(script.str().c_str());
    document_->recompute();

    Base::Console().message(
        "FREESHAPE_RECTANGLE PASS first_geo={}\n",
        geometryCount_
    );
    return geometryCount_;
}

void SketchController::applyDiameterConstraint(double diameter)
{
    if (pendingDiameterGeometry_ < 0) {
        return;
    }

    std::ostringstream script;
    script
        << "import FreeCAD as App\n"
        << "import Sketcher\n"
        << "doc = App.getDocument('" << document_->getName() << "')\n"
        << "sketch = doc.getObject('Sketch')\n"
        << "sketch.addConstraint(Sketcher.Constraint("
        << "'Diameter'," << pendingDiameterGeometry_ << "," << diameter << "))\n"
        << "doc.recompute()\n";

    Base::Interpreter().runString(script.str().c_str());
    document_->recompute();

    Base::Console().message(
        "FREESHAPE_DIAMETER PASS geo={} diameter={:.6f}\n",
        pendingDiameterGeometry_,
        diameter
    );
}

void SketchController::promptDiameter(
    int geometryIndex,
    double diameter,
    const QPoint& screenPosition
)
{
    pendingDiameterGeometry_ = geometryIndex;

    numericInput_->setText(QString::number(diameter, 'f', 3));
    numericInput_->selectAll();
    numericInput_->adjustSize();

    const int x = std::min(
        std::max(8, screenPosition.x() + 12),
        std::max(8, viewportHost_->width() - numericInput_->width() - 8)
    );
    const int y = std::min(
        std::max(8, screenPosition.y() + 12),
        std::max(8, viewportHost_->height() - numericInput_->height() - 8)
    );

    numericInput_->move(x, y);
    numericInput_->show();
    numericInput_->raise();
    numericInput_->setFocus();
}

void SketchController::emitChanged()
{
    if (changed_) {
        changed_();
    }
}

void SketchController::emitMessage(const QString& message)
{
    if (message_) {
        message_(message);
    }
}

void SketchController::syncOverlayGeometry()
{
    if (overlay_ != nullptr && viewportHost_ != nullptr) {
        overlay_->setGeometry(viewportHost_->rect());
        overlay_->raise();
    }
}

}  // namespace freeshape::sketch
