#include "app/FreeShapeWindow.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderPlane.h>

#include "commands/CommandRegistry.h"
#include "input/ViewportInteractionFilter.h"
#include "sketch/SketchController.h"
#include "ui/CommandSearch.h"
#include "ui/DesignSystem.h"
#include "ui/DocumentTabs.h"
#include "ui/FeaturePopup.h"
#include "ui/PartStudioPanel.h"
#include "ui/ShortcutPalette.h"

#include <QAction>
#include <QComboBox>
#include <QCursor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSizePolicy>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace freeshape::app {

namespace {

class SelectionProbe final : public Gui::SelectionObserver
{
public:
    using Callback = std::function<void(const Gui::SelectionChanges&)>;

    explicit SelectionProbe(Callback callback)
        : Gui::SelectionObserver(true, Gui::ResolveMode::NoResolve)
        , callback_(std::move(callback))
    {}

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            Base::Console().message(
                "FREESHAPE_SELECTION doc={} object={} sub={} type={} picked={} "
                "xyz=({:.6f},{:.6f},{:.6f})\n",
                msg.pDocName != nullptr ? msg.pDocName : "",
                msg.pObjectName != nullptr ? msg.pObjectName : "",
                msg.pSubName != nullptr ? msg.pSubName : "",
                msg.pTypeName != nullptr ? msg.pTypeName : "",
                msg.hasPickedPoint ? "true" : "false",
                msg.x,
                msg.y,
                msg.z
            );
        }

        if (callback_) {
            callback_(msg);
        }
    }

    Callback callback_;
};

QWidget* stretchWidget(QWidget* parent)
{
    auto* stretch = new QWidget(parent);
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return stretch;
}

}  // namespace

FreeShapeWindow::FreeShapeWindow(App::Document* document, QWidget* parent)
    : QMainWindow(parent)
    , document_(document)
{
    if (document_ == nullptr) {
        throw std::invalid_argument("FreeShapeWindow requires an App::Document");
    }

    guiDocument_ = Gui::Application::Instance->getDocument(document_);
    if (guiDocument_ == nullptr) {
        throw std::runtime_error("FreeCAD GUI document was not created after reload");
    }

    setObjectName(QStringLiteral("FreeShapeMainWindow"));
    setWindowTitle(QStringLiteral("FreeShape"));
    resize(1500, 930);
    setMinimumSize(1000, 650);
    setStyleSheet(freeshape::ui::DesignSystem::applicationStyleSheet());
    statusBar()->hide();

    configureViewportBehavior();

    auto* mdiView = guiDocument_->createView(
        Gui::View3DInventor::getClassTypeId(),
        Gui::CreateViewMode::Clone
    );

    view_ = dynamic_cast<Gui::View3DInventor*>(mdiView);
    if (view_ == nullptr) {
        throw std::runtime_error("FreeCAD failed to create a View3DInventor");
    }

    view_->getViewer()->setNavigationType(
        Gui::TinkerCADNavigationStyle::getClassTypeId()
    );

    buildCommands();
    buildToolbars();
    buildShell();

    sketchController_ =
        std::make_unique<freeshape::sketch::SketchController>(
            document_,
            guiDocument_,
            view_,
            viewportHost_
        );

    sketchController_->setMessageHandler([this](const QString& message) {
        showToast(message, 3200);
    });

    sketchController_->setChangedHandler([this] {
        refreshPartStudio();
        updateToolbarContext(sketchController_ != nullptr
                             && sketchController_->isEditing());
    });

    refreshPartStudio();

    // FreeCAD origin planes are children of the hidden App::Origin view
    // provider. Showing XY/XZ/YZ alone is insufficient because the parent's
    // visibility switch still clips them from the Coin scene.
    setReferencePlanesVisible(true);

    selectionObserver_ = std::make_unique<SelectionProbe>(
        [this](const Gui::SelectionChanges& msg) {
            if (msg.Type == Gui::SelectionChanges::ClrSelection) {
                showToast(QStringLiteral("Selection cleared"));
                return;
            }

            if (msg.Type != Gui::SelectionChanges::AddSelection) {
                return;
            }

            const QString object =
                QString::fromUtf8(msg.pObjectName != nullptr ? msg.pObjectName : "");
            const QString sub =
                QString::fromUtf8(msg.pSubName != nullptr ? msg.pSubName : "");

            if (sketchController_ != nullptr
                && sketchController_->isAwaitingPlane()
                && (object == QStringLiteral("XY_Plane")
                    || object == QStringLiteral("XZ_Plane")
                    || object == QStringLiteral("YZ_Plane"))) {
                if (sketchController_->selectPlane(object)) {
                    featurePopup_->setSelectionText(
                        object == QStringLiteral("XY_Plane")
                            ? QStringLiteral("Top")
                            : object == QStringLiteral("XZ_Plane")
                                  ? QStringLiteral("Front")
                                  : QStringLiteral("Right")
                    );
                    setReferencePlanesVisible(false);
                    updateToolbarContext(true);
                    refreshPartStudio();
                }
            }

            if (partStudioPanel_ != nullptr) {
                partStudioPanel_->setSelectedFeature(object);
            }

            QString text = QStringLiteral("%1").arg(object);
            if (!sub.isEmpty()) {
                text += QStringLiteral(" · %1").arg(sub);
            }
            showToast(text);
        }
    );

    Gui::Selection().enablePickedList(true);

    if (auto* glWidget = view_->getViewer()->getGLWidget(); glWidget != nullptr) {
        viewportInput_ =
            std::make_unique<freeshape::input::ViewportInteractionFilter>(
                guiDocument_,
                glWidget
            );

        viewportInput_->setContextMenuHandler(
            [this](const QPoint& globalPosition) {
                showViewportContextMenu(globalPosition);
            }
        );

        viewportInput_->setSketchHandlers(
            [this](const QPoint& position, Qt::MouseButton button) {
                return sketchController_ != nullptr
                       && sketchController_->handleMousePress(position, button);
            },
            [this](const QPoint& position) {
                if (sketchController_ != nullptr) {
                    sketchController_->handleMouseMove(position);
                }
            }
        );

        glWidget->installEventFilter(viewportInput_.get());
    }

    QTimer::singleShot(0, this, [this] {
        if (view_ != nullptr) {
            view_->viewAll();
            view_->update();
        }
        showToast(
            QStringLiteral(
                "Onshape interaction profile · RMB orbit · MMB pan · wheel zoom · S shortcuts"
            ),
            4200
        );
    });
}

