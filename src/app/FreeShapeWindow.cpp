#include "app/FreeShapeWindow.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderPlane.h>

#include "commands/CommandRegistry.h"
#include "input/ViewportInteractionFilter.h"
#include "sketch/SketchController.h"
#include "ui/CommandSearch.h"
#include "ui/DesignSystem.h"
#include "ui/DocumentTabs.h"
#include "ui/FeaturePopup.h"
#include "ui/PartStudioPanel.h"
#include "ui/ShortcutPalette.h"
#include "ui/SelectionOverlay.h"
#include "ui/SelectOtherPopup.h"
#include "ui/MeasurementHud.h"
#include "ui/ReferenceGeometryOverlay.h"
#include "ui/ToolIconFactory.h"
#include "ui/ToolNameBubble.h"

#include <QAction>
#include <QComboBox>
#include <QCursor>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSizePolicy>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace freeshape::app {

namespace {

class SelectionProbe final : public Gui::SelectionObserver
{
public:
    using Callback = std::function<void(const Gui::SelectionChanges&)>;

    explicit SelectionProbe(Callback callback)
        : Gui::SelectionObserver(true, Gui::ResolveMode::NoResolve)
        , callback_(std::move(callback))
    {}

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            Base::Console().message(
                "FREESHAPE_SELECTION doc={} object={} sub={} type={} picked={} "
                "xyz=({:.6f},{:.6f},{:.6f})\n",
                msg.pDocName != nullptr ? msg.pDocName : "",
                msg.pObjectName != nullptr ? msg.pObjectName : "",
                msg.pSubName != nullptr ? msg.pSubName : "",
                msg.pTypeName != nullptr ? msg.pTypeName : "",
                msg.hasPickedPoint ? "true" : "false",
                msg.x,
                msg.y,
                msg.z
            );
        }

        if (callback_) {
            callback_(msg);
        }
    }

    Callback callback_;
};

QWidget* stretchWidget(QWidget* parent)
{
    auto* stretch = new QWidget(parent);
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return stretch;
}

QString datumPlaneFromSelection(
    const QString& object,
    const QString& subElement
)
{
    const auto match = [&](const QString& internal) {
        return object == internal
            || subElement == internal
            || subElement.contains(QStringLiteral(".") + internal)
            || subElement.startsWith(internal + QStringLiteral("."));
    };

    if (match(QStringLiteral("XY_Plane"))) {
        return QStringLiteral("XY_Plane");
    }
    if (match(QStringLiteral("XZ_Plane"))) {
        return QStringLiteral("XZ_Plane");
    }
    if (match(QStringLiteral("YZ_Plane"))) {
        return QStringLiteral("YZ_Plane");
    }
    return {};
}

QString semanticPlaneName(const QString& internal)
{
    if (internal == QStringLiteral("XY_Plane")) {
        return QStringLiteral("Top");
    }
    if (internal == QStringLiteral("XZ_Plane")) {
        return QStringLiteral("Front");
    }
    if (internal == QStringLiteral("YZ_Plane")) {
        return QStringLiteral("Right");
    }
    return internal;
}

QString semanticSelectionText(const QString& object, const QString& subElement)
{
    const QString plane = datumPlaneFromSelection(object, subElement);
    if (!plane.isEmpty()) {
        return semanticPlaneName(plane);
    }

    if (subElement.isEmpty()) {
        return object;
    }
    return object + QStringLiteral(" · ") + subElement;
}

}  // namespace

FreeShapeWindow::FreeShapeWindow(App::Document* document, QWidget* parent)
    : QMainWindow(parent)
    , document_(document)
{
    if (document_ == nullptr) {
        throw std::invalid_argument("FreeShapeWindow requires an App::Document");
    }

    guiDocument_ = Gui::Application::Instance->getDocument(document_);
    if (guiDocument_ == nullptr) {
        throw std::runtime_error("FreeCAD GUI document was not created after reload");
    }

    setObjectName(QStringLiteral("FreeShapeMainWindow"));
    setWindowTitle(QStringLiteral("FreeShape"));
    resize(1500, 930);
    setMinimumSize(1000, 650);
    setStyleSheet(freeshape::ui::DesignSystem::applicationStyleSheet());
    statusBar()->hide();

    configureViewportBehavior();

    auto* mdiView = guiDocument_->createView(
        Gui::View3DInventor::getClassTypeId(),
        Gui::CreateViewMode::Clone
    );

    view_ = dynamic_cast<Gui::View3DInventor*>(mdiView);
    if (view_ == nullptr) {
        throw std::runtime_error("FreeCAD failed to create a View3DInventor");
    }

    view_->getViewer()->setNavigationType(
        Gui::TinkerCADNavigationStyle::getClassTypeId()
    );

    buildCommands();
    buildToolbars();
    buildShell();

    sketchController_ =
        std::make_unique<freeshape::sketch::SketchController>(
            document_,
            guiDocument_,
            view_,
            viewportHost_
        );

    sketchController_->setMessageHandler([this](const QString& message) {
        showToast(message, 3200);
    });

    sketchController_->setChangedHandler([this] {
        refreshPartStudio();
        updateToolbarContext(sketchController_ != nullptr
                             && sketchController_->isEditing());
        syncActiveToolButtons();
    });

    refreshPartStudio();

    // FreeCAD origin planes are children of the hidden App::Origin view
    // provider. Showing XY/XZ/YZ alone is insufficient because the parent's
    // visibility switch still clips them from the Coin scene.
    setReferencePlanesVisible(true);

    selectionObserver_ = std::make_unique<SelectionProbe>(
        [this](const Gui::SelectionChanges& msg) {
            if (msg.Type == Gui::SelectionChanges::SetPreselect
                || msg.Type == Gui::SelectionChanges::MovePreselect) {
                lastPreselectionObject_ = QString::fromUtf8(
                    msg.pObjectName != nullptr ? msg.pObjectName : ""
                );
                lastPreselectionSubElement_ = QString::fromUtf8(
                    msg.pSubName != nullptr ? msg.pSubName : ""
                );

                if (selectionOverlay_ != nullptr) {
                    selectionOverlay_->setPreselection(
                        semanticSelectionText(
                            lastPreselectionObject_,
                            lastPreselectionSubElement_
                        ),
                        lastViewportCursor_
                    );
                }
                return;
            }

            if (msg.Type == Gui::SelectionChanges::RmvPreselect
                || msg.Type == Gui::SelectionChanges::RmvPreselectSignal) {
                lastPreselectionObject_.clear();
                lastPreselectionSubElement_.clear();
                if (selectionOverlay_ != nullptr) {
                    selectionOverlay_->clearPreselection();
                }
                return;
            }

            if (msg.Type == Gui::SelectionChanges::ClrSelection) {
                refreshSelectionUi();
                showToast(QStringLiteral("Selection cleared"));
                return;
            }

            if (msg.Type != Gui::SelectionChanges::AddSelection
                && msg.Type != Gui::SelectionChanges::RmvSelection) {
                return;
            }

            const QString object =
                QString::fromUtf8(msg.pObjectName != nullptr ? msg.pObjectName : "");
            const QString sub =
                QString::fromUtf8(msg.pSubName != nullptr ? msg.pSubName : "");

            const QString selectedPlane = datumPlaneFromSelection(object, sub);
            if (sketchController_ != nullptr
                && sketchController_->isAwaitingPlane()
                && !selectedPlane.isEmpty()) {
                if (sketchController_->selectPlane(selectedPlane)) {
                    featurePopup_->setSelectionText(
                        semanticPlaneName(selectedPlane)
                    );
                    setReferencePlanesVisible(false);
                    updateToolbarContext(true);
                    refreshPartStudio();
                }
            }

            if (partStudioPanel_ != nullptr) {
                partStudioPanel_->setSelectedFeature(object);
            }

            showToast(semanticSelectionText(object, sub));
            refreshSelectionUi();
        }
    );

    Gui::Selection().enablePickedList(true);

    if (auto* glWidget = view_->getViewer()->getGLWidget(); glWidget != nullptr) {
        viewportInput_ =
            std::make_unique<freeshape::input::ViewportInteractionFilter>(
                guiDocument_,
                view_->getViewer(),
                glWidget
            );

        viewportInput_->setContextMenuHandler(
            [this](const QPoint& globalPosition) {
                showViewportContextMenu(globalPosition);
            }
        );

        viewportInput_->setSketchHandlers(
            [this](const QPoint& position, Qt::MouseButton button) {
                return sketchController_ != nullptr
                       && sketchController_->handleMousePress(position, button);
            },
            [this](const QPoint& position) {
                if (sketchController_ != nullptr) {
                    sketchController_->handleMouseMove(position);
                }
            }
        );

        viewportInput_->setCursorPositionHandler([this](const QPoint& position) {
            lastViewportCursor_ = position;
        });
        viewportInput_->setSelectionChangedHandler([this] {
            refreshSelectionUi();
        });
        viewportInput_->setSelectionOverlay(selectionOverlay_);

        glWidget->installEventFilter(viewportInput_.get());
    }

    QTimer::singleShot(0, this, [this] {
        if (view_ != nullptr) {
            view_->viewAll();
            view_->update();
        }
        showToast(
            QStringLiteral(
                "Onshape interaction profile · RMB orbit · MMB pan · wheel zoom · S shortcuts"
            ),
            4200
        );
    });
}

