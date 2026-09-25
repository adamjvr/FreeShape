#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

class QFrame;
class QLabel;
class QTimer;
class QToolButton;
class QWidget;

namespace freeshape::ui {

class ToolNameBubble final : public QObject
{
public:
    explicit ToolNameBubble(QWidget* owner);

    void watch(
        QToolButton* button,
        const QString& title,
        const QString& shortcut = {},
        const QString& detail = {}
    );

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void schedule(QToolButton* button);
    void showFor(QToolButton* button);
    void hideBubble();

    QWidget* owner_ = nullptr;
    QFrame* bubble_ = nullptr;
    QLabel* title_ = nullptr;
    QLabel* detail_ = nullptr;
    QLabel* shortcut_ = nullptr;
    QTimer* timer_ = nullptr;
    QPointer<QToolButton> pending_;
};

}  // namespace freeshape::ui
