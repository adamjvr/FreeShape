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
    void setLinePreview(const QPoint& start, const QPoint& end, bool snapX, bool snapY);
    void setRectanglePreview(const QPoint& first, const QPoint& opposite, bool snapX, bool snapY);
    void clearPreview();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    enum class PreviewKind { None, Circle, Line, Rectangle };
    PreviewKind previewKind_ = PreviewKind::None;
    bool previewVisible_ = false;
    QPoint center_;
    QPoint edge_;
    bool snapX_ = false;
    bool snapY_ = false;
};

}  // namespace freeshape::sketch