FreeShapeWindow::~FreeShapeWindow()
{
    if (extrudePreviewActive_) {
        finishExtrudePreview(false);
    }
}

void FreeShapeWindow::configureViewportBehavior()
{
    auto prefs = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    prefs->SetASCII("NavigationStyle", "Gui::TinkerCADNavigationStyle");
    prefs->SetBool("SameStyleForAllViews", true);
}

void FreeShapeWindow::buildCommands()
{
    commands_ = std::make_unique<freeshape::commands::CommandRegistry>(this);

    commands_->add(
        QStringLiteral("shortcut_palette"),
        QStringLiteral("Shortcut toolbar"),
        QKeySequence(QStringLiteral("S")),
        [this] { showShortcutPalette(); }
    );

    commands_->add(
        QStringLiteral("search_tools"),
        QStringLiteral("Search tools"),
        QKeySequence(QStringLiteral("Alt+C")),
        [this] { showCommandSearch(); }
    );

    commands_->add(
        QStringLiteral("sketch"),
        QStringLiteral("Sketch"),
        QKeySequence(QStringLiteral("Shift+S")),
        [this] { showSketchCommand(); }
    );

    commands_->add(
        QStringLiteral("extrude"),
        QStringLiteral("Extrude"),
        QKeySequence(QStringLiteral("Shift+E")),
        [this] { showExtrudeCommand(); }
    );

    commands_->add(
        QStringLiteral("fillet"),
        QStringLiteral("Fillet"),
        QKeySequence(QStringLiteral("Shift+F")),
        [this] { showFilletCommand(); }
    );

    commands_->add(
        QStringLiteral("sketch_circle"),
        QStringLiteral("Center point circle"),
        QKeySequence(QStringLiteral("C")),
        [this] {
            if (sketchController_ != nullptr && sketchController_->isEditing()) {
                sketchController_->activateCircle();
            }
            else {
                showToast(QStringLiteral("Circle requires an active sketch"));
            }
        }
    );

    commands_->add(
        QStringLiteral("sketch_line"),
        QStringLiteral("Line"),
        QKeySequence(QStringLiteral("L")),
        [this] {
            if (sketchController_ != nullptr && sketchController_->isEditing()) {
                sketchController_->activateLine();
            }
        }
    );

    commands_->add(
        QStringLiteral("sketch_rectangle"),
        QStringLiteral("Corner rectangle"),
        QKeySequence(QStringLiteral("G")),
        [this] {
            if (sketchController_ != nullptr && sketchController_->isEditing()) {
                sketchController_->activateCornerRectangle();
            }
        }
    );

    commands_->add(
        QStringLiteral("sketch_center_rectangle"),
        QStringLiteral("Center point rectangle"),
        QKeySequence(QStringLiteral("R")),
        [this] {
            if (sketchController_ != nullptr && sketchController_->isEditing()) {
                sketchController_->activateCenterRectangle();
            }
        }
    );

    commands_->add(
        QStringLiteral("fit"),
        QStringLiteral("Fit"),
        QKeySequence(QStringLiteral("F")),
        [this] {
            if (view_ != nullptr) {
                view_->viewAll();
            }
        }
    );

    commands_->add(
        QStringLiteral("zoom_out"),
        QStringLiteral("Zoom out"),
        QKeySequence(QStringLiteral("Z")),
        [this] {
            if (guiDocument_ != nullptr) {
                guiDocument_->sendMsgToViews("ZoomOut");
            }
        }
    );

    commands_->add(
        QStringLiteral("zoom_in"),
        QStringLiteral("Zoom in"),
        QKeySequence(QStringLiteral("Shift+Z")),
        [this] {
            if (guiDocument_ != nullptr) {
                guiDocument_->sendMsgToViews("ZoomIn");
            }
        }
    );

    commands_->add(
        QStringLiteral("clear_selection"),
        QStringLiteral("Clear selection"),
        QKeySequence(Qt::Key_Space),
        [] {
            Gui::Selection().clearCompleteSelection();
        }
    );

    commands_->add(
        QStringLiteral("toggle_planes"),
        QStringLiteral("Planes"),
        QKeySequence(QStringLiteral("P")),
        [this] { togglePlanes(); }
    );

    commands_->add(
        QStringLiteral("toggle_sketches"),
        QStringLiteral("Sketches"),
        QKeySequence(QStringLiteral("Shift+H")),
        [this] { toggleSketches(); }
    );

    commands_->add(
        QStringLiteral("hide"),
        QStringLiteral("Hide"),
        QKeySequence(QStringLiteral("Y")),
        [this] { hideSelected(); }
    );

    commands_->add(
        QStringLiteral("show_hidden"),
        QStringLiteral("Show hidden"),
        QKeySequence(QStringLiteral("Shift+Y")),
        [this] { showLastHidden(); }
    );

    commands_->add(
        QStringLiteral("select_other"),
        QStringLiteral("Select other"),
        QKeySequence(QStringLiteral("`")),
        [this] { cycleSelectOther(1); }
    );

    commands_->add(
        QStringLiteral("select_other_previous"),
        QStringLiteral("Select other previous"),
        QKeySequence(QStringLiteral("Shift+`")),
        [this] { cycleSelectOther(-1); }
    );

    commands_->add(
        QStringLiteral("normal_to"),
        QStringLiteral("Normal to"),
        QKeySequence(QStringLiteral("N")),
        [this] { normalToSelectionOrPlane(); }
    );

    struct ViewShortcut {
        const char* id;
        const char* name;
        const char* shortcut;
        const char* message;
    };

    const ViewShortcut views[] = {
        {"view_front", "Front", "Shift+1", "ViewFront"},
        {"view_rear", "Rear", "Shift+2", "ViewRear"},
        {"view_left", "Left", "Shift+3", "ViewLeft"},
        {"view_right", "Right", "Shift+4", "ViewRight"},
        {"view_top", "Top", "Shift+5", "ViewTop"},
        {"view_bottom", "Bottom", "Shift+6", "ViewBottom"},
        {"view_iso", "Isometric", "Shift+7", "ViewAxonometric"},
    };

    for (const auto& item : views) {
        commands_->add(
            QString::fromUtf8(item.id),
            QString::fromUtf8(item.name),
            QKeySequence(QString::fromUtf8(item.shortcut)),
            [this, message = std::string(item.message)] {
                if (guiDocument_ != nullptr) {
                    guiDocument_->sendMsgToViews(message.c_str());
                }
            }
        );
    }

    commands_->add(
        QStringLiteral("accept_feature"),
        QStringLiteral("Accept feature"),
        QKeySequence(Qt::Key_Return),
        [this] {
            if (featurePopup_ != nullptr && featurePopup_->isVisible()) {
                acceptFeatureCommand();
            }
        }
    );

    commands_->add(
        QStringLiteral("cancel_feature"),
        QStringLiteral("Cancel / exit active tool"),
        QKeySequence(Qt::Key_Escape),
        [this] {
            // Onshape-style layering: Escape exits the active sketch tool
            // first. A subsequent Escape can cancel the containing command.
            if (sketchController_ != nullptr
                && sketchController_->isEditing()
                && sketchController_->activeTool()
                    != freeshape::sketch::SketchController::Tool::None) {
                sketchController_->cancelActiveTool();
                syncActiveToolButtons();
                return;
            }

            if (selectOtherPopup_ != nullptr && selectOtherPopup_->isVisible()) {
                selectOtherPopup_->hide();
                return;
            }
            if (shortcutPalette_ != nullptr && shortcutPalette_->isVisible()) {
                shortcutPalette_->hide();
                return;
            }
            if (commandSearch_ != nullptr && commandSearch_->isVisible()) {
                commandSearch_->hide();
                return;
            }
            if (featurePopup_ != nullptr && featurePopup_->isVisible()) {
                cancelFeatureCommand();
            }
        }
    );
}

