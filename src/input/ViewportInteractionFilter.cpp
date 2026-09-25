#include "input/ViewportInteractionFilter.h"

#include <algorithm>
#include <utility>

#include <Gui/Selection/BoxSelection.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventorViewer.h>

#include "ui/SelectionOverlay.h"

#include <Inventor/SbVec2s.h>

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>

namespace freeshape::input {

ViewportInteractionFilter::ViewportInteractionFilter(
    Gui::Document* guiDocument,
    Gui::View3DInventorViewer* viewer,
    QObject* parent
)
    : QObject(parent)
    , guiDocument_(guiDocument)
    , viewer_(viewer)
{}

void ViewportInteractionFilter::setContextMenuHandler(
    std::function<void(const QPoint&)> handler
)
{
    contextMenu_ = std::move(handler);
}

void ViewportInteractionFilter::setCursorPositionHandler(
    std::function<void(const QPoint&)> handler
)
{
    cursorPosition_ = std::move(handler);
}

void ViewportInteractionFilter::setSelectionChangedHandler(
    std::function<void()> handler
)
{
    selectionChanged_ = std::move(handler);
}

void ViewportInteractionFilter::setSketchHandlers(
    std::function<bool(const QPoint&, Qt::MouseButton)> mousePress,
    std::function<void(const QPoint&)> mouseMove
)
{
    sketchMousePress_ = std::move(mousePress);
    sketchMouseMove_ = std::move(mouseMove);
}

void ViewportInteractionFilter::setSelectionOverlay(
    freeshape::ui::SelectionOverlay* overlay
)
{
    selectionOverlay_ = overlay;
}

bool ViewportInteractionFilter::eventFilter(QObject* watched, QEvent* event)
{
    (void)watched;

    if (event->type() == QEvent::MouseMove) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        const QPoint position = mouse->position().toPoint();

        if (cursorPosition_) {
            cursorPosition_(position);
        }

        if (sketchMouseMove_) {
            sketchMouseMove_(position);
        }

        const int threshold = QApplication::startDragDistance();

        if (mouse->buttons().testFlag(Qt::LeftButton)) {
            leftCurrentPosition_ = position;
            if ((position - leftPressPosition_).manhattanLength() > threshold) {
                leftDragged_ = true;
                if (!boxSelecting_) {
                    boxSelecting_ = true;
                    if (selectionOverlay_ != nullptr) {
                        selectionOverlay_->beginBox(leftPressPosition_);
                    }
                }
                if (selectionOverlay_ != nullptr) {
                    selectionOverlay_->updateBox(position);
                }
                return true;
            }
        }

        if (mouse->buttons().testFlag(Qt::RightButton)
            && (position - rightPressPosition_).manhattanLength() > threshold) {
            rightDragged_ = true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouse = static_cast<QMouseEvent*>(event);

        if (sketchMousePress_
            && sketchMousePress_(mouse->position().toPoint(), mouse->button())) {
            return true;
        }

        if (mouse->button() == Qt::LeftButton) {
            selectionBeforeClick_ = snapshotSelection();
            leftPressPosition_ = mouse->position().toPoint();
            leftCurrentPosition_ = leftPressPosition_;
            leftDragged_ = false;
            boxSelecting_ = false;
        }

        if (mouse->button() == Qt::RightButton) {
            rightPressPosition_ = mouse->position().toPoint();
            rightDragged_ = false;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouse = static_cast<QMouseEvent*>(event);

        if (mouse->button() == Qt::LeftButton) {
            if (boxSelecting_ && leftDragged_) {
                leftCurrentPosition_ = mouse->position().toPoint();
                applyDirectionalBoxSelection(
                    mouse->modifiers().testFlag(Qt::ControlModifier)
                );
                if (selectionOverlay_ != nullptr) {
                    selectionOverlay_->endBox();
                }
                boxSelecting_ = false;
                if (selectionChanged_) {
                    selectionChanged_();
                }
                return true;
            }

            if (mouse->modifiers() == Qt::NoModifier && !leftDragged_) {
                QTimer::singleShot(0, this, [this] {
                    reconcileOnshapeToggleSelection();
                    if (selectionChanged_) {
                        selectionChanged_();
                    }
                });
            }
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
            item.SubName != nullptr ? item.SubName : "",
            item.x,
            item.y,
            item.z
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
            item.x,
            item.y,
            item.z,
            nullptr,
            false,
            Gui::SelectionChanges::PickedPoint::Valid
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

void ViewportInteractionFilter::applyDirectionalBoxSelection(bool subtract)
{
    if (viewer_ == nullptr) {
        return;
    }

    auto* gl = viewer_->getGLWidget();
    if (gl == nullptr) {
        return;
    }

    const int h = gl->height();
    std::vector<SbVec2s> polygon;
    polygon.emplace_back(
        static_cast<short>(leftPressPosition_.x()),
        static_cast<short>(h - leftPressPosition_.y())
    );
    polygon.emplace_back(
        static_cast<short>(leftCurrentPosition_.x()),
        static_cast<short>(h - leftCurrentPosition_.y())
    );

    const auto before = selectionBeforeClick_;

    // Current FreeCAD main implements the exact window/crossing distinction we
    // want: left→right uses center/containment semantics; right→left uses
    // intersection semantics. Ask it for subelements, then impose FreeShape's
    // subtract policy if Ctrl was held.
    Gui::applyBoxSelection(viewer_, polygon, true, false);

    if (!subtract) {
        return;
    }

    const auto boxed = snapshotSelection();
    auto desired = before;
    for (const auto& hit : boxed) {
        desired.erase(
            std::remove(desired.begin(), desired.end(), hit),
            desired.end()
        );
    }
    applySelection(desired);
}

}  // namespace freeshape::input
