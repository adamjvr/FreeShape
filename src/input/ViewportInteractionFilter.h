#pragma once

#include <functional>
#include <string>
#include <vector>

#include <QObject>
#include <QPoint>
#include <Qt>

namespace Gui {
class Document;
class View3DInventorViewer;
}

namespace freeshape::ui {
class SelectionOverlay;
}

namespace freeshape::input {

struct SelectionKey
{
    std::string document;
    std::string object;
    std::string subElement;
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;

    bool operator==(const SelectionKey& other) const
    {
        return document == other.document
               && object == other.object
               && subElement == other.subElement;
    }
};

class ViewportInteractionFilter final : public QObject
{
public:
    ViewportInteractionFilter(
        Gui::Document* guiDocument,
        Gui::View3DInventorViewer* viewer,
        QObject* parent = nullptr
    );

    void setContextMenuHandler(std::function<void(const QPoint&)> handler);
    void setCursorPositionHandler(std::function<void(const QPoint&)> handler);
    void setSelectionChangedHandler(std::function<void()> handler);

    void setSketchHandlers(
        std::function<bool(const QPoint&, Qt::MouseButton)> mousePress,
        std::function<void(const QPoint&)> mouseMove
    );

    void setSelectionOverlay(freeshape::ui::SelectionOverlay* overlay);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    std::vector<SelectionKey> snapshotSelection() const;
    void applySelection(const std::vector<SelectionKey>& selection);
    void reconcileOnshapeToggleSelection();
    void applyDirectionalBoxSelection(bool subtract);

    Gui::Document* guiDocument_ = nullptr;
    Gui::View3DInventorViewer* viewer_ = nullptr;
    freeshape::ui::SelectionOverlay* selectionOverlay_ = nullptr;

    std::function<void(const QPoint&)> contextMenu_;
    std::function<void(const QPoint&)> cursorPosition_;
    std::function<void()> selectionChanged_;
    std::function<bool(const QPoint&, Qt::MouseButton)> sketchMousePress_;
    std::function<void(const QPoint&)> sketchMouseMove_;

    std::vector<SelectionKey> selectionBeforeClick_;
    QPoint leftPressPosition_;
    QPoint leftCurrentPosition_;
    QPoint rightPressPosition_;
    bool leftDragged_ = false;
    bool rightDragged_ = false;
    bool boxSelecting_ = false;
};

}  // namespace freeshape::input
