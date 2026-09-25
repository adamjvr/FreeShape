#pragma once

#include <QFrame>
#include <QString>

class QLabel;

namespace freeshape::ui {

class MeasurementHud final : public QFrame
{
public:
    explicit MeasurementHud(QWidget* parent = nullptr);

    void refresh();
    void clear();

private:
    QString buildMeasurementText() const;

    QLabel* title_ = nullptr;
    QLabel* detail_ = nullptr;
};

}  // namespace freeshape::ui
