#pragma once

#include <QIcon>
#include <QString>

namespace freeshape::ui {

class ToolIconFactory
{
public:
    static QIcon icon(const QString& id);
};

}  // namespace freeshape::ui