FreeShapeWindow::~FreeShapeWindow()
{
    if (extrudePreviewActive_) {
        finishExtrudePreview(false);
    }
}

void FreeShapeWindow::configureViewportBehavior()
{
    auto prefs = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    prefs->SetASCII("NavigationStyle", "Gui::TinkerCADNavigationStyle");
    prefs->SetBool("SameStyleForAllViews", true);
}

void FreeShapeWindow::buildCommands()
{
    commands_ = std::make_unique<freeshape::commands::CommandRegistry>(this);

    commands_->add(
        QStringLiteral("shortcut_palette"),
        QStringLiteral("Shortcut toolbar"),
        QKeySequence(QStringLiteral("S")),
        [this] { showShortcutPalette(); }
    );

    commands_->add(
        QStringLiteral("search_tools"),
        QStringLiteral("Search tools"),
        QKeySequence(QStringLiteral("Alt+C")),
        [this] { showCommandSearch(); }
    );

    commands_->add(
        QStringLiteral("sketch"),
        QStringLiteral("Sketch"),
        QKeySequence(QStringLiteral("Shift+S")),
        [this] { showSketchCommand(); }
    );

    commands_->add(
        QStringLiteral("extrude"),
        QStringLiteral("Extrude"),
        QKeySequence(QStringLiteral("Shift+E")),
        [this] { showExtrudeCommand(); }
    );

    commands_->add(
        QStringLiteral("fillet"),
        QStringLiteral("Fillet"),
        QKeySequence(QStringLiteral("Shift+F")),
        [this] { showFilletCommand(); }
    );

    commands_->add(
        QStringLiteral("sketch_circle"),
        QStringLiteral("Center point circle"),
        QKeySequence(QStringLiteral("C")),
        [this] {
            if (sketchController_ != nullptr && sketchController_->isEditing()) {
                sketchController_->activateCircle();
            }
            else {
                showToast(QStringLiteral("Circle requires an active sketch"));
            }
        }
    );

    commands_->add(
        QStringLiteral("fit"),
        QStringLiteral("Fit"),
        QKeySequence(QStringLiteral("F")),
        [this] {
            if (view_ != nullptr) {
                view_->viewAll();
            }
        }
    );

    commands_->add(
        QStringLiteral("zoom_out"),
        QStringLiteral("Zoom out"),
        QKeySequence(QStringLiteral("Z")),
        [this] {
            if (guiDocument_ != nullptr) {
                guiDocument_->sendMsgToViews("ZoomOut");
            }
        }
    );

    commands_->add(
        QStringLiteral("zoom_in"),
        QStringLiteral("Zoom in"),
        QKeySequence(QStringLiteral("Shift+Z")),
        [this] {
            if (guiDocument_ != nullptr) {
                guiDocument_->sendMsgToViews("ZoomIn");
            }
        }
    );

    commands_->add(
        QStringLiteral("clear_selection"),
        QStringLiteral("Clear selection"),
        QKeySequence(Qt::Key_Space),
        [] {
            Gui::Selection().clearCompleteSelection();
        }
    );

    commands_->add(
        QStringLiteral("toggle_planes"),
        QStringLiteral("Planes"),
        QKeySequence(QStringLiteral("P")),
        [this] { togglePlanes(); }
    );

    commands_->add(
        QStringLiteral("toggle_sketches"),
        QStringLiteral("Sketches"),
        QKeySequence(QStringLiteral("Shift+H")),
        [this] { toggleSketches(); }
    );

    commands_->add(
        QStringLiteral("hide"),
        QStringLiteral("Hide"),
        QKeySequence(QStringLiteral("Y")),
        [this] { hideSelected(); }
    );

    commands_->add(
        QStringLiteral("show_hidden"),
        QStringLiteral("Show hidden"),
        QKeySequence(QStringLiteral("Shift+Y")),
        [this] { showLastHidden(); }
    );

    commands_->add(
        QStringLiteral("select_other"),
        QStringLiteral("Select other"),
        QKeySequence(QStringLiteral("`")),
        [this] { cycleSelectOther(); }
    );

    commands_->add(
        QStringLiteral("normal_to"),
        QStringLiteral("Normal to"),
        QKeySequence(QStringLiteral("N")),
        [this] { normalToSelectionOrPlane(); }
    );

    struct ViewShortcut {
        const char* id;
        const char* name;
        const char* shortcut;
        const char* message;
    };

    const ViewShortcut views[] = {
        {"view_front", "Front", "Shift+1", "ViewFront"},
        {"view_rear", "Rear", "Shift+2", "ViewRear"},
        {"view_left", "Left", "Shift+3", "ViewLeft"},
        {"view_right", "Right", "Shift+4", "ViewRight"},
        {"view_top", "Top", "Shift+5", "ViewTop"},
        {"view_bottom", "Bottom", "Shift+6", "ViewBottom"},
        {"view_iso", "Isometric", "Shift+7", "ViewAxonometric"},
    };

    for (const auto& item : views) {
        commands_->add(
            QString::fromUtf8(item.id),
            QString::fromUtf8(item.name),
            QKeySequence(QString::fromUtf8(item.shortcut)),
            [this, message = std::string(item.message)] {
                if (guiDocument_ != nullptr) {
                    guiDocument_->sendMsgToViews(message.c_str());
                }
            }
        );
    }

    commands_->add(
        QStringLiteral("accept_feature"),
        QStringLiteral("Accept feature"),
        QKeySequence(Qt::Key_Return),
        [this] {
            if (featurePopup_ != nullptr && featurePopup_->isVisible()) {
                acceptFeatureCommand();
            }
        }
    );

    commands_->add(
        QStringLiteral("cancel_feature"),
        QStringLiteral("Cancel feature"),
        QKeySequence(Qt::Key_Escape),
        [this] {
            if (featurePopup_ != nullptr && featurePopup_->isVisible()) {
                cancelFeatureCommand();
            }
            else if (shortcutPalette_ != nullptr) {
                shortcutPalette_->hide();
            }
        }
    );
}

