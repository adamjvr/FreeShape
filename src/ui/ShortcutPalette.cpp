#include "ui/ShortcutPalette.h"

#include "commands/CommandRegistry.h"

#include <QGridLayout>
#include <QLayoutItem>
#include <QPoint>
#include <QToolButton>
#include <QWidget>

namespace freeshape::ui {

ShortcutPalette::ShortcutPalette(
    freeshape::commands::CommandRegistry* registry,
    QWidget* parent
)
    : QFrame(parent, Qt::Popup)
    , registry_(registry)
{
    setObjectName(QStringLiteral("ShortcutPalette"));

    layout_ = new QGridLayout(this);
    layout_->setContentsMargins(7, 7, 7, 7);
    layout_->setSpacing(4);

    rebuild();
}

void ShortcutPalette::setContext(Context context)
{
    if (context_ == context) {
        return;
    }

    context_ = context;
    rebuild();
}

void ShortcutPalette::rebuild()
{
    while (QLayoutItem* item = layout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    struct Entry {
        const char* command;
        const char* text;
    };

    const Entry partStudioEntries[] = {
        {"sketch", "Sketch\nShift+S"},
        {"extrude", "Extrude\nShift+E"},
        {"fillet", "Fillet\nShift+F"},
        {"fit", "Fit\nF"},
        {"toggle_planes", "Planes\nP"},
        {"clear_selection", "Clear\nSpace"},
    };

    const Entry sketchEntries[] = {
        {"sketch_circle", "Circle\nC"},
        {"extrude", "Extrude\nShift+E"},
        {"fit", "Fit\nF"},
        {"normal_to", "Normal to\nN"},
        {"toggle_sketches", "Sketches\nShift+H"},
        {"clear_selection", "Clear\nSpace"},
    };

    const Entry* entries =
        context_ == Context::Sketch ? sketchEntries : partStudioEntries;
    const int count = 6;

    int row = 0;
    int column = 0;

    for (int i = 0; i < count; ++i) {
        const Entry& entry = entries[i];
        auto* button = new QToolButton(this);
        button->setText(QString::fromUtf8(entry.text));
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setMinimumSize(92, 48);

        QObject::connect(
            button,
            &QToolButton::clicked,
            this,
            [this, command = QString::fromUtf8(entry.command)] {
                hide();
                if (registry_ != nullptr) {
                    registry_->trigger(command);
                }
            }
        );

        layout_->addWidget(button, row, column);

        ++column;
        if (column == 3) {
            column = 0;
            ++row;
        }
    }
}

void ShortcutPalette::popupAtGlobal(const QPoint& globalPosition)
{
    adjustSize();
    move(globalPosition);
    show();
    raise();
    activateWindow();
}

}  // namespace freeshape::ui
