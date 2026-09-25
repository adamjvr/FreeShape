#include "ui/SelectionOverlay.h"

#include <algorithm>

#include <QBrush>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QRect>

namespace freeshape::ui {

SelectionOverlay::SelectionOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    hide();
}

void SelectionOverlay::beginBox(const QPoint& start)
{
    boxVisible_ = true;
    boxStart_ = start;
    boxCurrent_ = start;
    show();
    raise();
    update();
}

void SelectionOverlay::updateBox(const QPoint& current)
{
    boxCurrent_ = current;
    show();
    raise();
    update();
}

void SelectionOverlay::endBox()
{
    boxVisible_ = false;
    if (preselectionText_.isEmpty() && selectionCount_ == 0) {
        hide();
    }
    update();
}

void SelectionOverlay::setSelectionCount(int count)
{
    selectionCount_ = count;
    if (count > 0) {
        show();
        raise();
    }
    else if (!boxVisible_ && preselectionText_.isEmpty()) {
        hide();
    }
    update();
}

void SelectionOverlay::setPreselection(
    const QString& text,
    const QPoint& cursorPosition
)
{
    preselectionText_ = text;
    cursorPosition_ = cursorPosition;
    show();
    raise();
    update();
}

void SelectionOverlay::clearPreselection()
{
    preselectionText_.clear();
    if (!boxVisible_ && selectionCount_ == 0) {
        hide();
    }
    update();
}

void SelectionOverlay::paintEvent(QPaintEvent* event)
{
    (void)event;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (boxVisible_) {
        const bool crossing = boxCurrent_.x() < boxStart_.x();
        const QRect rect(boxStart_, boxCurrent_);
        const QRect normalized = rect.normalized();

        QColor stroke = crossing ? QColor(214, 166, 31) : QColor(42, 128, 202);
        QColor fill = stroke;
        fill.setAlpha(38);

        QPen pen(stroke);
        pen.setWidthF(1.25);
        if (crossing) {
            pen.setStyle(Qt::DashLine);
        }
        painter.setPen(pen);
        painter.setBrush(fill);
        painter.drawRect(normalized);

        const QString mode = crossing
            ? QStringLiteral("Crossing")
            : QStringLiteral("Window");
        painter.setPen(QColor(50, 54, 58));
        painter.drawText(
            normalized.topLeft() + QPoint(5, -5),
            mode
        );
    }

    if (selectionCount_ > 0) {
        const QString countText = selectionCount_ > 5
            ? QStringLiteral("5+")
            : QString::number(selectionCount_);
        const QRect badge(width() - 47, height() - 51, 34, 28);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(45, 50, 56, 220));
        painter.drawRoundedRect(badge, 8, 8);
        painter.setPen(Qt::white);
        painter.drawText(badge, Qt::AlignCenter, countText);
    }

    if (!preselectionText_.isEmpty()) {
        QFontMetrics metrics(font());
        const int textWidth = metrics.horizontalAdvance(preselectionText_);
        const int boxWidth = std::min(textWidth + 18, 300);
        QPoint pos = cursorPosition_ + QPoint(14, 17);
        pos.setX(std::clamp(pos.x(), 8, std::max(8, width() - boxWidth - 8)));
        pos.setY(std::clamp(pos.y(), 8, std::max(8, height() - 31)));

        QRect pill(pos, QSize(boxWidth, 25));
        painter.setPen(QPen(QColor(125, 150, 175), 1));
        painter.setBrush(QColor(255, 255, 255, 238));
        painter.drawRoundedRect(pill, 4, 4);
        painter.setPen(QColor(48, 54, 60));
        painter.drawText(pill.adjusted(8, 0, -8, 0), Qt::AlignVCenter, preselectionText_);
    }
}

}  // namespace freeshape::ui
