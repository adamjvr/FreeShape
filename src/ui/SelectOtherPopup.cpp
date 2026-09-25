#include "ui/SelectOtherPopup.h"

#include <algorithm>
#include <utility>

#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPoint>
#include <QCursor>
#include <QVBoxLayout>

namespace freeshape::ui {

SelectOtherPopup::SelectOtherPopup(QWidget* parent)
    : QFrame(parent, Qt::Popup)
{
    setObjectName(QStringLiteral("SelectOtherPopup"));
    setFixedWidth(330);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 7, 8, 7);
    layout->setSpacing(4);

    auto* title = new QLabel(QStringLiteral("Select other"), this);
    title->setObjectName(QStringLiteral("PopupHeading"));
    layout->addWidget(title);

    list_ = new QListWidget(this);
    list_->setMinimumHeight(115);
    list_->setMaximumHeight(245);
    list_->installEventFilter(this);
    layout->addWidget(list_);

    hint_ = new QLabel(
        QStringLiteral("` next   Shift+` previous   Enter accept   Esc close"),
        this
    );
    hint_->setObjectName(QStringLiteral("PopupHint"));
    layout->addWidget(hint_);

    QObject::connect(list_, &QListWidget::itemActivated, this,
                     [this](QListWidgetItem*) {
                         acceptCurrent();
                     });
}

void SelectOtherPopup::setCandidates(
    std::vector<SelectOtherCandidate> candidates
)
{
    candidates_ = std::move(candidates);
    rebuild();
}

void SelectOtherPopup::popupAtGlobal(const QPoint& globalPosition)
{
    if (candidates_.empty()) {
        return;
    }

    adjustSize();
    move(globalPosition);
    show();
    raise();
    activateWindow();
    list_->setFocus();
}

void SelectOtherPopup::cycle(int delta)
{
    if (candidates_.empty()) {
        return;
    }

    if (!isVisible()) {
        popupAtGlobal(QCursor::pos());
    }

    const int count = static_cast<int>(candidates_.size());
    int row = list_->currentRow();
    if (row < 0) {
        row = 0;
    }
    row = (row + delta) % count;
    if (row < 0) {
        row += count;
    }
    list_->setCurrentRow(row);
}

bool SelectOtherPopup::acceptCurrent()
{
    const int row = list_->currentRow();
    if (row < 0 || row >= static_cast<int>(candidates_.size())) {
        return false;
    }

    const auto candidate = candidates_[row];
    hide();
    if (accepted_) {
        accepted_(candidate);
    }
    return true;
}

void SelectOtherPopup::setAcceptedHandler(
    std::function<void(const SelectOtherCandidate&)> handler
)
{
    accepted_ = std::move(handler);
}

bool SelectOtherPopup::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == list_ && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            acceptCurrent();
            return true;
        }
        if (key->key() == Qt::Key_Escape) {
            hide();
            return true;
        }
        if (key->key() == Qt::Key_QuoteLeft) {
            cycle(key->modifiers().testFlag(Qt::ShiftModifier) ? -1 : 1);
            return true;
        }
    }

    return QFrame::eventFilter(watched, event);
}

void SelectOtherPopup::rebuild()
{
    list_->clear();

    for (std::size_t i = 0; i < candidates_.size(); ++i) {
        const auto& candidate = candidates_[i];
        QString label = candidate.object;
        if (!candidate.subElement.isEmpty()) {
            label += QStringLiteral(" · ") + candidate.subElement;
        }
        if (!candidate.typeName.isEmpty()) {
            label += QStringLiteral("    ") + candidate.typeName;
        }
        auto* item = new QListWidgetItem(label, list_);
        item->setToolTip(
            QStringLiteral("%1 / %2 / %3")
                .arg(candidate.document, candidate.object, candidate.subElement)
        );
    }

    if (list_->count() > 0) {
        list_->setCurrentRow(0);
    }
}

}  // namespace freeshape::ui