void FreeShapeWindow::buildToolbars()
{
    documentToolbar_ = new QToolBar(this);
    documentToolbar_->setObjectName(QStringLiteral("DocumentBar"));
    documentToolbar_->setMovable(false);
    documentToolbar_->setFloatable(false);

    auto* menu = new QToolButton(documentToolbar_);
    menu->setText(QStringLiteral("☰"));
    menu->setToolTip(QStringLiteral("Document menu"));
    documentToolbar_->addWidget(menu);

    auto* brand = new QLabel(QStringLiteral("FreeShape"), documentToolbar_);
    brand->setObjectName(QStringLiteral("BrandLabel"));
    documentToolbar_->addWidget(brand);

    auto* workspace = new QLabel(QStringLiteral("Main"), documentToolbar_);
    workspace->setObjectName(QStringLiteral("WorkspaceLabel"));
    documentToolbar_->addWidget(workspace);

    auto* link = new QToolButton(documentToolbar_);
    link->setText(QStringLiteral("↗"));
    link->setToolTip(QStringLiteral("Document link"));
    documentToolbar_->addWidget(link);

    documentToolbar_->addWidget(stretchWidget(documentToolbar_));

    auto* search = new QLineEdit(documentToolbar_);
    search->setPlaceholderText(QStringLiteral("Search tools…  Alt+C"));
    search->setReadOnly(true);
    search->setCursor(Qt::PointingHandCursor);
    search->setFixedWidth(170);
    search->setToolTip(QStringLiteral("Search all FreeShape commands (Alt+C)"));
    documentToolbar_->addWidget(search);

    auto* help = new QToolButton(documentToolbar_);
    help->setText(QStringLiteral("?"));
    documentToolbar_->addWidget(help);

    addToolBar(Qt::TopToolBarArea, documentToolbar_);

    featureToolbar_ = new QToolBar(this);
    featureToolbar_->setObjectName(QStringLiteral("FeatureBar"));
    featureToolbar_->setMovable(false);
    featureToolbar_->setFloatable(false);

    auto addCommandButton = [&](const QString& commandId, const QString& text) {
        auto* button = new QToolButton(featureToolbar_);
        button->setText(text);
        button->setToolTip(commands_->action(commandId)->text()
                           + QStringLiteral("  ")
                           + commands_->action(commandId)->shortcut().toString());
        QObject::connect(button, &QToolButton::clicked, this, [this, commandId] {
            commands_->trigger(commandId);
        });
        featureToolbar_->addWidget(button);
    };

    addCommandButton(QStringLiteral("sketch"), QStringLiteral("✎ Sketch"));
    featureToolbar_->addSeparator();
    addCommandButton(QStringLiteral("extrude"), QStringLiteral("▰ Extrude"));
    featureToolbar_->addWidget(new QToolButton(featureToolbar_));

    const QStringList passiveFeatures = {
        QStringLiteral("Revolve"),
        QStringLiteral("Sweep"),
        QStringLiteral("Loft"),
        QStringLiteral("Hole"),
        QStringLiteral("Chamfer"),
        QStringLiteral("Shell"),
        QStringLiteral("Draft"),
        QStringLiteral("Pattern"),
        QStringLiteral("Mirror"),
        QStringLiteral("Boolean"),
        QStringLiteral("Transform")
    };

    for (const auto& text : passiveFeatures) {
        auto* button = new QToolButton(featureToolbar_);
        button->setText(text);
        button->setToolTip(text + QStringLiteral(" — command foundation"));
        featureToolbar_->addWidget(button);
    }

    addCommandButton(QStringLiteral("fillet"), QStringLiteral("Fillet"));

    addToolBar(Qt::TopToolBarArea, featureToolbar_);

    sketchToolbar_ = new QToolBar(this);
    sketchToolbar_->setObjectName(QStringLiteral("SketchBar"));
    sketchToolbar_->setMovable(false);
    sketchToolbar_->setFloatable(false);

    for (const QString& text : {
             QStringLiteral("Line  L"),
             QStringLiteral("Circle  C"),
             QStringLiteral("Rectangle  R"),
             QStringLiteral("Arc  A"),
             QStringLiteral("Dimension  D"),
             QStringLiteral("Construction  Q"),
             QStringLiteral("Trim  M"),
             QStringLiteral("Use  U")
         }) {
        auto* button = new QToolButton(sketchToolbar_);
        button->setText(text);
        sketchToolbar_->addWidget(button);
    }

    addToolBar(Qt::TopToolBarArea, sketchToolbar_);
    sketchToolbar_->hide();
}