void FreeShapeWindow::buildToolbars()
{
    toolNameBubble_ = new freeshape::ui::ToolNameBubble(this);

    documentToolbar_ = new QToolBar(this);
    documentToolbar_->setObjectName(QStringLiteral("DocumentBar"));
    documentToolbar_->setMovable(false);
    documentToolbar_->setFloatable(false);

    auto* menu = new QToolButton(documentToolbar_);
    menu->setText(QStringLiteral("☰"));
    menu->setFixedSize(30, 28);
    toolNameBubble_->watch(
        menu,
        QStringLiteral("Document menu"),
        {},
        QStringLiteral("Document and workspace commands")
    );
    documentToolbar_->addWidget(menu);

    auto* brand = new QLabel(QStringLiteral("FreeShape"), documentToolbar_);
    brand->setObjectName(QStringLiteral("BrandLabel"));
    documentToolbar_->addWidget(brand);

    auto* workspace = new QLabel(QStringLiteral("Main"), documentToolbar_);
    workspace->setObjectName(QStringLiteral("WorkspaceLabel"));
    documentToolbar_->addWidget(workspace);

    auto* link = new QToolButton(documentToolbar_);
    link->setText(QStringLiteral("↗"));
    link->setFixedSize(30, 28);
    toolNameBubble_->watch(link, QStringLiteral("Document link"));
    documentToolbar_->addWidget(link);

    documentToolbar_->addWidget(stretchWidget(documentToolbar_));

    auto* search = new QLineEdit(documentToolbar_);
    search->setPlaceholderText(QStringLiteral("Search tools…  Alt+C"));
    search->setReadOnly(true);
    search->setCursor(Qt::PointingHandCursor);
    search->setFixedWidth(170);
    documentToolbar_->addWidget(search);

    auto* help = new QToolButton(documentToolbar_);
    help->setText(QStringLiteral("?"));
    help->setFixedSize(30, 28);
    toolNameBubble_->watch(
        help,
        QStringLiteral("Help"),
        QStringLiteral("Shift+/"),
        QStringLiteral("Keyboard shortcuts and FreeShape documentation")
    );
    documentToolbar_->addWidget(help);

    addToolBar(Qt::TopToolBarArea, documentToolbar_);

    auto makeIconButton = [this](
        QToolBar* toolbar,
        const QString& iconId,
        const QString& title,
        const QString& shortcut,
        const QString& detail,
        std::function<void()> callback,
        bool checkable = false,
        const QString& objectName = {}
    ) {
        auto* button = new QToolButton(toolbar);
        button->setIcon(freeshape::ui::ToolIconFactory::icon(iconId));
        button->setIconSize(QSize(25, 25));
        button->setFixedSize(40, 36);
        button->setAutoRaise(true);
        button->setCheckable(checkable);
        if (!objectName.isEmpty()) {
            button->setObjectName(objectName);
        }

        toolNameBubble_->watch(button, title, shortcut, detail);

        QObject::connect(button, &QToolButton::clicked, this, [callback = std::move(callback)] {
            callback();
        });

        toolbar->addWidget(button);
        return button;
    };

    auto addCommandIcon = [&](QToolBar* toolbar,
                              const QString& iconId,
                              const QString& commandId,
                              const QString& detail = {},
                              bool checkable = false) {
        QAction* action = commands_->action(commandId);
        const QString title = action != nullptr ? action->text() : commandId;
        const QString shortcut = action != nullptr
            ? action->shortcut().toString(QKeySequence::NativeText)
            : QString();
        return makeIconButton(
            toolbar,
            iconId,
            title,
            shortcut,
            detail,
            [this, commandId] { commands_->trigger(commandId); },
            checkable,
            QStringLiteral("tool_") + commandId
        );
    };

    auto addPassiveIcon = [&](QToolBar* toolbar,
                              const QString& iconId,
                              const QString& title,
                              const QString& detail = {}) {
        return makeIconButton(
            toolbar,
            iconId,
            title,
            {},
            detail,
            [this, title] {
                showToast(title + QStringLiteral(" · implementation is on the active roadmap"));
            }
        );
    };

    // ---------------------------------------------------------------------
    // Part Studio feature toolbar
    // ---------------------------------------------------------------------
    featureToolbar_ = new QToolBar(this);
    featureToolbar_->setObjectName(QStringLiteral("FeatureBar"));
    featureToolbar_->setMovable(false);
    featureToolbar_->setFloatable(false);

    addCommandIcon(
        featureToolbar_,
        QStringLiteral("sketch"),
        QStringLiteral("sketch"),
        QStringLiteral("Create or edit a sketch")
    );
    featureToolbar_->addSeparator();

    addCommandIcon(
        featureToolbar_,
        QStringLiteral("extrude"),
        QStringLiteral("extrude"),
        QStringLiteral("Create, add, remove, or intersect material")
    );
    addPassiveIcon(featureToolbar_, QStringLiteral("revolve"), QStringLiteral("Revolve"));
    addPassiveIcon(featureToolbar_, QStringLiteral("sweep"), QStringLiteral("Sweep"));
    addPassiveIcon(featureToolbar_, QStringLiteral("loft"), QStringLiteral("Loft"));

    featureToolbar_->addSeparator();

    addPassiveIcon(featureToolbar_, QStringLiteral("hole"), QStringLiteral("Hole"));
    addCommandIcon(
        featureToolbar_,
        QStringLiteral("fillet"),
        QStringLiteral("fillet"),
        QStringLiteral("Round selected edges")
    );
    addPassiveIcon(featureToolbar_, QStringLiteral("chamfer"), QStringLiteral("Chamfer"));
    addPassiveIcon(featureToolbar_, QStringLiteral("shell"), QStringLiteral("Shell"));
    addPassiveIcon(featureToolbar_, QStringLiteral("draft"), QStringLiteral("Draft"));

    featureToolbar_->addSeparator();

    addPassiveIcon(featureToolbar_, QStringLiteral("pattern"), QStringLiteral("Pattern"));
    addPassiveIcon(featureToolbar_, QStringLiteral("mirror"), QStringLiteral("Mirror"));
    addPassiveIcon(featureToolbar_, QStringLiteral("boolean"), QStringLiteral("Boolean"));
    addPassiveIcon(featureToolbar_, QStringLiteral("transform"), QStringLiteral("Transform"));

    addToolBar(Qt::TopToolBarArea, featureToolbar_);

    // ---------------------------------------------------------------------
    // Sketch toolbar. Onshape keeps Extrude/Revolve directly available while
    // sketching, then switches the rest of the toolbar to sketch tools.
    // ---------------------------------------------------------------------
    sketchToolbar_ = new QToolBar(this);
    sketchToolbar_->setObjectName(QStringLiteral("SketchBar"));
    sketchToolbar_->setMovable(false);
    sketchToolbar_->setFloatable(false);

    addCommandIcon(
        sketchToolbar_,
        QStringLiteral("extrude"),
        QStringLiteral("extrude"),
        QStringLiteral("Accept the current sketch and extrude its closed regions")
    );
    addPassiveIcon(
        sketchToolbar_,
        QStringLiteral("revolve"),
        QStringLiteral("Revolve"),
        QStringLiteral("Accept the current sketch and revolve a region")
    );

    sketchToolbar_->addSeparator();

    addCommandIcon(
        sketchToolbar_,
        QStringLiteral("line"),
        QStringLiteral("sketch_line"),
        QStringLiteral("Click endpoints to create connected line segments"),
        true
    );
    addCommandIcon(
        sketchToolbar_,
        QStringLiteral("circle"),
        QStringLiteral("sketch_circle"),
        QStringLiteral("Click center, then radius"),
        true
    );
    addCommandIcon(
        sketchToolbar_,
        QStringLiteral("rectangle"),
        QStringLiteral("sketch_rectangle"),
        QStringLiteral("Click opposite corners"),
        true
    );
    addCommandIcon(
        sketchToolbar_,
        QStringLiteral("center_rectangle"),
        QStringLiteral("sketch_center_rectangle"),
        QStringLiteral("Click center, then a corner"),
        true
    );

    sketchToolbar_->addSeparator();

    addPassiveIcon(sketchToolbar_, QStringLiteral("arc"), QStringLiteral("3-point arc"),
                   QStringLiteral("Shortcut A"));
    addPassiveIcon(sketchToolbar_, QStringLiteral("spline"), QStringLiteral("Spline"));

    sketchToolbar_->addSeparator();

    addPassiveIcon(sketchToolbar_, QStringLiteral("dimension"), QStringLiteral("Dimension"),
                   QStringLiteral("Shortcut D"));
    addPassiveIcon(sketchToolbar_, QStringLiteral("construction"), QStringLiteral("Construction"),
                   QStringLiteral("Shortcut Q"));
    addPassiveIcon(sketchToolbar_, QStringLiteral("trim"), QStringLiteral("Trim"),
                   QStringLiteral("Shortcut M"));
    addPassiveIcon(sketchToolbar_, QStringLiteral("use"), QStringLiteral("Use / project"),
                   QStringLiteral("Shortcut U"));
    addPassiveIcon(sketchToolbar_, QStringLiteral("offset"), QStringLiteral("Offset"),
                   QStringLiteral("Shortcut O"));
    addPassiveIcon(sketchToolbar_, QStringLiteral("constraints"), QStringLiteral("Constraints"));

    addToolBar(Qt::TopToolBarArea, sketchToolbar_);
    sketchToolbar_->hide();
}

