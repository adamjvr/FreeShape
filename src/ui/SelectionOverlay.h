#pragma once

#include <QPoint>
#include <QString>
#include <QWidget>

namespace freeshape::ui {

class SelectionOverlay final : public QWidget
{
public:
    explicit SelectionOverlay(QWidget* parent = nullptr);

    void beginBox(const QPoint& start);
    void updateBox(const QPoint& current);
    void endBox();

    void setSelectionCount(int count);
    void setPreselection(const QString& text, const QPoint& cursorPosition);
    void clearPreselection();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool boxVisible_ = false;
    QPoint boxStart_;
    QPoint boxCurrent_;
    int selectionCount_ = 0;
    QString preselectionText_;
    QPoint cursorPosition_;
};

}  // namespace freeshape::ui
