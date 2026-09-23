#include "ui/DesignSystem.h"

namespace freeshape::ui {

QString DesignSystem::applicationStyleSheet()
{
    return QStringLiteral(R"CSS(
        QMainWindow, QWidget {
            background: #f7f7f7;
            color: #30343a;
            font-family: "Noto Sans", "DejaVu Sans", sans-serif;
            font-size: 12px;
        }

        QToolBar {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #d5d8dc;
            spacing: 2px;
            padding: 2px 6px;
        }

        QToolBar#DocumentBar {
            background: #f1f2f3;
            min-height: 34px;
            max-height: 34px;
        }

        QToolBar#FeatureBar, QToolBar#SketchBar {
            min-height: 39px;
            max-height: 39px;
        }

        QToolButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 3px;
            padding: 4px 7px;
            min-height: 22px;
        }

        QToolButton:hover {
            background: #edf4fb;
            border-color: #c8d9e9;
        }

        QToolButton:pressed, QToolButton:checked {
            background: #dcecf9;
            border-color: #8db7dc;
        }

        QLabel#BrandLabel {
            font-size: 17px;
            font-weight: 700;
            padding-left: 2px;
            padding-right: 5px;
        }

        QLabel#WorkspaceLabel {
            color: #6f7479;
            padding-right: 7px;
        }

        QLineEdit {
            background: #ffffff;
            border: 1px solid #c8ccd0;
            border-radius: 3px;
            padding: 4px 7px;
            selection-background-color: #82b6df;
        }

        QLineEdit:focus {
            border-color: #4c9bd4;
        }

        QFrame#PartStudioPanel {
            background: #ffffff;
            border-right: 1px solid #cfd2d5;
        }

        QLabel#PanelHeading {
            font-weight: 700;
            font-size: 12px;
        }

        QTreeWidget {
            background: #ffffff;
            border: none;
            outline: none;
            show-decoration-selected: 1;
        }

        QTreeWidget::item {
            min-height: 24px;
        }

        QTreeWidget::item:selected {
            background: #d9eaf8;
            color: #20252a;
        }

        QFrame#RollbackBar {
            background: #b9bbbd;
            min-height: 6px;
            max-height: 6px;
            border: none;
        }

        QFrame#DocumentTabs {
            background: #efefef;
            border-top: 1px solid #c9cccf;
            min-height: 34px;
            max-height: 34px;
        }

        QToolButton#ActiveDocumentTab {
            background: #ffffff;
            border: none;
            border-top: 3px solid #3c94d1;
            border-radius: 0px;
            padding: 4px 14px 5px 14px;
        }

        QToolButton#InactiveDocumentTab {
            background: #e5e5e5;
            border: none;
            border-right: 1px solid #c7c7c7;
            border-radius: 0px;
            padding: 4px 14px 5px 14px;
        }

        QFrame#FeaturePopup {
            background: #ffffff;
            border: 1px solid #aeb5bb;
            border-radius: 4px;
        }

        QLabel#FeaturePopupTitle {
            font-size: 14px;
            font-weight: 700;
        }

        QToolButton#AcceptFeature {
            color: #179340;
            font-size: 18px;
            font-weight: 700;
        }

        QToolButton#CancelFeature {
            color: #c94343;
            font-size: 17px;
            font-weight: 700;
        }

        QFrame#SelectionField {
            background: #eff7ff;
            border: 1px solid #5ca4d8;
            border-radius: 3px;
        }

        QLabel#SelectionFieldText {
            color: #2d78ad;
            padding: 5px 7px;
        }

        QComboBox, QDoubleSpinBox {
            background: #ffffff;
            border: 1px solid #c7cbd0;
            border-radius: 3px;
            padding: 3px 5px;
            min-height: 23px;
        }

        QFrame#ShortcutPalette {
            background: #ffffff;
            border: 1px solid #9da5ac;
            border-radius: 4px;
        }

        QLabel#ViewportToast {
            background: rgba(43, 49, 55, 220);
            color: #ffffff;
            border-radius: 4px;
            padding: 6px 10px;
        }
    )CSS");
}

}  // namespace freeshape::ui
