#pragma once

#include <QPoint>
#include <QWidget>

namespace freeshape::sketch {

class SketchOverlay final : public QWidget
{
public:
    explicit SketchOverlay(QWidget* parent = nullptr);

    void setCirclePreview(
        const QPoint& center,
        const QPoint& edge,
        bool snapX,
        bool snapY
    );
    void clearPreview();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool previewVisible_ = false;
    QPoint center_;
    QPoint edge_;
    bool snapX_ = false;
    bool snapY_ = false;
};

}  // namespace freeshape::sketch
