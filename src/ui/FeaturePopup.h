#pragma once

#include <functional>

#include <QString>
#include <QFrame>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QStackedWidget;

namespace freeshape::ui {

class FeaturePopup final : public QFrame
{
public:
    enum class Mode {
        Sketch,
        Extrude,
        Fillet
    };

    explicit FeaturePopup(QWidget* parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;

    void setSelectionText(const QString& text);
    void setDepth(double value);
    double depth() const;

    void setAcceptHandler(std::function<void()> handler);
    void setCancelHandler(std::function<void()> handler);
    void setDepthChangedHandler(std::function<void(double)> handler);

private:
    void buildUi();
    void rebuildBody();

    Mode mode_ = Mode::Sketch;
    QLabel* title_ = nullptr;
    QLabel* selectionText_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QWidget* sketchPage_ = nullptr;
    QWidget* extrudePage_ = nullptr;
    QWidget* filletPage_ = nullptr;
    QDoubleSpinBox* depth_ = nullptr;
    QDoubleSpinBox* radius_ = nullptr;

    std::function<void()> accept_;
    std::function<void()> cancel_;
    std::function<void(double)> depthChanged_;
};

}  // namespace freeshape::ui