void FreeShapeWindow::buildShell()
{
    shell_ = new QWidget(this);
    auto* root = new QVBoxLayout(shell_);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* workspace = new QWidget(shell_);
    auto* workspaceLayout = new QHBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    partStudioPanel_ = new freeshape::ui::PartStudioPanel(workspace);
    workspaceLayout->addWidget(partStudioPanel_);

    viewportHost_ = new QWidget(workspace);
    viewportHost_->setMinimumSize(640, 480);
    viewportHost_->setStyleSheet(QStringLiteral("background:#ffffff;"));

    auto* viewportLayout = new QVBoxLayout(viewportHost_);
    viewportLayout->setContentsMargins(0, 0, 0, 0);
    viewportLayout->setSpacing(0);

    view_->setParent(viewportHost_);
    view_->setWindowFlags(Qt::Widget);
    view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    viewportLayout->addWidget(view_, 1);
    view_->show();

    featurePopup_ = new freeshape::ui::FeaturePopup(viewportHost_);
    featurePopup_->move(14, 14);
    featurePopup_->raise();

    featurePopup_->setAcceptHandler([this] {
        acceptFeatureCommand();
    });
    featurePopup_->setCancelHandler([this] {
        cancelFeatureCommand();
    });
    featurePopup_->setDepthChangedHandler([this](double depth) {
        applyExtrudePreview(depth);
    });

    shortcutPalette_ =
        new freeshape::ui::ShortcutPalette(commands_.get(), viewportHost_);

    commandSearch_ =
        new freeshape::ui::CommandSearch(commands_.get(), viewportHost_);

    selectionOverlay_ = new freeshape::ui::SelectionOverlay(viewportHost_);
    selectionOverlay_->setGeometry(viewportHost_->rect());

    selectOtherPopup_ = new freeshape::ui::SelectOtherPopup(viewportHost_);
    selectOtherPopup_->setAcceptedHandler(
        [this](const freeshape::ui::SelectOtherCandidate& candidate) {
            acceptSelectOtherCandidate(candidate);
        }
    );

    measurementHud_ = new freeshape::ui::MeasurementHud(viewportHost_);
    measurementHud_->move(
        std::max(12, viewportHost_->width() - measurementHud_->width() - 16),
        std::max(12, viewportHost_->height() - 110)
    );

    referenceOverlay_ =
        new freeshape::ui::ReferenceGeometryOverlay(view_, viewportHost_);
    referenceOverlay_->setGeometry(viewportHost_->rect());

    toast_ = new QLabel(viewportHost_);
    toast_->setObjectName(QStringLiteral("ViewportToast"));
    toast_->hide();
    toast_->raise();

    workspaceLayout->addWidget(viewportHost_, 1);
    root->addWidget(workspace, 1);

    documentTabs_ = new freeshape::ui::DocumentTabs(shell_);
    root->addWidget(documentTabs_);

    setCentralWidget(shell_);

    partStudioPanel_->setPlaneActivatedHandler(
        [this](const QString& objectName) {
            Gui::Selection().clearCompleteSelection();
            Gui::Selection().addSelection(
                document_->getName(),
                objectName.toUtf8().constData()
            );

            if (sketchController_ != nullptr
                && sketchController_->isAwaitingPlane()) {
                const QString label =
                    objectName == QStringLiteral("XY_Plane") ? QStringLiteral("Top")
                    : objectName == QStringLiteral("XZ_Plane") ? QStringLiteral("Front")
                                                               : QStringLiteral("Right");

                if (sketchController_->selectPlane(objectName)) {
                    featurePopup_->setSelectionText(label);
                    setReferencePlanesVisible(false);
                    updateToolbarContext(true);
                    refreshPartStudio();
                }
            }
        }
    );

    partStudioPanel_->setFeatureActivatedHandler(
        [this](const QString& objectName) {
            Gui::Selection().clearCompleteSelection();
            Gui::Selection().addSelection(
                document_->getName(),
                objectName.toUtf8().constData()
            );

            if (objectName == QStringLiteral("Pad")) {
                showExtrudeCommand();
            }
        }
    );
}

