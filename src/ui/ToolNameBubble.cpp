#include "ui/ToolNameBubble.h"

#include <algorithm>

#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace freeshape::ui {

ToolNameBubble::ToolNameBubble(QWidget* owner)
    : QObject(owner)
    , owner_(owner)
{
    bubble_ = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    bubble_->setObjectName(QStringLiteral("ToolNameBubble"));
    bubble_->setAttribute(Qt::WA_ShowWithoutActivating, true);

    auto* root = new QVBoxLayout(bubble_);
    root->setContentsMargins(9, 6, 9, 6);
    root->setSpacing(2);

    auto* first = new QHBoxLayout;
    title_ = new QLabel(bubble_);
    title_->setObjectName(QStringLiteral("ToolBubbleTitle"));
    first->addWidget(title_);

    shortcut_ = new QLabel(bubble_);
    shortcut_->setObjectName(QStringLiteral("ToolBubbleShortcut"));
    first->addWidget(shortcut_);
    root->addLayout(first);

    detail_ = new QLabel(bubble_);
    detail_->setObjectName(QStringLiteral("ToolBubbleDetail"));
    detail_->setWordWrap(true);
    detail_->setMaximumWidth(280);
    root->addWidget(detail_);

    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    timer_->setInterval(360);

    QObject::connect(timer_, &QTimer::timeout, this, [this] {
        if (pending_ != nullptr) {
            showFor(pending_);
        }
    });
}

void ToolNameBubble::watch(
    QToolButton* button,
    const QString& title,
    const QString& shortcut,
    const QString& detail
)
{
    if (button == nullptr) {
        return;
    }

    button->setProperty("toolBubbleTitle", title);
    button->setProperty("toolBubbleShortcut", shortcut);
    button->setProperty("toolBubbleDetail", detail);
    button->setMouseTracking(true);
    button->installEventFilter(this);

    // Avoid a second native tooltip appearing on top of the FreeShape bubble.
    button->setToolTip(QString());
}

bool ToolNameBubble::eventFilter(QObject* watched, QEvent* event)
{
    auto* button = qobject_cast<QToolButton*>(watched);
    if (button == nullptr) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
        case QEvent::Enter:
            schedule(button);
            break;
        case QEvent::Leave:
        case QEvent::MouseButtonPress:
        case QEvent::Hide:
            if (pending_ == button) {
                pending_.clear();
            }
            timer_->stop();
            hideBubble();
            break;
        default:
            break;
    }

    return QObject::eventFilter(watched, event);
}

void ToolNameBubble::schedule(QToolButton* button)
{
    pending_ = button;
    hideBubble();
    timer_->start();
}

void ToolNameBubble::showFor(QToolButton* button)
{
    if (button == nullptr || !button->isVisible()) {
        return;
    }

    const QString title = button->property("toolBubbleTitle").toString();
    const QString shortcut = button->property("toolBubbleShortcut").toString();
    const QString detail = button->property("toolBubbleDetail").toString();

    title_->setText(title);
    shortcut_->setText(shortcut);
    shortcut_->setVisible(!shortcut.isEmpty());
    detail_->setText(detail);
    detail_->setVisible(!detail.isEmpty());

    bubble_->adjustSize();

    QPoint global = button->mapToGlobal(QPoint(0, button->height() + 5));
    QScreen* screen = QGuiApplication::screenAt(global);
    if (screen != nullptr) {
        const QRect available = screen->availableGeometry();
        if (global.x() + bubble_->width() > available.right()) {
            global.setX(available.right() - bubble_->width() - 4);
        }
        if (global.y() + bubble_->height() > available.bottom()) {
            global = button->mapToGlobal(QPoint(0, -bubble_->height() - 5));
        }
        global.setX(std::max(global.x(), available.left() + 4));
        global.setY(std::max(global.y(), available.top() + 4));
    }

    bubble_->move(global);
    bubble_->show();
    bubble_->raise();
}

void ToolNameBubble::hideBubble()
{
    if (bubble_ != nullptr) {
        bubble_->hide();
    }
}

}  // namespace freeshape::ui
