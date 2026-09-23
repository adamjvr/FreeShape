#pragma once

#include <functional>
#include <string>
#include <vector>

#include <QObject>
#include <QPoint>
#include <Qt>

namespace Gui {
class Document;
}

namespace freeshape::input {

struct SelectionKey
{
    std::string document;
    std::string object;
    std::string subElement;

    bool operator==(const SelectionKey&) const = default;
};

class ViewportInteractionFilter final : public QObject
{
public:
    explicit ViewportInteractionFilter(
        Gui::Document* guiDocument,
        QObject* parent = nullptr
    );

    void setContextMenuHandler(std::function<void(const QPoint&)> handler);

    // Sketch input gets first refusal on viewport events.  Returning true from
    // mousePress consumes that click so ordinary model selection does not run.
    void setSketchHandlers(
        std::function<bool(const QPoint&, Qt::MouseButton)> mousePress,
        std::function<void(const QPoint&)> mouseMove
    );

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    std::vector<SelectionKey> snapshotSelection() const;
    void applySelection(const std::vector<SelectionKey>& selection);
    void reconcileOnshapeToggleSelection();

    Gui::Document* guiDocument_ = nullptr;
    std::function<void(const QPoint&)> contextMenu_;
    std::function<bool(const QPoint&, Qt::MouseButton)> sketchMousePress_;
    std::function<void(const QPoint&)> sketchMouseMove_;

    std::vector<SelectionKey> selectionBeforeClick_;
    QPoint leftPressPosition_;
    QPoint rightPressPosition_;
    bool leftDragged_ = false;
    bool rightDragged_ = false;
};

}  // namespace freeshape::input
