#include "ui/CommandSearch.h"

#include "commands/CommandRegistry.h"

#include <algorithm>

#include <QAction>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPoint>
#include <QVBoxLayout>

namespace freeshape::ui {

namespace {

class SearchLineEdit final : public QLineEdit
{
public:
    using Handler = std::function<void(int)>;

    explicit SearchLineEdit(QWidget* parent = nullptr)
        : QLineEdit(parent)
    {}

    void setKeyHandler(Handler handler)
    {
        handler_ = std::move(handler);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (handler_ != nullptr
            && (event->key() == Qt::Key_Down
                || event->key() == Qt::Key_Up
                || event->key() == Qt::Key_Escape)) {
            handler_(event->key());
            event->accept();
            return;
        }

        QLineEdit::keyPressEvent(event);
    }

private:
    Handler handler_;
};

}  // namespace

CommandSearch::CommandSearch(
    freeshape::commands::CommandRegistry* registry,
    QWidget* parent
)
    : QFrame(parent, Qt::Popup)
    , registry_(registry)
{
    setObjectName(QStringLiteral("CommandSearch"));
    setFixedWidth(420);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(5);

    auto* search = new SearchLineEdit(this);
    search_ = search;
    search_->setPlaceholderText(QStringLiteral("Search tools…"));
    root->addWidget(search_);

    results_ = new QListWidget(this);
    results_->setMinimumHeight(230);
    results_->setMaximumHeight(330);
    root->addWidget(results_);

    QObject::connect(search_, &QLineEdit::textChanged, this,
                     [this](const QString& query) {
                         rebuildResults(query);
                     });

    QObject::connect(search_, &QLineEdit::returnPressed, this, [this] {
        executeCurrent();
    });

    QObject::connect(results_, &QListWidget::itemActivated, this,
                     [this](QListWidgetItem*) {
                         executeCurrent();
                     });

    search->setKeyHandler([this](int key) {
        if (key == Qt::Key_Escape) {
            hide();
            return;
        }

        int row = results_->currentRow();
        if (key == Qt::Key_Down) {
            row = std::min(row + 1, results_->count() - 1);
        }
        else if (key == Qt::Key_Up) {
            row = std::max(row - 1, 0);
        }

        if (results_->count() > 0) {
            results_->setCurrentRow(row < 0 ? 0 : row);
        }
    });

    rebuildResults(QString());
}

void CommandSearch::popupAtGlobal(const QPoint& globalPosition)
{
    rebuildResults(QString());
    search_->clear();
    adjustSize();
    move(globalPosition);
    show();
    raise();
    activateWindow();
    search_->setFocus();
}

void CommandSearch::rebuildResults(const QString& query)
{
    results_->clear();

    if (registry_ == nullptr) {
        return;
    }

    const QString needle = query.trimmed();

    auto actions = registry_->actions();
    std::sort(actions.begin(), actions.end(), [](const QAction* a, const QAction* b) {
        return a->text().localeAwareCompare(b->text()) < 0;
    });

    for (QAction* action : actions) {
        if (action == nullptr || !action->isEnabled()) {
            continue;
        }

        const QString shortcut = action->shortcut().toString(QKeySequence::NativeText);
        const bool match =
            needle.isEmpty()
            || action->text().contains(needle, Qt::CaseInsensitive)
            || shortcut.contains(needle, Qt::CaseInsensitive);

        if (!match) {
            continue;
        }

        QString label = action->text();
        if (!shortcut.isEmpty()) {
            label += QStringLiteral("    ") + shortcut;
        }

        auto* item = new QListWidgetItem(label, results_);
        item->setData(Qt::UserRole, action->objectName());
    }

    if (results_->count() > 0) {
        results_->setCurrentRow(0);
    }
}

void CommandSearch::executeCurrent()
{
    auto* item = results_->currentItem();
    if (item == nullptr || registry_ == nullptr) {
        return;
    }

    const QString objectName = item->data(Qt::UserRole).toString();
    if (!objectName.startsWith(QStringLiteral("command_"))) {
        return;
    }

    const QString id = objectName.mid(QStringLiteral("command_").size());
    hide();
    registry_->trigger(id);
}

}  // namespace freeshape::ui
