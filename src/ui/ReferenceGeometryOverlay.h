#pragma once

#include <QWidget>
#include <QPoint>

namespace Gui {
class View3DInventor;
}

class QLabel;
class QTimer;

namespace freeshape::ui {

class ReferenceGeometryOverlay final : public QWidget
{
public:
    ReferenceGeometryOverlay(
        Gui::View3DInventor* view,
        QWidget* parent = nullptr
    );

    void setReferenceGeometryVisible(bool visible);

private:
    void updateLabels();
    QPoint project(float x, float y, float z) const;

    Gui::View3DInventor* view_ = nullptr;
    QLabel* top_ = nullptr;
    QLabel* front_ = nullptr;
    QLabel* right_ = nullptr;
    QTimer* timer_ = nullptr;
    bool referencesVisible_ = true;
};

}  // namespace freeshape::ui