void FreeShapeWindow::buildShell()
{
    shell_ = new QWidget(this);
    auto* root = new QVBoxLayout(shell_);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* workspace = new QWidget(shell_);
    auto* workspaceLayout = new QHBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    partStudioPanel_ = new freeshape::ui::PartStudioPanel(workspace);
    workspaceLayout->addWidget(partStudioPanel_);

    viewportHost_ = new QWidget(workspace);
    viewportHost_->setMinimumSize(640, 480);
    viewportHost_->setStyleSheet(QStringLiteral("background:#ffffff;"));

    auto* viewportLayout = new QVBoxLayout(viewportHost_);
    viewportLayout->setContentsMargins(0, 0, 0, 0);
    viewportLayout->setSpacing(0);

    view_->setParent(viewportHost_);
    view_->setWindowFlags(Qt::Widget);
    view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    viewportLayout->addWidget(view_, 1);
    view_->show();

    featurePopup_ = new freeshape::ui::FeaturePopup(viewportHost_);
    featurePopup_->move(14, 14);
    featurePopup_->raise();

    featurePopup_->setAcceptHandler([this] {
        acceptFeatureCommand();
    });
    featurePopup_->setCancelHandler([this] {
        cancelFeatureCommand();
    });
    featurePopup_->setDepthChangedHandler([this](double depth) {
        applyExtrudePreview(depth);
    });

    shortcutPalette_ =
        new freeshape::ui::ShortcutPalette(commands_.get(), viewportHost_);

    commandSearch_ =
        new freeshape::ui::CommandSearch(commands_.get(), viewportHost_);

    toast_ = new QLabel(viewportHost_);
    toast_->setObjectName(QStringLiteral("ViewportToast"));
    toast_->hide();
    toast_->raise();

    workspaceLayout->addWidget(viewportHost_, 1);
    root->addWidget(workspace, 1);

    documentTabs_ = new freeshape::ui::DocumentTabs(shell_);
    root->addWidget(documentTabs_);

    setCentralWidget(shell_);

    partStudioPanel_->setPlaneActivatedHandler(
        [this](const QString& objectName) {
            Gui::Selection().clearCompleteSelection();
            Gui::Selection().addSelection(
                document_->getName(),
                objectName.toUtf8().constData()
            );

            if (sketchController_ != nullptr
                && sketchController_->isAwaitingPlane()) {
                const QString label =
                    objectName == QStringLiteral("XY_Plane") ? QStringLiteral("Top")
                    : objectName == QStringLiteral("XZ_Plane") ? QStringLiteral("Front")
                                                               : QStringLiteral("Right");

                if (sketchController_->selectPlane(objectName)) {
                    featurePopup_->setSelectionText(label);
                    setReferencePlanesVisible(false);
                    updateToolbarContext(true);
                    refreshPartStudio();
                }
            }
        }
    );

    partStudioPanel_->setFeatureActivatedHandler(
        [this](const QString& objectName) {
            Gui::Selection().clearCompleteSelection();
            Gui::Selection().addSelection(
                document_->getName(),
                objectName.toUtf8().constData()
            );

            if (objectName == QStringLiteral("Pad")) {
                showExtrudeCommand();
            }
        }
    );
}

