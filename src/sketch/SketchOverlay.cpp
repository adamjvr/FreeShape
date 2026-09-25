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
    previewKind_ = PreviewKind::Circle;
    previewVisible_ = true;
    show();
    raise();
    update();
}

void SketchOverlay::setLinePreview(
    const QPoint& start,
    const QPoint& end,
    bool snapX,
    bool snapY
)
{
    center_ = start;
    edge_ = end;
    snapX_ = snapX;
    snapY_ = snapY;
    previewKind_ = PreviewKind::Line;
    previewVisible_ = true;
    show();
    raise();
    update();
}

void SketchOverlay::setRectanglePreview(
    const QPoint& first,
    const QPoint& opposite,
    bool snapX,
    bool snapY
)
{
    center_ = first;
    edge_ = opposite;
    snapX_ = snapX;
    snapY_ = snapY;
    previewKind_ = PreviewKind::Rectangle;
    previewVisible_ = true;
    show();
    raise();
    update();
}

void SketchOverlay::setCenterRectanglePreview(
    const QPoint& center,
    const QPoint& corner,
    bool snapX,
    bool snapY
)
{
    center_ = center;
    edge_ = corner;
    snapX_ = snapX;
    snapY_ = snapY;
    previewKind_ = PreviewKind::CenterRectangle;
    previewVisible_ = true;
    show();
    raise();
    update();
}

void SketchOverlay::clearPreview()
{
    previewKind_ = PreviewKind::None;
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

    switch (previewKind_) {
        case PreviewKind::Circle:
            painter.drawEllipse(QPointF(center_), radius, radius);
            break;
        case PreviewKind::Line:
            painter.drawLine(center_, edge_);
            break;
        case PreviewKind::Rectangle:
            painter.drawRect(QRect(center_, edge_).normalized());
            break;
        case PreviewKind::CenterRectangle: {
            const QPoint delta = edge_ - center_;
            painter.drawRect(
                QRect(center_ - delta, center_ + delta).normalized()
            );
            break;
        }
        case PreviewKind::None:
            break;
    }

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
