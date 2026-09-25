#include "ui/ReferenceGeometryOverlay.h"

#include <algorithm>

#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Inventor/SbViewVolume.h>
#include <Inventor/nodes/SoCamera.h>

#include <QLabel>
#include <QTimer>

namespace freeshape::ui {

namespace {

QLabel* makePlaneLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("ReferencePlaneLabel"));
    label->adjustSize();
    label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    return label;
}

}  // namespace

ReferenceGeometryOverlay::ReferenceGeometryOverlay(
    Gui::View3DInventor* view,
    QWidget* parent
)
    : QWidget(parent)
    , view_(view)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    top_ = makePlaneLabel(QStringLiteral("Top"), this);
    front_ = makePlaneLabel(QStringLiteral("Front"), this);
    right_ = makePlaneLabel(QStringLiteral("Right"), this);

    timer_ = new QTimer(this);
    timer_->setInterval(50);
    QObject::connect(timer_, &QTimer::timeout, this, [this] {
        updateLabels();
    });
    timer_->start();
}

void ReferenceGeometryOverlay::setReferenceGeometryVisible(bool visible)
{
    referencesVisible_ = visible;
    setVisible(visible);
    if (visible) {
        raise();
        updateLabels();
    }
}

QPoint ReferenceGeometryOverlay::project(float x, float y, float z) const
{
    if (view_ == nullptr || view_->getViewer() == nullptr) {
        return {};
    }

    auto* viewer = view_->getViewer();
    auto* gl = viewer->getGLWidget();
    auto* camera = viewer->getSoRenderManager()->getCamera();
    if (gl == nullptr || camera == nullptr || gl->width() <= 0 || gl->height() <= 0) {
        return {};
    }

    const float aspect = static_cast<float>(gl->width()) / static_cast<float>(gl->height());
    SbViewVolume volume = camera->getViewVolume(aspect);
    SbVec3f point(x, y, z);
    volume.projectToScreen(point, point);

    const int px = static_cast<int>(point[0] * width());
    const int py = static_cast<int>((1.0F - point[1]) * height());
    return QPoint(px, py);
}

void ReferenceGeometryOverlay::updateLabels()
{
    if (!referencesVisible_ || parentWidget() == nullptr) {
        return;
    }

    setGeometry(parentWidget()->rect());

    struct Placement {
        QLabel* label;
        QPoint point;
    };

    const Placement placements[] = {
        {top_, project(25.0F, -21.0F, 0.0F)},
        {front_, project(24.0F, 0.0F, 18.0F)},
        {right_, project(0.0F, 24.0F, 18.0F)},
    };

    for (const auto& placement : placements) {
        placement.label->adjustSize();

        // Never clamp an off-screen 3D label to a random viewport edge. That
        // looked like a toolbar tooltip in the screencast. If the actual
        // reference anchor is off screen, the semantic label is hidden.
        const QRect safe = rect().adjusted(
            -placement.label->width(),
            -placement.label->height(),
            placement.label->width(),
            placement.label->height()
        );
        if (!safe.contains(placement.point)) {
            placement.label->hide();
            continue;
        }

        placement.label->show();
        const int x = std::clamp(
            placement.point.x() - placement.label->width() / 2,
            4,
            std::max(4, width() - placement.label->width() - 4)
        );
        const int y = std::clamp(
            placement.point.y() - placement.label->height() / 2,
            4,
            std::max(4, height() - placement.label->height() - 4)
        );
        placement.label->move(x, y);
    }

    raise();
}

}  // namespace freeshape::ui