void FreeShapeWindow::showSketchCommand()
{
    if (extrudePreviewActive_) {
        finishExtrudePreview(false);
    }

    if (sketchController_ != nullptr) {
        sketchController_->beginSketch();
    }

    // Selecting the sketch support should feel like Onshape: the three base
    // reference planes are immediately visible and selectable.
    setReferencePlanesVisible(true);

    featurePopup_->setMode(freeshape::ui::FeaturePopup::Mode::Sketch);
    featurePopup_->setSelectionText(QStringLiteral("Select a sketch plane"));
    featurePopup_->move(14, 14);
    featurePopup_->show();
    featurePopup_->raise();

    updateToolbarContext(false);
    showToast(QStringLiteral("Sketch · select Top, Front, Right, or a planar face"));
}

void FreeShapeWindow::showExtrudeCommand()
{
    if (featurePopup_->isVisible()
        && featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Extrude) {
        return;
    }

    if (sketchController_ != nullptr && sketchController_->isEditing()) {
        if (!sketchController_->finishSketch(true)) {
            showToast(QStringLiteral("Extrude requires a closed sketch region"));
            return;
        }
        refreshPartStudio();
    }

    if (!beginExtrudePreview()) {
        showToast(QStringLiteral("Extrude requires Sketch 1 or a selected planar face"));
        return;
    }

    featurePopup_->setMode(freeshape::ui::FeaturePopup::Mode::Extrude);
    featurePopup_->setDepth(10.0);
    featurePopup_->move(14, 14);
    featurePopup_->show();
    featurePopup_->raise();

    updateToolbarContext(false);
    showToast(
        QStringLiteral(
            "Extrude · Sketch 1 region auto-selected · type depth · Enter accepts"
        ),
        4200
    );
}