void FreeShapeWindow::showSketchCommand()
{
    if (extrudePreviewActive_) {
        finishExtrudePreview(false);
    }

    if (sketchController_ != nullptr) {
        sketchController_->beginSketch();
    }

    // Selecting the sketch support should feel like Onshape: the three base
    // reference planes are immediately visible and selectable.
    setReferencePlanesVisible(true);

    featurePopup_->setMode(freeshape::ui::FeaturePopup::Mode::Sketch);
    featurePopup_->setSelectionText(QStringLiteral("Select a sketch plane"));
    featurePopup_->move(14, 14);
    featurePopup_->show();
    featurePopup_->raise();

    updateToolbarContext(false);
    showToast(QStringLiteral("Sketch · select Top, Front, Right, or a planar face"));
}

void FreeShapeWindow::showExtrudeCommand()
{
    if (featurePopup_->isVisible()
        && featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Extrude) {
        return;
    }

    if (sketchController_ != nullptr && sketchController_->isEditing()) {
        if (!sketchController_->finishSketch(true)) {
            showToast(QStringLiteral("Extrude requires a closed sketch region"));
            return;
        }
        refreshPartStudio();
    }

    if (!beginExtrudePreview()) {
        showToast(QStringLiteral("Extrude requires Sketch 1 or a selected planar face"));
        return;
    }

    featurePopup_->setMode(freeshape::ui::FeaturePopup::Mode::Extrude);
    featurePopup_->setDepth(10.0);
    featurePopup_->move(14, 14);
    featurePopup_->show();
    featurePopup_->raise();
    featurePopup_->focusPrimaryField();

    updateToolbarContext(false);
    showToast(
        QStringLiteral(
            "Extrude · Sketch 1 region auto-selected · type depth · Enter accepts"
        ),
        4200
    );
}

