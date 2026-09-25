#pragma once

#include <functional>
#include <vector>

#include <QFrame>
#include <QString>

class QLabel;
class QEvent;
class QListWidget;
class QPoint;

namespace freeshape::ui {

struct SelectOtherCandidate
{
    QString document;
    QString object;
    QString subElement;
    QString typeName;
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

class SelectOtherPopup final : public QFrame
{
public:
    explicit SelectOtherPopup(QWidget* parent = nullptr);

    void setCandidates(std::vector<SelectOtherCandidate> candidates);
    void popupAtGlobal(const QPoint& globalPosition);
    void cycle(int delta);
    bool acceptCurrent();

    void setAcceptedHandler(
        std::function<void(const SelectOtherCandidate&)> handler
    );

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void rebuild();

    std::vector<SelectOtherCandidate> candidates_;
    QListWidget* list_ = nullptr;
    QLabel* hint_ = nullptr;
    std::function<void(const SelectOtherCandidate&)> accepted_;
};

}  // namespace freeshape::ui
