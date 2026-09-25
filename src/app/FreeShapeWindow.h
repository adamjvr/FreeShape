#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QMainWindow>
#include <QPoint>

namespace App {
class Document;
}

namespace Gui {
class Document;
class SelectionObserver;
class View3DInventor;
}

class QLabel;
class QToolBar;
class QWidget;

namespace freeshape::commands {
class CommandRegistry;
}

namespace freeshape::input {
class ViewportInteractionFilter;
}

namespace freeshape::sketch {
class SketchController;
}

namespace freeshape::ui {
struct SelectOtherCandidate;
class DocumentTabs;
class FeaturePopup;
class PartStudioPanel;
class ShortcutPalette;
class CommandSearch;
class SelectionOverlay;
class SelectOtherPopup;
class MeasurementHud;
class ReferenceGeometryOverlay;
}

namespace freeshape::app {

class FreeShapeWindow final : public QMainWindow
{
public:
    explicit FreeShapeWindow(App::Document* document, QWidget* parent = nullptr);
    ~FreeShapeWindow() override;

private:
    void buildShell();
    void buildToolbars();
    void buildCommands();
    void configureViewportBehavior();

    void showSketchCommand();
    void showExtrudeCommand();
    void showFilletCommand();
    void acceptFeatureCommand();
    void cancelFeatureCommand();
    void showShortcutPalette();
    void showCommandSearch();
    void showViewportContextMenu(const QPoint& globalPosition);

    bool beginExtrudePreview();
    void applyExtrudePreview(double depth);
    void finishExtrudePreview(bool accept);

    void togglePlanes();
    void toggleSketches();
    void hideSelected();
    void showLastHidden();
    void cycleSelectOther(int delta = 1);
    void acceptSelectOtherCandidate(const freeshape::ui::SelectOtherCandidate& candidate);
    void normalToSelectionOrPlane();

    void setReferencePlanesVisible(bool visible);
    void styleReferencePlanes();

    void refreshPartStudio();
    void refreshSelectionUi();
    void refreshSelectOtherCandidates();
    void showToast(const QString& text, int milliseconds = 2800);
    void updateToolbarContext(bool sketchMode);

    App::Document* document_ = nullptr;
    Gui::Document* guiDocument_ = nullptr;
    Gui::View3DInventor* view_ = nullptr;

    QWidget* shell_ = nullptr;
    QWidget* viewportHost_ = nullptr;
    QToolBar* documentToolbar_ = nullptr;
    QToolBar* featureToolbar_ = nullptr;
    QToolBar* sketchToolbar_ = nullptr;
    QLabel* toast_ = nullptr;

    freeshape::ui::PartStudioPanel* partStudioPanel_ = nullptr;
    freeshape::ui::DocumentTabs* documentTabs_ = nullptr;
    freeshape::ui::FeaturePopup* featurePopup_ = nullptr;
    freeshape::ui::ShortcutPalette* shortcutPalette_ = nullptr;
    freeshape::ui::CommandSearch* commandSearch_ = nullptr;
    freeshape::ui::SelectionOverlay* selectionOverlay_ = nullptr;
    freeshape::ui::SelectOtherPopup* selectOtherPopup_ = nullptr;
    freeshape::ui::MeasurementHud* measurementHud_ = nullptr;
    freeshape::ui::ReferenceGeometryOverlay* referenceOverlay_ = nullptr;

    std::unique_ptr<freeshape::commands::CommandRegistry> commands_;
    std::unique_ptr<Gui::SelectionObserver> selectionObserver_;
    std::unique_ptr<freeshape::input::ViewportInteractionFilter> viewportInput_;
    std::unique_ptr<freeshape::sketch::SketchController> sketchController_;

    bool planesVisible_ = true;
    bool sketchVisible_ = false;
    bool extrudePreviewActive_ = false;
    bool extrudeCreatedPad_ = false;
    std::vector<std::string> lastHiddenObjects_;
    std::size_t selectOtherIndex_ = 0;
    QPoint lastViewportCursor_;
};

}  // namespace freeshape::app
