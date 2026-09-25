#include "ui/FeaturePopup.h"

#include <utility>
#include <algorithm>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QSizeGrip>
#include <QMouseEvent>
#include <QToolButton>
#include <QVBoxLayout>

namespace freeshape::ui {

namespace {

QFrame* makeSelectionField(const QString& text, QLabel** outLabel, QWidget* parent)
{
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("SelectionField"));
    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel(text, frame);
    label->setObjectName(QStringLiteral("SelectionFieldText"));
    label->setWordWrap(true);
    layout->addWidget(label, 1);

    if (outLabel != nullptr) {
        *outLabel = label;
    }
    return frame;
}

}  // namespace

FeaturePopup::FeaturePopup(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("FeaturePopup"));
    setMinimumWidth(300);
    setMaximumWidth(520);
    resize(310, 180);
    buildUi();
    setMode(Mode::Sketch);
    hide();
}

void FeaturePopup::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 10);
    root->setSpacing(8);

    auto* header = new QHBoxLayout;
    title_ = new QLabel(QStringLiteral("Sketch"), this);
    title_->setObjectName(QStringLiteral("FeaturePopupTitle"));
    header->addWidget(title_);
    header->addStretch(1);

    auto* acceptButton = new QToolButton(this);
    acceptButton->setObjectName(QStringLiteral("AcceptFeature"));
    acceptButton->setText(QStringLiteral("✓"));
    acceptButton->setToolTip(QStringLiteral("Accept (Enter)"));

    auto* cancelButton = new QToolButton(this);
    cancelButton->setObjectName(QStringLiteral("CancelFeature"));
    cancelButton->setText(QStringLiteral("✕"));
    cancelButton->setToolTip(QStringLiteral("Cancel (Esc)"));

    header->addWidget(acceptButton);
    header->addWidget(cancelButton);
    root->addLayout(header);

    pages_ = new QStackedWidget(this);

    sketchPage_ = new QWidget(pages_);
    {
        auto* layout = new QVBoxLayout(sketchPage_);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(7);
        layout->addWidget(new QLabel(QStringLiteral("Sketch plane"), sketchPage_));
        layout->addWidget(
            makeSelectionField(
                QStringLiteral("Select a sketch plane"),
                &selectionText_,
                sketchPage_
            )
        );
        auto* hint = new QLabel(
            QStringLiteral("Select Top, Front, Right, or a planar face."),
            sketchPage_
        );
        hint->setWordWrap(true);
        hint->setStyleSheet(QStringLiteral("color:#6f7479;"));
        layout->addWidget(hint);
        layout->addStretch(1);
    }

    extrudePage_ = new QWidget(pages_);
    {
        auto* layout = new QVBoxLayout(extrudePage_);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(7);

        auto* typeRow = new QHBoxLayout;
        for (const QString& name : {
                 QStringLiteral("Solid"),
                 QStringLiteral("Surface"),
                 QStringLiteral("Thin")
             }) {
            auto* button = new QToolButton(extrudePage_);
            button->setText(name);
            button->setCheckable(true);
            button->setChecked(name == QStringLiteral("Solid"));
            typeRow->addWidget(button);
        }
        typeRow->addStretch(1);
        layout->addLayout(typeRow);

        auto* operationRow = new QHBoxLayout;
        for (const QString& name : {
                 QStringLiteral("New"),
                 QStringLiteral("Add"),
                 QStringLiteral("Remove"),
                 QStringLiteral("Intersect")
             }) {
            auto* button = new QToolButton(extrudePage_);
            button->setText(name);
            button->setCheckable(true);
            button->setChecked(name == QStringLiteral("New"));
            operationRow->addWidget(button);
        }
        layout->addLayout(operationRow);

        QLabel* extrudeSelection = nullptr;
        layout->addWidget(new QLabel(QStringLiteral("Faces and sketch regions"), extrudePage_));
        layout->addWidget(
            makeSelectionField(
                QStringLiteral("Sketch 1"),
                &extrudeSelection,
                extrudePage_
            )
        );

        auto* form = new QFormLayout;
        auto* endType = new QComboBox(extrudePage_);
        endType->addItems({
            QStringLiteral("Blind"),
            QStringLiteral("Through all"),
            QStringLiteral("Up to next"),
            QStringLiteral("Up to face"),
            QStringLiteral("Up to part"),
            QStringLiteral("Up to vertex")
        });

        depth_ = new QDoubleSpinBox(extrudePage_);
        depth_->setRange(-100000.0, 100000.0);
        depth_->setDecimals(3);
        depth_->setSuffix(QStringLiteral(" mm"));
        depth_->setValue(10.0);

        form->addRow(QStringLiteral("End type"), endType);
        form->addRow(QStringLiteral("Depth"), depth_);
        layout->addLayout(form);

        auto* hint = new QLabel(
            QStringLiteral("Live preview is wired to the bootstrap Pad for this UX spike."),
            extrudePage_
        );
        hint->setWordWrap(true);
        hint->setStyleSheet(QStringLiteral("color:#6f7479;"));
        layout->addWidget(hint);
    }

    filletPage_ = new QWidget(pages_);
    {
        auto* form = new QFormLayout(filletPage_);
        QLabel* fieldText = nullptr;
        form->addRow(
            QStringLiteral("Entities"),
            makeSelectionField(
                QStringLiteral("Select edges or faces"),
                &fieldText,
                filletPage_
            )
        );
        radius_ = new QDoubleSpinBox(filletPage_);
        radius_->setRange(0.001, 100000.0);
        radius_->setDecimals(3);
        radius_->setValue(2.0);
        radius_->setSuffix(QStringLiteral(" mm"));
        form->addRow(QStringLiteral("Radius"), radius_);
    }

    pages_->addWidget(sketchPage_);
    pages_->addWidget(extrudePage_);
    pages_->addWidget(filletPage_);
    root->addWidget(pages_);

    auto* gripRow = new QHBoxLayout;
    gripRow->addStretch(1);
    sizeGrip_ = new QSizeGrip(this);
    gripRow->addWidget(sizeGrip_);
    root->addLayout(gripRow);

    QObject::connect(acceptButton, &QToolButton::clicked, this, [this] {
        if (accept_) {
            accept_();
        }
    });

    QObject::connect(cancelButton, &QToolButton::clicked, this, [this] {
        if (cancel_) {
            cancel_();
        }
    });

    QObject::connect(
        depth_,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        [this](double value) {
            if (depthChanged_) {
                depthChanged_(value);
            }
        }
    );
}