void FreeShapeWindow::showFilletCommand()
{
    if (extrudePreviewActive_) {
        finishExtrudePreview(false);
    }

    featurePopup_->setMode(freeshape::ui::FeaturePopup::Mode::Fillet);
    featurePopup_->move(14, 14);
    featurePopup_->show();
    featurePopup_->raise();

    updateToolbarContext(false);
    showToast(QStringLiteral("Fillet · select edges or faces"));
}

void FreeShapeWindow::acceptFeatureCommand()
{
    if (featurePopup_ == nullptr || !featurePopup_->isVisible()) {
        return;
    }

    if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Extrude) {
        finishExtrudePreview(true);
    }

    if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Sketch) {
        if (sketchController_ != nullptr) {
            sketchController_->finishSketch(true);
        }
        refreshPartStudio();
    }

    featurePopup_->hide();
    updateToolbarContext(false);
}

void FreeShapeWindow::cancelFeatureCommand()
{
    if (featurePopup_ == nullptr || !featurePopup_->isVisible()) {
        return;
    }

    if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Extrude) {
        finishExtrudePreview(false);
    }
    else if (featurePopup_->mode() == freeshape::ui::FeaturePopup::Mode::Sketch
             && sketchController_ != nullptr) {
        sketchController_->finishSketch(false);
        setReferencePlanesVisible(true);
        refreshPartStudio();
    }

    featurePopup_->hide();
    updateToolbarContext(false);
    showToast(QStringLiteral("Command cancelled"));
}

bool FreeShapeWindow::beginExtrudePreview()
{
    if (extrudePreviewActive_ || document_ == nullptr) {
        return extrudePreviewActive_;
    }

    if (document_->getObject("Sketch") == nullptr) {
        return false;
    }

    document_->openTransaction("FreeShape Extrude Preview");
    extrudePreviewActive_ = true;
    extrudeCreatedPad_ = document_->getObject("Pad") == nullptr;

    if (extrudeCreatedPad_) {
        std::ostringstream script;
        script
            << "import FreeCAD as App\n"
            << "doc = App.getDocument('" << document_->getName() << "')\n"
            << "body = doc.getObject('Body')\n"
            << "sketch = doc.getObject('Sketch')\n"
            << "pad = body.newObject('PartDesign::Pad', 'Pad')\n"
            << "pad.Label = 'Extrude 1'\n"
            << "pad.Profile = sketch\n"
            << "pad.Length = 10.0\n"
            << "doc.recompute()\n";
        Base::Interpreter().runString(script.str().c_str());
        document_->recompute();

        if (document_->getObject("Pad") == nullptr) {
            document_->abortTransaction();
            extrudePreviewActive_ = false;
            extrudeCreatedPad_ = false;
            return false;
        }

        Base::Console().message("FREESHAPE_EXTRUDE_BEGIN PASS profile=Sketch\n");
    }

    refreshPartStudio();
    return true;
}

void FreeShapeWindow::applyExtrudePreview(double depth)
{
    if (!extrudePreviewActive_ || document_ == nullptr) {
        return;
    }

    std::ostringstream script;
    script
        << "doc = App.getDocument('" << document_->getName() << "')\n"
        << "pad = doc.getObject('Pad')\n"
        << "pad.Length = " << depth << "\n"
        << "doc.recompute()\n";

    Base::Interpreter().runString(script.str().c_str());
}

void FreeShapeWindow::finishExtrudePreview(bool accept)
{
    if (!extrudePreviewActive_ || document_ == nullptr) {
        return;
    }

    if (accept) {
        document_->commitTransaction();
        guiDocument_->setHide("Sketch");
        Base::Console().message(
            "FREESHAPE_EXTRUDE PASS depth={:.6f}\\n",
            featurePopup_ != nullptr ? featurePopup_->depth() : 0.0
        );
        showToast(QStringLiteral("Extrude accepted · Part 1 created"));
    }
    else {
        document_->abortTransaction();
        document_->recompute();
        guiDocument_->setShow("Sketch");
        Base::Console().message("FREESHAPE_EXTRUDE CANCEL\\n");
        showToast(QStringLiteral("Extrude cancelled"));
    }

    extrudePreviewActive_ = false;
    extrudeCreatedPad_ = false;
    refreshPartStudio();
}

void FreeShapeWindow::showShortcutPalette()
{
    if (shortcutPalette_ == nullptr) {
        return;
    }

    shortcutPalette_->setContext(
        sketchController_ != nullptr && sketchController_->isEditing()
            ? freeshape::ui::ShortcutPalette::Context::Sketch
            : freeshape::ui::ShortcutPalette::Context::PartStudio
    );
    shortcutPalette_->popupAtGlobal(QCursor::pos());
}

void FreeShapeWindow::showCommandSearch()
{
    if (commandSearch_ == nullptr || viewportHost_ == nullptr) {
        return;
    }

    const QPoint global = viewportHost_->mapToGlobal(
        QPoint(std::max(12, (viewportHost_->width() - 420) / 2), 12)
    );
    commandSearch_->popupAtGlobal(global);
}

void FreeShapeWindow::showViewportContextMenu(const QPoint& globalPosition)
{
    QMenu menu(this);

    const QString datum = !preselectedDatumPlane().isEmpty()
        ? preselectedDatumPlane()
        : selectedDatumPlane();

    if (!datum.isEmpty()) {
        const QString semantic = semanticPlaneName(datum);
        menu.addAction(
            QStringLiteral("New sketch on %1").arg(semantic),
            [this, datum] {
                showSketchCommand();
                if (sketchController_ != nullptr
                    && sketchController_->isAwaitingPlane()
                    && sketchController_->selectPlane(datum)) {
                    featurePopup_->setSelectionText(semanticPlaneName(datum));
                    setReferencePlanesVisible(false);
                    updateToolbarContext(true);
                    refreshPartStudio();
                }
            }
        );

        menu.addAction(
            QStringLiteral("View normal to %1").arg(semantic),
            [this] { normalToSelectionOrPlane(); }
        );
        menu.addSeparator();
    }
    else {
        menu.addAction(QStringLiteral("Sketch"), [this] {
            showSketchCommand();
        });
        menu.addAction(QStringLiteral("Extrude"), [this] {
            showExtrudeCommand();
        });
        menu.addAction(QStringLiteral("Fillet"), [this] {
            showFilletCommand();
        });
        menu.addSeparator();
    }

    menu.addAction(QStringLiteral("Zoom to fit"), [this] {
        commands_->trigger(QStringLiteral("fit"));
    });
    menu.addAction(QStringLiteral("Select other"), [this] {
        cycleSelectOther();
    });

    menu.addSeparator();

    menu.addAction(QStringLiteral("Hide"), [this] {
        hideSelected();
    });
    menu.addAction(QStringLiteral("Show reference planes"), [this] {
        setReferencePlanesVisible(true);
    });
    menu.addAction(QStringLiteral("Clear selection"), [] {
        Gui::Selection().clearCompleteSelection();
    });

    menu.exec(globalPosition);
}