void FreeShapeWindow::showFilletCommand()
{
    if (extrudePreviewActive_) {
        finishExtrudePreview(false);
    }

    featurePopup_->setMode(freeshape::ui::FeaturePopup::Mode::Fillet);
    featurePopup_->move(14, 14);
    featurePopup_->show();
    featurePopup_->raise();

    updateToolbarContext(false);
    showToast(QStringLiteral("Fillet · select edges or faces"));
}

void FreeShapeWindow::acceptFeatureCommand()
{
    if (featurePopup_ == nullptr || !featurePopup_->isVisible()) {
        return;
    }

    if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Extrude) {
        finishExtrudePreview(true);
    }

    if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Sketch) {
        if (sketchController_ != nullptr) {
            sketchController_->finishSketch(true);
        }
        refreshPartStudio();
    }

    featurePopup_->hide();
    updateToolbarContext(false);
}

void FreeShapeWindow::cancelFeatureCommand()
{
    if (featurePopup_ == nullptr || !featurePopup_->isVisible()) {
        return;
    }

    if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Extrude) {
        finishExtrudePreview(false);
    }
    else if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Sketch
             && sketchController_ != nullptr) {
        sketchController_->finishSketch(false);
        setReferencePlanesVisible(true);
        refreshPartStudio();
    }

    featurePopup_->hide();
    updateToolbarContext(false);
    showToast(QStringLiteral("Command cancelled"));
}

bool FreeShapeWindow::beginExtrudePreview()
{
    if (extrudePreviewActive_ || document_ == nullptr) {
        return extrudePreviewActive_;
    }

    if (document_->getObject("Sketch") == nullptr) {
        return false;
    }

    document_->openTransaction("FreeShape Extrude Preview");
    extrudePreviewActive_ = true;
    extrudeCreatedPad_ = document_->getObject("Pad") == nullptr;

    if (extrudeCreatedPad_) {
        std::ostringstream script;
        script
            << "import FreeCAD as App\n"
            << "doc = App.getDocument('" << document_->getName() << "')\n"
            << "body = doc.getObject('Body')\n"
            << "sketch = doc.getObject('Sketch')\n"
            << "pad = body.newObject('PartDesign::Pad', 'Pad')\n"
            << "pad.Label = 'Extrude 1'\n"
            << "pad.Profile = sketch\n"
            << "pad.Length = 10.0\n"
            << "doc.recompute()\n";
        Base::Interpreter().runString(script.str().c_str());
        document_->recompute();

        if (document_->getObject("Pad") == nullptr) {
            document_->abortTransaction();
            extrudePreviewActive_ = false;
            extrudeCreatedPad_ = false;
            return false;
        }

        Base::Console().message("FREESHAPE_EXTRUDE_BEGIN PASS profile=Sketch\n");
    }

    refreshPartStudio();
    return true;
}

void FreeShapeWindow::applyExtrudePreview(double depth)
{
    if (!extrudePreviewActive_ || document_ == nullptr) {
        return;
    }

    std::ostringstream script;
    script
        << "doc = App.getDocument('" << document_->getName() << "')\n"
        << "pad = doc.getObject('Pad')\n"
        << "pad.Length = " << depth << "\n"
        << "doc.recompute()\n";

    Base::Interpreter().runString(script.str().c_str());
}

void FreeShapeWindow::finishExtrudePreview(bool accept)
{
    if (!extrudePreviewActive_ || document_ == nullptr) {
        return;
    }

    if (accept) {
        document_->commitTransaction();
        guiDocument_->setHide("Sketch");
        Base::Console().message(
            "FREESHAPE_EXTRUDE PASS depth={:.6f}\\n",
            featurePopup_ != nullptr ? featurePopup_->depth() : 0.0
        );
        showToast(QStringLiteral("Extrude accepted · Part 1 created"));
    }
    else {
        document_->abortTransaction();
        document_->recompute();
        guiDocument_->setShow("Sketch");
        Base::Console().message("FREESHAPE_EXTRUDE CANCEL\\n");
        showToast(QStringLiteral("Extrude cancelled"));
    }

    extrudePreviewActive_ = false;
    extrudeCreatedPad_ = false;
    refreshPartStudio();
}

