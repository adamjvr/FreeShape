#pragma once

#include <QFrame>

class QGridLayout;
class QPoint;

namespace freeshape::commands {
class CommandRegistry;
}

namespace freeshape::ui {

class ShortcutPalette final : public QFrame
{
public:
    enum class Context {
        PartStudio,
        Sketch
    };

    ShortcutPalette(
        freeshape::commands::CommandRegistry* registry,
        QWidget* parent = nullptr
    );

    void setContext(Context context);
    void popupAtGlobal(const QPoint& globalPosition);

private:
    void rebuild();

    freeshape::commands::CommandRegistry* registry_ = nullptr;
    QGridLayout* layout_ = nullptr;
    Context context_ = Context::PartStudio;
};

}  // namespace freeshape::ui