void FreeShapeWindow::togglePlanes()
{
    setReferencePlanesVisible(!planesVisible_);

    showToast(
        planesVisible_ ? QStringLiteral("Planes shown")
                       : QStringLiteral("Planes hidden")
    );
}

void FreeShapeWindow::setReferencePlanesVisible(bool visible)
{
    planesVisible_ = visible;

    // PartDesign Body owns an App::Origin. In FreeCAD's scene graph the Body
    // claims Origin, and Origin claims its planes. Parent visibility therefore
    // gates the child plane ViewProviders.
    if (visible) {
        guiDocument_->setShow("Body");
        guiDocument_->setShow("Origin");

        // FreeShape's default-geometry presentation is intentionally planes
        // only. Keep the FreeCAD origin axes out of the initial Part Studio.
        for (const char* axis : {"X_Axis", "Y_Axis", "Z_Axis"}) {
            guiDocument_->setHide(axis);
        }

        for (const char* plane : {"XY_Plane", "XZ_Plane", "YZ_Plane"}) {
            guiDocument_->setShow(plane);
        }

        styleReferencePlanes();
        if (referenceOverlay_ != nullptr) {
            referenceOverlay_->setReferenceGeometryVisible(true);
        }

        Base::Console().message(
            "FREESHAPE_REFERENCE_PLANES PASS origin={} xy={} xz={} yz={}\n",
            guiDocument_->isShow("Origin"),
            guiDocument_->isShow("XY_Plane"),
            guiDocument_->isShow("XZ_Plane"),
            guiDocument_->isShow("YZ_Plane")
        );
    }
    else {
        for (const char* plane : {"XY_Plane", "XZ_Plane", "YZ_Plane"}) {
            guiDocument_->setHide(plane);
        }
        guiDocument_->setHide("Origin");
        if (referenceOverlay_ != nullptr) {
            referenceOverlay_->setReferenceGeometryVisible(false);
        }
    }

    if (view_ != nullptr) {
        view_->update();
    }
}

void FreeShapeWindow::styleReferencePlanes()
{
    // Current FreeCAD origin-plane ViewProviders are already translucent and
    // selectable. Keep those engine-backed pick targets, but make them behave
    // more like persistent Part Studio reference planes: always labeled and
    // a little larger than FreeCAD's default datum size.
    for (const char* name : {"XY_Plane", "XZ_Plane", "YZ_Plane"}) {
        auto* object = document_ != nullptr ? document_->getObject(name) : nullptr;
        if (object == nullptr) {
            continue;
        }

        auto* provider = Gui::Application::Instance->getViewProvider(object);
        auto* plane = dynamic_cast<Gui::ViewProviderPlane*>(provider);
        if (plane == nullptr) {
            continue;
        }

        plane->resetTemporarySize();
        plane->setTemporaryScale(2.25);
        plane->setLabelVisibility(false);
    }
}

void FreeShapeWindow::toggleSketches()
{
    sketchVisible_ = !sketchVisible_;
    if (sketchVisible_) {
        guiDocument_->setShow("Sketch");
    }
    else {
        guiDocument_->setHide("Sketch");
    }

    showToast(
        sketchVisible_ ? QStringLiteral("Sketches shown")
                       : QStringLiteral("Sketches hidden")
    );
}

void FreeShapeWindow::hideSelected()
{
    lastHiddenObjects_.clear();

    const auto selected = Gui::Selection().getSelection();
    for (const auto& item : selected) {
        if (item.FeatName == nullptr) {
            continue;
        }

        const QString object = QString::fromUtf8(item.FeatName);
        const QString sub = QString::fromUtf8(item.SubName != nullptr ? item.SubName : "");
        const QString plane = datumPlaneFromSelection(object, sub);
        const std::string name = !plane.isEmpty()
            ? plane.toStdString()
            : object.toStdString();

        if (std::find(lastHiddenObjects_.begin(), lastHiddenObjects_.end(), name)
            == lastHiddenObjects_.end()) {
            lastHiddenObjects_.push_back(name);
            guiDocument_->setHide(name.c_str());
        }
    }

    // Onshape's Y shortcut also acts on the entity under the cursor. Use the
    // live preselection when there is no explicit selection.
    if (lastHiddenObjects_.empty() && !lastPreselectionObject_.isEmpty()) {
        const QString plane = datumPlaneFromSelection(
            lastPreselectionObject_,
            lastPreselectionSubElement_
        );
        const std::string name = !plane.isEmpty()
            ? plane.toStdString()
            : lastPreselectionObject_.toStdString();
        lastHiddenObjects_.push_back(name);
        guiDocument_->setHide(name.c_str());
    }

    if (!lastHiddenObjects_.empty()) {
        Gui::Selection().clearCompleteSelection();
        showToast(QStringLiteral("Hidden"));
    }
}

void FreeShapeWindow::showLastHidden()
{
    for (const auto& name : lastHiddenObjects_) {
        guiDocument_->setShow(name.c_str());
    }

    if (!lastHiddenObjects_.empty()) {
        showToast(QStringLiteral("Previously hidden entities shown"));
    }
}

void FreeShapeWindow::refreshSelectOtherCandidates()
{
    if (selectOtherPopup_ == nullptr || document_ == nullptr) {
        return;
    }

    std::vector<freeshape::ui::SelectOtherCandidate> candidates;
    for (const auto& item : Gui::Selection().getPickedList(document_->getName())) {
        freeshape::ui::SelectOtherCandidate candidate;
        candidate.document = QString::fromUtf8(item.DocName != nullptr ? item.DocName : "");
        candidate.object = QString::fromUtf8(item.FeatName != nullptr ? item.FeatName : "");
        candidate.subElement = QString::fromUtf8(item.SubName != nullptr ? item.SubName : "");
        candidate.typeName = QString::fromUtf8(item.TypeName.data(), static_cast<int>(item.TypeName.size()));
        candidate.x = item.x;
        candidate.y = item.y;
        candidate.z = item.z;
        candidates.push_back(std::move(candidate));
    }

    selectOtherPopup_->setCandidates(std::move(candidates));
}

