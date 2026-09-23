#include "ui/DocumentTabs.h"

#include <QHBoxLayout>
#include <QToolButton>

namespace freeshape::ui {

DocumentTabs::DocumentTabs(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("DocumentTabs"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 0, 0);
    layout->setSpacing(0);

    auto* add = new QToolButton(this);
    add->setText(QStringLiteral("＋"));
    add->setToolTip(QStringLiteral("Insert new tab"));
    layout->addWidget(add);

    auto* partStudio = new QToolButton(this);
    partStudio->setObjectName(QStringLiteral("ActiveDocumentTab"));
    partStudio->setText(QStringLiteral("▱  Part Studio 1"));
    layout->addWidget(partStudio);

    auto* assembly = new QToolButton(this);
    assembly->setObjectName(QStringLiteral("InactiveDocumentTab"));
    assembly->setText(QStringLiteral("◇  Assembly 1"));
    assembly->setToolTip(QStringLiteral("Assembly tab foundation"));
    layout->addWidget(assembly);

    layout->addStretch(1);
}

}  // namespace freeshape::ui
