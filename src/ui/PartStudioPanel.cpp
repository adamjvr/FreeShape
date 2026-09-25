#include "ui/PartStudioPanel.h"

#include <utility>

#include <App/Document.h>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace freeshape::ui {

PartStudioPanel::PartStudioPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("PartStudioPanel"));
    setFixedWidth(240);
    buildUi();
    rebuildTree();
}

void PartStudioPanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(7, 6, 7, 4);
    root->setSpacing(4);

    auto* searchRow = new QHBoxLayout;
    auto* filterButton = new QToolButton(this);
    filterButton->setText(QStringLiteral("▽"));
    filterButton->setToolTip(QStringLiteral("Filter features and parts"));

    filter_ = new QLineEdit(this);
    filter_->setPlaceholderText(QStringLiteral("Filter by name or type"));
    searchRow->addWidget(filterButton);
    searchRow->addWidget(filter_, 1);
    root->addLayout(searchRow);

    auto* headingRow = new QHBoxLayout;
    featuresLabel_ = new QLabel(this);
    featuresLabel_->setObjectName(QStringLiteral("PanelHeading"));
    headingRow->addWidget(featuresLabel_);
    headingRow->addStretch(1);

    for (const QString& glyph : {
             QStringLiteral("＋"),
             QStringLiteral("Ⅱ"),
             QStringLiteral("◴")
         }) {
        auto* button = new QToolButton(this);
        button->setText(glyph);
        headingRow->addWidget(button);
    }
    root->addLayout(headingRow);

    splitter_ = new QSplitter(Qt::Vertical, this);
    splitter_->setChildrenCollapsible(false);
    splitter_->setHandleWidth(6);

    auto* featureRegion = new QWidget(splitter_);
    auto* featureLayout = new QVBoxLayout(featureRegion);
    featureLayout->setContentsMargins(0, 0, 0, 0);
    featureLayout->setSpacing(2);

    tree_ = new QTreeWidget(featureRegion);
    tree_->setHeaderHidden(true);
    tree_->setRootIsDecorated(true);
    tree_->setIndentation(14);
    tree_->setUniformRowHeights(true);
    featureLayout->addWidget(tree_, 1);

    auto* rollback = new QFrame(featureRegion);
    rollback->setObjectName(QStringLiteral("RollbackBar"));
    rollback->setToolTip(QStringLiteral("Rollback bar · model history position"));
    featureLayout->addWidget(rollback);

    auto* partsRegion = new QWidget(splitter_);
    auto* partsLayout = new QVBoxLayout(partsRegion);
    partsLayout->setContentsMargins(0, 3, 0, 0);
    partsLayout->setSpacing(2);

    partsLabel_ = new QLabel(partsRegion);
    partsLabel_->setObjectName(QStringLiteral("PanelHeading"));
    partsLayout->addWidget(partsLabel_);

    partLabel_ = new QLabel(partsRegion);
    partLabel_->setMinimumHeight(24);
    partsLayout->addWidget(partLabel_);
    partsLayout->addStretch(1);

    splitter_->addWidget(featureRegion);
    splitter_->addWidget(partsRegion);
    splitter_->setStretchFactor(0, 4);
    splitter_->setStretchFactor(1, 2);
    splitter_->setSizes({410, 180});
    root->addWidget(splitter_, 1);

    QObject::connect(filter_, &QLineEdit::textChanged, this, [this](const QString& text) {
        filterTree(text);
    });

    QObject::connect(tree_, &QTreeWidget::itemClicked, this,
                     [this](QTreeWidgetItem* item, int) {
                         handleItemActivated(item);
                     });
}

void PartStudioPanel::refreshFromDocument(App::Document* document)
{
    hasSketch_ = document != nullptr && document->getObject("Sketch") != nullptr;
    hasPad_ = document != nullptr && document->getObject("Pad") != nullptr;
    rebuildTree();
}