void FreeShapeWindow::showShortcutPalette()
{
    if (shortcutPalette_ == nullptr) {
        return;
    }

    shortcutPalette_->setContext(
        sketchController_ != nullptr && sketchController_->isEditing()
            ? freeshape::ui::ShortcutPalette::Context::Sketch
            : freeshape::ui::ShortcutPalette::Context::PartStudio
    );
    shortcutPalette_->popupAtGlobal(QCursor::pos());
}

void FreeShapeWindow::showCommandSearch()
{
    if (commandSearch_ == nullptr || viewportHost_ == nullptr) {
        return;
    }

    const QPoint global = viewportHost_->mapToGlobal(
        QPoint(std::max(12, (viewportHost_->width() - 420) / 2), 12)
    );
    commandSearch_->popupAtGlobal(global);
}

void FreeShapeWindow::showViewportContextMenu(const QPoint& globalPosition)
{
    QMenu menu(this);

    menu.addAction(QStringLiteral("Sketch"), [this] {
        showSketchCommand();
    });
    menu.addAction(QStringLiteral("Extrude"), [this] {
        showExtrudeCommand();
    });
    menu.addAction(QStringLiteral("Fillet"), [this] {
        showFilletCommand();
    });

    menu.addSeparator();

    menu.addAction(QStringLiteral("View normal to"), [this] {
        normalToSelectionOrPlane();
    });
    menu.addAction(QStringLiteral("Zoom to fit"), [this] {
        commands_->trigger(QStringLiteral("fit"));
    });
    menu.addAction(QStringLiteral("Select other"), [this] {
        cycleSelectOther();
    });

    menu.addSeparator();

    menu.addAction(QStringLiteral("Hide"), [this] {
        hideSelected();
    });
    menu.addAction(QStringLiteral("Clear selection"), [] {
        Gui::Selection().clearCompleteSelection();
    });

    menu.exec(globalPosition);
}

void FreeShapeWindow::togglePlanes()
{
    setReferencePlanesVisible(!planesVisible_);

    showToast(
        planesVisible_ ? QStringLiteral("Planes shown")
                       : QStringLiteral("Planes hidden")
    );
}

void FreeShapeWindow::setReferencePlanesVisible(bool visible)
{
    planesVisible_ = visible;

    // PartDesign Body owns an App::Origin. In FreeCAD's scene graph the Body
    // claims Origin, and Origin claims its planes. Parent visibility therefore
    // gates the child plane ViewProviders.
    if (visible) {
        guiDocument_->setShow("Body");
        guiDocument_->setShow("Origin");

        // FreeShape's default-geometry presentation is intentionally planes
        // only. Keep the FreeCAD origin axes out of the initial Part Studio.
        for (const char* axis : {"X_Axis", "Y_Axis", "Z_Axis"}) {
            guiDocument_->setHide(axis);
        }

        for (const char* plane : {"XY_Plane", "XZ_Plane", "YZ_Plane"}) {
            guiDocument_->setShow(plane);
        }

        styleReferencePlanes();

        Base::Console().message(
            "FREESHAPE_REFERENCE_PLANES PASS origin={} xy={} xz={} yz={}\n",
            guiDocument_->isShow("Origin"),
            guiDocument_->isShow("XY_Plane"),
            guiDocument_->isShow("XZ_Plane"),
            guiDocument_->isShow("YZ_Plane")
        );
    }
    else {
        for (const char* plane : {"XY_Plane", "XZ_Plane", "YZ_Plane"}) {
            guiDocument_->setHide(plane);
        }
        guiDocument_->setHide("Origin");
    }

    if (view_ != nullptr) {
        view_->update();
    }
}

void FreeShapeWindow::styleReferencePlanes()
{
    // Current FreeCAD origin-plane ViewProviders are already translucent and
    // selectable. Keep those engine-backed pick targets, but make them behave
    // more like persistent Part Studio reference planes: always labeled and
    // a little larger than FreeCAD's default datum size.
    for (const char* name : {"XY_Plane", "XZ_Plane", "YZ_Plane"}) {
        auto* object = document_ != nullptr ? document_->getObject(name) : nullptr;
        if (object == nullptr) {
            continue;
        }

        auto* provider = Gui::Application::Instance->getViewProvider(object);
        auto* plane = dynamic_cast<Gui::ViewProviderPlane*>(provider);
        if (plane == nullptr) {
            continue;
        }

        plane->resetTemporarySize();
        plane->setTemporaryScale(2.25);
        plane->setLabelVisibility(true);
    }
}

