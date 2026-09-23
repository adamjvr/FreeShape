#include "input/ViewportInteractionFilter.h"

#include <algorithm>
#include <utility>

#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>

namespace freeshape::input {

ViewportInteractionFilter::ViewportInteractionFilter(
    Gui::Document* guiDocument,
    QObject* parent
)
    : QObject(parent)
    , guiDocument_(guiDocument)
{}

void ViewportInteractionFilter::setContextMenuHandler(
    std::function<void(const QPoint&)> handler
)
{
    contextMenu_ = std::move(handler);
}

void ViewportInteractionFilter::setSketchHandlers(
    std::function<bool(const QPoint&, Qt::MouseButton)> mousePress,
    std::function<void(const QPoint&)> mouseMove
)
{
    sketchMousePress_ = std::move(mousePress);
    sketchMouseMove_ = std::move(mouseMove);
}

bool ViewportInteractionFilter::eventFilter(QObject* watched, QEvent* event)
{
    (void)watched;

    if (event->type() == QEvent::MouseMove) {
        auto* mouse = static_cast<QMouseEvent*>(event);

        if (sketchMouseMove_) {
            sketchMouseMove_(mouse->position().toPoint());
        }

        const int threshold = QApplication::startDragDistance();

        if (mouse->buttons().testFlag(Qt::LeftButton)
            && (mouse->position().toPoint() - leftPressPosition_).manhattanLength() > threshold) {
            leftDragged_ = true;
        }

        if (mouse->buttons().testFlag(Qt::RightButton)
            && (mouse->position().toPoint() - rightPressPosition_).manhattanLength() > threshold) {
            rightDragged_ = true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouse = static_cast<QMouseEvent*>(event);

        if (sketchMousePress_
            && sketchMousePress_(mouse->position().toPoint(), mouse->button())) {
            return true;
        }

        if (mouse->button() == Qt::LeftButton
            && mouse->modifiers() == Qt::NoModifier) {
            selectionBeforeClick_ = snapshotSelection();
            leftPressPosition_ = mouse->position().toPoint();
            leftDragged_ = false;
        }

        if (mouse->button() == Qt::RightButton) {
            rightPressPosition_ = mouse->position().toPoint();
            rightDragged_ = false;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouse = static_cast<QMouseEvent*>(event);

        if (mouse->button() == Qt::LeftButton
            && mouse->modifiers() == Qt::NoModifier
            && !leftDragged_) {
            QTimer::singleShot(0, this, [this] {
                reconcileOnshapeToggleSelection();
            });
        }

        if (mouse->button() == Qt::RightButton && !rightDragged_) {
            const QPoint globalPosition = mouse->globalPosition().toPoint();
            QTimer::singleShot(0, this, [this, globalPosition] {
                if (contextMenu_) {
                    contextMenu_(globalPosition);
                }
            });
        }
    }

    return false;
}

std::vector<SelectionKey> ViewportInteractionFilter::snapshotSelection() const
{
    std::vector<SelectionKey> result;

    for (const auto& item : Gui::Selection().getSelection()) {
        if (item.DocName == nullptr || item.FeatName == nullptr) {
            continue;
        }

        result.push_back({
            item.DocName,
            item.FeatName,
            item.SubName != nullptr ? item.SubName : ""
        });
    }

    return result;
}

void ViewportInteractionFilter::applySelection(
    const std::vector<SelectionKey>& selection
)
{
    auto& selected = Gui::Selection();
    selected.clearCompleteSelection(false);

    for (const auto& item : selection) {
        selected.addSelection(
            item.document.c_str(),
            item.object.c_str(),
            item.subElement.empty() ? nullptr : item.subElement.c_str(),
            0.0F,
            0.0F,
            0.0F,
            nullptr,
            false
        );
    }
}

void ViewportInteractionFilter::reconcileOnshapeToggleSelection()
{
    const auto after = snapshotSelection();

    if (after.empty()) {
        return;
    }

    const SelectionKey clicked = after.back();
    auto desired = selectionBeforeClick_;

    const auto existing = std::find(desired.begin(), desired.end(), clicked);
    if (existing != desired.end()) {
        desired.erase(existing);
    }
    else {
        desired.push_back(clicked);
    }

    if (desired != after) {
        applySelection(desired);
    }
}

}  // namespace freeshape::input
