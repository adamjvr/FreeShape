#pragma once

#include <functional>

#include <QString>
#include <QWidget>

namespace App {
class Document;
}

class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace freeshape::ui {

class PartStudioPanel final : public QWidget
{
public:
    explicit PartStudioPanel(QWidget* parent = nullptr);

    void setPlaneActivatedHandler(std::function<void(const QString&)> handler);
    void setFeatureActivatedHandler(std::function<void(const QString&)> handler);

    void refreshFromDocument(App::Document* document);
    void setSelectedFeature(const QString& objectName);

private:
    void buildUi();
    void rebuildTree();
    void filterTree(const QString& text);
    void handleItemActivated(QTreeWidgetItem* item);

    QLineEdit* filter_ = nullptr;
    QLabel* featuresLabel_ = nullptr;
    QLabel* partsLabel_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QLabel* partLabel_ = nullptr;

    bool hasSketch_ = false;
    bool hasPad_ = false;

    std::function<void(const QString&)> planeActivated_;
    std::function<void(const QString&)> featureActivated_;
};

}  // namespace freeshape::ui