void FreeShapeWindow::toggleSketches()
{
    sketchVisible_ = !sketchVisible_;
    if (sketchVisible_) {
        guiDocument_->setShow("Sketch");
    }
    else {
        guiDocument_->setHide("Sketch");
    }

    showToast(
        sketchVisible_ ? QStringLiteral("Sketches shown")
                       : QStringLiteral("Sketches hidden")
    );
}

void FreeShapeWindow::hideSelected()
{
    lastHiddenObjects_.clear();

    for (const auto& selected : Gui::Selection().getSelection()) {
        if (selected.FeatName == nullptr) {
            continue;
        }
        const std::string name = selected.FeatName;
        if (std::find(lastHiddenObjects_.begin(), lastHiddenObjects_.end(), name)
            == lastHiddenObjects_.end()) {
            lastHiddenObjects_.push_back(name);
            guiDocument_->setHide(name.c_str());
        }
    }

    if (!lastHiddenObjects_.empty()) {
        Gui::Selection().clearCompleteSelection();
        showToast(QStringLiteral("Hidden selected entities"));
    }
}

void FreeShapeWindow::showLastHidden()
{
    for (const auto& name : lastHiddenObjects_) {
        guiDocument_->setShow(name.c_str());
    }

    if (!lastHiddenObjects_.empty()) {
        showToast(QStringLiteral("Previously hidden entities shown"));
    }
}

void FreeShapeWindow::cycleSelectOther()
{
    auto& selection = Gui::Selection();
    const auto picked = selection.getPickedList(document_->getName());

    if (picked.empty()) {
        showToast(
            QStringLiteral("Select Other · hover/click geometry first"),
            2600
        );
        return;
    }

    selectOtherIndex_ %= picked.size();
    const auto& item = picked[selectOtherIndex_];
    ++selectOtherIndex_;

    selection.clearCompleteSelection(false);
    selection.addSelection(
        item.DocName,
        item.FeatName,
        item.SubName,
        item.x,
        item.y,
        item.z,
        nullptr,
        false,
        Gui::SelectionChanges::PickedPoint::Valid
    );

    showToast(
        QStringLiteral("Select Other · %1.%2  (` cycles)")
            .arg(QString::fromUtf8(item.FeatName != nullptr ? item.FeatName : ""))
            .arg(QString::fromUtf8(item.SubName != nullptr ? item.SubName : ""))
    );
}

void FreeShapeWindow::normalToSelectionOrPlane()
{
    const auto selected = Gui::Selection().getSelection();
    if (selected.empty()) {
        showToast(QStringLiteral("Normal to · select or hover a plane/face"));
        return;
    }

    const std::string object =
        selected.back().FeatName != nullptr ? selected.back().FeatName : "";

    const char* message = nullptr;
    if (object == "XY_Plane") {
        message = "ViewTop";
    }
    else if (object == "XZ_Plane") {
        message = "ViewFront";
    }
    else if (object == "YZ_Plane") {
        message = "ViewRight";
    }

    if (message != nullptr) {
        guiDocument_->sendMsgToViews(message);
        showToast(QStringLiteral("View normal to selected datum plane"));
    }
    else {
        showToast(
            QStringLiteral("Normal-to arbitrary face is queued for ViewportInput pass 2"),
            3200
        );
    }
}

void FreeShapeWindow::showToast(const QString& text, int milliseconds)
{
    if (toast_ == nullptr || viewportHost_ == nullptr) {
        return;
    }

    toast_->setText(text);
    toast_->adjustSize();
    toast_->move(
        14,
        std::max(14, viewportHost_->height() - toast_->height() - 14)
    );
    toast_->show();
    toast_->raise();

    QTimer::singleShot(milliseconds, toast_, [label = toast_, text] {
        if (label->text() == text) {
            label->hide();
        }
    });
}

void FreeShapeWindow::updateToolbarContext(bool sketchMode)
{
    featureToolbar_->setVisible(!sketchMode);
    sketchToolbar_->setVisible(sketchMode);

    if (commands_ != nullptr) {
        if (auto* circle = commands_->action(QStringLiteral("sketch_circle"));
            circle != nullptr) {
            circle->setEnabled(sketchMode);
        }
    }
}

void FreeShapeWindow::refreshPartStudio()
{
    if (partStudioPanel_ != nullptr) {
        partStudioPanel_->refreshFromDocument(document_);
    }
}

}  // namespace freeshape::app