void FreeShapeWindow::cycleSelectOther(int delta)
{
    if (selectOtherPopup_ == nullptr) {
        return;
    }

    if (!selectOtherPopup_->isVisible()) {
        refreshSelectOtherCandidates();
        const QPoint global = viewportHost_ != nullptr
            ? viewportHost_->mapToGlobal(lastViewportCursor_ + QPoint(12, 12))
            : QCursor::pos();
        selectOtherPopup_->popupAtGlobal(global);
    }
    else {
        selectOtherPopup_->cycle(delta);
    }
}

void FreeShapeWindow::acceptSelectOtherCandidate(
    const freeshape::ui::SelectOtherCandidate& candidate
)
{
    Gui::Selection().clearCompleteSelection(false);
    Gui::Selection().addSelection(
        candidate.document.toUtf8().constData(),
        candidate.object.toUtf8().constData(),
        candidate.subElement.isEmpty() ? nullptr : candidate.subElement.toUtf8().constData(),
        candidate.x,
        candidate.y,
        candidate.z,
        nullptr,
        false,
        Gui::SelectionChanges::PickedPoint::Valid
    );
    refreshSelectionUi();
}

QString FreeShapeWindow::selectedDatumPlane() const
{
    const auto selected = Gui::Selection().getSelection();
    if (selected.empty()) {
        return {};
    }

    const auto& item = selected.back();
    return datumPlaneFromSelection(
        QString::fromUtf8(item.FeatName != nullptr ? item.FeatName : ""),
        QString::fromUtf8(item.SubName != nullptr ? item.SubName : "")
    );
}

QString FreeShapeWindow::preselectedDatumPlane() const
{
    return datumPlaneFromSelection(
        lastPreselectionObject_,
        lastPreselectionSubElement_
    );
}

void FreeShapeWindow::normalToSelectionOrPlane()
{
    QString plane = preselectedDatumPlane();
    if (plane.isEmpty()) {
        plane = selectedDatumPlane();
    }

    const char* message = nullptr;
    if (plane == QStringLiteral("XY_Plane")) {
        message = "ViewTop";
    }
    else if (plane == QStringLiteral("XZ_Plane")) {
        message = "ViewFront";
    }
    else if (plane == QStringLiteral("YZ_Plane")) {
        message = "ViewRight";
    }

    if (message != nullptr) {
        guiDocument_->sendMsgToViews(message);
        showToast(
            QStringLiteral("View normal to %1").arg(semanticPlaneName(plane))
        );
        return;
    }

    showToast(QStringLiteral("Normal to · hover or select a planar face/plane"));
}

void FreeShapeWindow::refreshSelectionUi()
{
    const auto selection = Gui::Selection().getSelection();
    const int count = static_cast<int>(selection.size());

    if (selectionOverlay_ != nullptr) {
        selectionOverlay_->setSelectionCount(count);
    }

    if (featurePopup_ != nullptr && featurePopup_->isVisible()
        && featurePopup_->mode() != freeshape::ui::FeaturePopup::Mode::Sketch) {
        QStringList labels;
        for (const auto& item : selection) {
            const QString object =
                QString::fromUtf8(item.FeatName != nullptr ? item.FeatName : "");
            const QString sub =
                QString::fromUtf8(item.SubName != nullptr ? item.SubName : "");
            labels.push_back(semanticSelectionText(object, sub));
            if (labels.size() == 3 && selection.size() > 3) {
                labels.push_back(
                    QStringLiteral("+%1 more").arg(selection.size() - 3)
                );
                break;
            }
        }

        featurePopup_->setEntitySelectionText(
            labels.isEmpty() ? QStringLiteral("Select entities")
                             : labels.join(QStringLiteral(", "))
        );
    }

    if (measurementHud_ != nullptr) {
        measurementHud_->refresh();
        measurementHud_->move(
            std::max(12, viewportHost_->width() - measurementHud_->width() - 16),
            std::max(12, viewportHost_->height() - measurementHud_->height() - 48)
        );
    }
}

void FreeShapeWindow::showToast(const QString& text, int milliseconds)
{
    if (toast_ == nullptr || viewportHost_ == nullptr) {
        return;
    }

    toast_->setText(text);
    toast_->adjustSize();
    toast_->move(
        14,
        std::max(14, viewportHost_->height() - toast_->height() - 14)
    );
    toast_->show();
    toast_->raise();

    QTimer::singleShot(milliseconds, toast_, [label = toast_, text] {
        if (label->text() == text) {
            label->hide();
        }
    });
}

void FreeShapeWindow::updateToolbarContext(bool sketchMode)
{
    featureToolbar_->setVisible(!sketchMode);
    sketchToolbar_->setVisible(sketchMode);

    if (commands_ != nullptr) {
        for (const QString& id : {
                 QStringLiteral("sketch_circle"),
                 QStringLiteral("sketch_line"),
                 QStringLiteral("sketch_rectangle"),
                 QStringLiteral("sketch_center_rectangle")
             }) {
            if (auto* action = commands_->action(id); action != nullptr) {
                action->setEnabled(sketchMode);
            }
        }
    }

    syncActiveToolButtons();
}

void FreeShapeWindow::syncActiveToolButtons()
{
    if (sketchToolbar_ == nullptr || sketchController_ == nullptr) {
        return;
    }

    using Tool = freeshape::sketch::SketchController::Tool;
    const Tool active = sketchController_->activeTool();

    struct State {
        const char* objectName;
        Tool tool;
    };

    const State states[] = {
        {"tool_sketch_line", Tool::Line},
        {"tool_sketch_circle", Tool::Circle},
        {"tool_sketch_rectangle", Tool::CornerRectangle},
        {"tool_sketch_center_rectangle", Tool::CenterRectangle},
    };

    for (const auto& state : states) {
        if (auto* button = sketchToolbar_->findChild<QToolButton*>(
                QString::fromUtf8(state.objectName));
            button != nullptr) {
            button->blockSignals(true);
            button->setChecked(active == state.tool);
            button->blockSignals(false);
        }
    }
}

void FreeShapeWindow::refreshPartStudio()
{
    if (partStudioPanel_ != nullptr) {
        partStudioPanel_->refreshFromDocument(document_);
    }
}

}  // namespace freeshape::app
