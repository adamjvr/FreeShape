#pragma once

#include <functional>
#include <optional>

#include <QPoint>
#include <QPointF>
#include <QString>
#include <Qt>

namespace App {
class Document;
}

namespace Gui {
class Document;
class View3DInventor;
}

class QLineEdit;
class QWidget;

namespace freeshape::sketch {

class SketchOverlay;

class SketchController final
{
public:
    enum class State {
        Idle,
        AwaitPlane,
        Editing
    };

    enum class Tool {
        None,
        Circle,
        Line,
        CornerRectangle
    };

    SketchController(
        App::Document* document,
        Gui::Document* guiDocument,
        Gui::View3DInventor* view,
        QWidget* viewportHost
    );

    ~SketchController();

    void beginSketch();
    bool selectPlane(const QString& planeObjectName);

    bool isAwaitingPlane() const;
    bool isEditing() const;
    bool hasClosedProfile() const;

    void activateCircle();
    void activateLine();
    void activateCornerRectangle();
    void cancelActiveTool();

    bool handleMousePress(const QPoint& viewportPosition, Qt::MouseButton button);
    void handleMouseMove(const QPoint& viewportPosition);

    // accept=true commits the sketch transaction.  false aborts it.
    bool finishSketch(bool accept);

    void setChangedHandler(std::function<void()> handler);
    void setMessageHandler(std::function<void(const QString&)> handler);

private:
    std::optional<QPointF> screenToXY(const QPoint& viewportPosition) const;
    QPointF applyInference(const QPointF& point, bool* snapX, bool* snapY) const;

    bool createSketchOnPlane(const QString& planeObjectName);
    int createCircle(const QPointF& center, double radius);
    int createLine(const QPointF& start, const QPointF& end);
    int createRectangle(const QPointF& first, const QPointF& opposite);
    void applyDiameterConstraint(double diameter);
    void promptDiameter(int geometryIndex, double diameter, const QPoint& screenPosition);

    void emitChanged();
    void emitMessage(const QString& message);
    void syncOverlayGeometry();

    App::Document* document_ = nullptr;
    Gui::Document* guiDocument_ = nullptr;
    Gui::View3DInventor* view_ = nullptr;
    QWidget* viewportHost_ = nullptr;

    SketchOverlay* overlay_ = nullptr;
    QLineEdit* numericInput_ = nullptr;

    State state_ = State::Idle;
    Tool tool_ = Tool::None;
    bool transactionOpen_ = false;

    std::optional<QPointF> circleCenter_;
    std::optional<QPointF> firstPoint_;
    QPoint firstPointScreen_;
    QPoint circleCenterScreen_;
    bool centerSnapX_ = false;
    bool centerSnapY_ = false;

    int geometryCount_ = 0;
    int pendingDiameterGeometry_ = -1;

    std::function<void()> changed_;
    std::function<void(const QString&)> message_;
};

}  // namespace freeshape::sketch
