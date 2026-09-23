#include "sketch/SketchOverlay.h"

#include <cmath>

#include <QPainter>
#include <QPen>

namespace freeshape::sketch {

SketchOverlay::SketchOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    hide();
}

void SketchOverlay::setCirclePreview(
    const QPoint& center,
    const QPoint& edge,
    bool snapX,
    bool snapY
)
{
    center_ = center;
    edge_ = edge;
    snapX_ = snapX;
    snapY_ = snapY;
    previewVisible_ = true;
    show();
    raise();
    update();
}

void SketchOverlay::clearPreview()
{
    previewVisible_ = false;
    hide();
    update();
}

void SketchOverlay::paintEvent(QPaintEvent* event)
{
    (void)event;

    if (!previewVisible_) {
        return;
    }

    const double dx = edge_.x() - center_.x();
    const double dy = edge_.y() - center_.y();
    const double radius = std::sqrt(dx * dx + dy * dy);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen geometryPen(QColor(48, 124, 194));
    geometryPen.setWidthF(1.6);
    painter.setPen(geometryPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(center_), radius, radius);

    QPen centerPen(QColor(28, 95, 151));
    centerPen.setWidthF(1.2);
    painter.setPen(centerPen);
    painter.drawLine(center_.x() - 6, center_.y(), center_.x() + 6, center_.y());
    painter.drawLine(center_.x(), center_.y() - 6, center_.x(), center_.y() + 6);

    QPen inferencePen(QColor(72, 144, 202));
    inferencePen.setStyle(Qt::DashLine);
    inferencePen.setWidthF(1.0);
    painter.setPen(inferencePen);

    if (snapX_) {
        painter.drawLine(center_.x(), 0, center_.x(), height());
    }
    if (snapY_) {
        painter.drawLine(0, center_.y(), width(), center_.y());
    }
}

}  // namespace freeshape::sketch
