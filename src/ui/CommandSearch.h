#pragma once

#include <QFrame>

class QLineEdit;
class QListWidget;
class QPoint;

namespace freeshape::commands {
class CommandRegistry;
}

namespace freeshape::ui {

class CommandSearch final : public QFrame
{
public:
    CommandSearch(
        freeshape::commands::CommandRegistry* registry,
        QWidget* parent = nullptr
    );

    void popupAtGlobal(const QPoint& globalPosition);

private:
    void rebuildResults(const QString& query);
    void executeCurrent();

    freeshape::commands::CommandRegistry* registry_ = nullptr;
    QLineEdit* search_ = nullptr;
    QListWidget* results_ = nullptr;
};

}  // namespace freeshape::ui