void PartStudioPanel::rebuildTree()
{
    const QString previousFilter = filter_ != nullptr ? filter_->text() : QString();

    tree_->clear();

    const int featureCount = 4 + (hasSketch_ ? 1 : 0) + (hasPad_ ? 1 : 0);
    featuresLabel_->setText(
        QStringLiteral("Features (%1)").arg(featureCount)
    );

    auto* defaultGeometry = new QTreeWidgetItem(tree_);
    defaultGeometry->setText(0, QStringLiteral("Default geometry"));
    defaultGeometry->setExpanded(true);
    defaultGeometry->setData(0, Qt::UserRole, QStringLiteral("group:default"));

    auto* origin = new QTreeWidgetItem(defaultGeometry);
    origin->setText(0, QStringLiteral("◉  Origin"));
    origin->setData(0, Qt::UserRole, QStringLiteral("origin:Origin"));

    auto addPlane = [&](const QString& label, const QString& objectName) {
        auto* item = new QTreeWidgetItem(defaultGeometry);
        item->setText(0, label);
        item->setData(0, Qt::UserRole, QStringLiteral("plane:") + objectName);
    };

    addPlane(QStringLiteral("▱  Top"), QStringLiteral("XY_Plane"));
    addPlane(QStringLiteral("▱  Front"), QStringLiteral("XZ_Plane"));
    addPlane(QStringLiteral("▱  Right"), QStringLiteral("YZ_Plane"));

    if (hasSketch_) {
        auto* sketch = new QTreeWidgetItem(tree_);
        sketch->setText(0, QStringLiteral("⌁  Sketch 1"));
        sketch->setData(0, Qt::UserRole, QStringLiteral("feature:Sketch"));
    }

    if (hasPad_) {
        auto* pad = new QTreeWidgetItem(tree_);
        pad->setText(0, QStringLiteral("▰  Extrude 1"));
        pad->setData(0, Qt::UserRole, QStringLiteral("feature:Pad"));
    }

    partsLabel_->setText(
        hasPad_ ? QStringLiteral("⌄  Parts (1)")
                : QStringLiteral("⌄  Parts (0)")
    );
    partLabel_->setText(hasPad_ ? QStringLiteral("    Part 1") : QString());
    partLabel_->setVisible(hasPad_);

    filterTree(previousFilter);
}

void PartStudioPanel::setPlaneActivatedHandler(
    std::function<void(const QString&)> handler
)
{
    planeActivated_ = std::move(handler);
}

void PartStudioPanel::setFeatureActivatedHandler(
    std::function<void(const QString&)> handler
)
{
    featureActivated_ = std::move(handler);
}

void PartStudioPanel::filterTree(const QString& text)
{
    const QString needle = text.trimmed();

    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        auto* item = tree_->topLevelItem(i);
        bool visible = needle.isEmpty() || item->text(0).contains(needle, Qt::CaseInsensitive);

        for (int j = 0; j < item->childCount(); ++j) {
            auto* child = item->child(j);
            const bool childVisible =
                needle.isEmpty() || child->text(0).contains(needle, Qt::CaseInsensitive);
            child->setHidden(!childVisible);
            visible = visible || childVisible;
        }
        item->setHidden(!visible);
    }
}

void PartStudioPanel::handleItemActivated(QTreeWidgetItem* item)
{
    if (item == nullptr) {
        return;
    }

    const QString key = item->data(0, Qt::UserRole).toString();
    if (key.startsWith(QStringLiteral("plane:"))) {
        if (planeActivated_) {
            planeActivated_(key.mid(6));
        }
        return;
    }

    if (key.startsWith(QStringLiteral("feature:"))) {
        if (featureActivated_) {
            featureActivated_(key.mid(8));
        }
    }
}

void PartStudioPanel::setSelectedFeature(const QString& objectName)
{
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        auto* item = tree_->topLevelItem(i);
        const QString key = item->data(0, Qt::UserRole).toString();
        if (key == QStringLiteral("feature:") + objectName) {
            tree_->setCurrentItem(item);
            return;
        }
    }
}

}  // namespace freeshape::ui
