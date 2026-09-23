#include "commands/CommandRegistry.h"

#include <utility>

#include <QAction>
#include <QWidget>

namespace freeshape::commands {

CommandRegistry::CommandRegistry(QWidget* host)
    : host_(host)
{}

QAction* CommandRegistry::add(
    const QString& id,
    const QString& text,
    const QKeySequence& shortcut,
    std::function<void()> callback
)
{
    auto* action = new QAction(text, host_);
    action->setObjectName(QStringLiteral("command_") + id);
    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
        action->setShortcutContext(Qt::ApplicationShortcut);
    }

    QObject::connect(action, &QAction::triggered, host_, [callback = std::move(callback)] {
        callback();
    });

    host_->addAction(action);
    actions_.insert(id, action);
    return action;
}

QAction* CommandRegistry::action(const QString& id) const
{
    return actions_.value(id, nullptr);
}

QList<QAction*> CommandRegistry::actions() const
{
    return actions_.values();
}

bool CommandRegistry::trigger(const QString& id) const
{
    if (auto* item = action(id); item != nullptr && item->isEnabled()) {
        item->trigger();
        return true;
    }
    return false;
}

}  // namespace freeshape::commands