void FeaturePopup::setMode(Mode mode)
{
    mode_ = mode;

    switch (mode_) {
        case Mode::Sketch:
            title_->setText(QStringLiteral("Sketch"));
            pages_->setCurrentWidget(sketchPage_);
            resize(width(), 190);
            break;
        case Mode::Extrude:
            title_->setText(QStringLiteral("Extrude"));
            pages_->setCurrentWidget(extrudePage_);
            resize(width(), 345);
            break;
        case Mode::Fillet:
            title_->setText(QStringLiteral("Fillet"));
            pages_->setCurrentWidget(filletPage_);
            resize(width(), 215);
            break;
    }
}

FeaturePopup::Mode FeaturePopup::mode() const
{
    return mode_;
}

void FeaturePopup::setSelectionText(const QString& text)
{
    if (selectionText_ != nullptr) {
        selectionText_->setText(text);
    }
}

void FeaturePopup::setDepth(double value)
{
    depth_->blockSignals(true);
    depth_->setValue(value);
    depth_->blockSignals(false);
}

double FeaturePopup::depth() const
{
    return depth_->value();
}

void FeaturePopup::setAcceptHandler(std::function<void()> handler)
{
    accept_ = std::move(handler);
}

void FeaturePopup::setCancelHandler(std::function<void()> handler)
{
    cancel_ = std::move(handler);
}

void FeaturePopup::setDepthChangedHandler(std::function<void(double)> handler)
{
    depthChanged_ = std::move(handler);
}


void FeaturePopup::focusPrimaryField()
{
    if (mode_ == Mode::Extrude && depth_ != nullptr) {
        depth_->setFocus();
        depth_->selectAll();
    }
    else if (mode_ == Mode::Fillet && radius_ != nullptr) {
        radius_->setFocus();
        radius_->selectAll();
    }
}

void FeaturePopup::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->position().y() <= 42.0) {
        dragging_ = true;
        dragOffset_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void FeaturePopup::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_ && event->buttons().testFlag(Qt::LeftButton)) {
        QPoint target = event->globalPosition().toPoint() - dragOffset_;
        if (parentWidget() != nullptr) {
            target = parentWidget()->mapFromGlobal(target);
            target.setX(std::clamp(target.x(), 0, std::max(0, parentWidget()->width() - width())));
            target.setY(std::clamp(target.y(), 0, std::max(0, parentWidget()->height() - height())));
        }
        move(target);
        event->accept();
        return;
    }
    QFrame::mouseMoveEvent(event);
}

void FeaturePopup::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
    }
    QFrame::mouseReleaseEvent(event);
}

}  // namespace freeshape::ui
