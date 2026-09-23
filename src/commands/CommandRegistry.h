#pragma once

#include <functional>

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QString>

class QAction;
class QWidget;

namespace freeshape::commands {

class CommandRegistry
{
public:
    explicit CommandRegistry(QWidget* host);

    QAction* add(
        const QString& id,
        const QString& text,
        const QKeySequence& shortcut,
        std::function<void()> callback
    );

    QAction* action(const QString& id) const;
    QList<QAction*> actions() const;
    bool trigger(const QString& id) const;

private:
    QWidget* host_ = nullptr;
    QHash<QString, QAction*> actions_;
};

}  // namespace freeshape::commands
