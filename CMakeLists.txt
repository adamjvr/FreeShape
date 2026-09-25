cmake_minimum_required(VERSION 3.28)

project(FreeShape VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "FreeShape currently targets Linux only.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FREESHAPE_FREECAD_SOURCE
    "${CMAKE_SOURCE_DIR}/third_party/FreeCAD"
    CACHE PATH "Path to the pinned FreeCAD source tree")

set(FREESHAPE_FREECAD_BUILD
    "${FREESHAPE_FREECAD_SOURCE}/build/relWithDebInfo"
    CACHE PATH "Path to the independently configured FreeCAD build tree")

if(NOT EXISTS "${FREESHAPE_FREECAD_SOURCE}/src/App/Application.h")
    message(FATAL_ERROR "FreeCAD source tree not initialized. Run scripts/bootstrap.sh.")
endif()

if(NOT EXISTS "${FREESHAPE_FREECAD_BUILD}/CMakeCache.txt")
    message(FATAL_ERROR "FreeCAD build tree not configured. Run scripts/bootstrap.sh.")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
    Xml
    PrintSupport
    Svg
    SvgWidgets
    Network
    UiTools
)
find_package(OpenGL REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(Eigen3 REQUIRED NO_MODULE)

find_library(FREESHAPE_FREECAD_BASE
    NAMES FreeCADBase
    PATHS "${FREESHAPE_FREECAD_BUILD}/lib"
    NO_DEFAULT_PATH REQUIRED)

find_library(FREESHAPE_FREECAD_APP
    NAMES FreeCADApp
    PATHS "${FREESHAPE_FREECAD_BUILD}/lib"
    NO_DEFAULT_PATH REQUIRED)

find_library(FREESHAPE_FREECAD_GUI
    NAMES FreeCADGui
    PATHS "${FREESHAPE_FREECAD_BUILD}/lib"
    NO_DEFAULT_PATH REQUIRED)

# FreeShape now directly uses Coin math/camera types in SketchController.
# Linking FreeCADGui is not sufficient for direct executable references:
# the executable must link Coin itself.  The bundled Coin subproject writes
# libraries into FreeCAD's top-level build/lib directory.
find_library(FREESHAPE_COIN_LIBRARY
    NAMES Coin
    PATHS "${FREESHAPE_FREECAD_BUILD}/lib"
    NO_DEFAULT_PATH REQUIRED)

add_library(FreeShapeFreeCADBridge INTERFACE)
add_library(FreeShape::FreeCADBridge ALIAS FreeShapeFreeCADBridge)

set(FREESHAPE_FREECAD_FASTSIGNALS_INCLUDE
    "${FREESHAPE_FREECAD_SOURCE}/src/3rdParty/FastSignals/libfastsignals/include")
set(FREESHAPE_FREECAD_PYCXX_INCLUDE
    "${FREESHAPE_FREECAD_SOURCE}/src/3rdParty/PyCXX")
set(FREESHAPE_FREECAD_COIN_SOURCE_INCLUDE
    "${FREESHAPE_FREECAD_SOURCE}/src/3rdParty/coin/include")
set(FREESHAPE_FREECAD_COIN_BUILD_INCLUDE
    "${FREESHAPE_FREECAD_BUILD}/src/3rdParty/coin/include")

foreach(required_header
    "${FREESHAPE_FREECAD_FASTSIGNALS_INCLUDE}/fastsignals/signal.h"
    "${FREESHAPE_FREECAD_PYCXX_INCLUDE}/CXX/Objects.hxx"
    "${FREESHAPE_FREECAD_COIN_BUILD_INCLUDE}/Inventor/C/basic.h"
    "${FREESHAPE_FREECAD_BUILD}/config.h"
)
    if(NOT EXISTS "${required_header}")
        message(FATAL_ERROR
            "Required FreeCAD consumer header is missing: ${required_header}")
    endif()
endforeach()

target_include_directories(FreeShapeFreeCADBridge
    SYSTEM INTERFACE
        "${FREESHAPE_FREECAD_SOURCE}/src"
        "${FREESHAPE_FREECAD_BUILD}"
        "${FREESHAPE_FREECAD_BUILD}/src"

        "${FREESHAPE_FREECAD_SOURCE}/src/App"
        "${FREESHAPE_FREECAD_BUILD}/src/App"
        "${FREESHAPE_FREECAD_SOURCE}/src/Base"
        "${FREESHAPE_FREECAD_BUILD}/src/Base"

        "${FREESHAPE_FREECAD_SOURCE}/src/Gui"
        "${FREESHAPE_FREECAD_BUILD}/src/Gui"
        "${FREESHAPE_FREECAD_SOURCE}/src/Gui/Quarter"
        "${FREESHAPE_FREECAD_SOURCE}/src/Gui/PreferencePages"
        "${FREESHAPE_FREECAD_SOURCE}/src/Gui/Selection"
        "${FREESHAPE_FREECAD_SOURCE}/src/Gui/StyleParameters"
        "${FREESHAPE_FREECAD_BUILD}/src/Gui/Language"
        "${FREESHAPE_FREECAD_BUILD}/src/Gui/propertyeditor"
        "${FREESHAPE_FREECAD_BUILD}/src/Gui/TaskView"
        "${FREESHAPE_FREECAD_BUILD}/src/Gui/Quarter"
        "${FREESHAPE_FREECAD_BUILD}/src/Gui/DAGView"

        "${FREESHAPE_FREECAD_FASTSIGNALS_INCLUDE}"
        "${FREESHAPE_FREECAD_PYCXX_INCLUDE}"
        "${FREESHAPE_FREECAD_COIN_SOURCE_INCLUDE}"
        "${FREESHAPE_FREECAD_COIN_BUILD_INCLUDE}"
        "${FREESHAPE_FREECAD_SOURCE}/src/3rdParty/GSL/include"
)

target_compile_definitions(FreeShapeFreeCADBridge
    INTERFACE
        QT_NO_KEYWORDS
        HAVE_CONFIG_H
        MINIMUM_CPLUSPLUS_VERSION=202302L
        PYCXX_6_2_COMPATIBILITY
        BOOST_NO_CXX98_FUNCTION_BASE
)

target_compile_features(FreeShapeFreeCADBridge INTERFACE cxx_std_23)

target_link_libraries(FreeShapeFreeCADBridge
    INTERFACE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Qt6::Xml
        Qt6::PrintSupport
        Qt6::Svg
        Qt6::SvgWidgets
        Qt6::Network
        Qt6::UiTools
        OpenGL::GL
        Python3::Python
        Eigen3::Eigen
        # Direct Coin dependency: SketchController calls SbViewVolume,
        # SbLine, SbPlane and SoRenderManager symbols directly.
        "${FREESHAPE_COIN_LIBRARY}"
        "${FREESHAPE_FREECAD_GUI}"
        "${FREESHAPE_FREECAD_APP}"
        "${FREESHAPE_FREECAD_BASE}"
)

add_executable(FreeShape
    src/app/main.cpp
    src/app/FreeShapeWindow.cpp
    src/app/FreeShapeWindow.h

    src/commands/CommandRegistry.cpp
    src/commands/CommandRegistry.h

    src/input/ViewportInteractionFilter.cpp
    src/input/ViewportInteractionFilter.h

    src/sketch/SketchController.cpp
    src/sketch/SketchController.h
    src/sketch/SketchOverlay.cpp
    src/sketch/SketchOverlay.h

    src/ui/CommandSearch.cpp
    src/ui/CommandSearch.h
    src/ui/DesignSystem.cpp
    src/ui/SelectionOverlay.cpp
    src/ui/SelectionOverlay.h
    src/ui/SelectOtherPopup.cpp
    src/ui/SelectOtherPopup.h
    src/ui/MeasurementHud.cpp
    src/ui/MeasurementHud.h
    src/ui/ReferenceGeometryOverlay.cpp
    src/ui/ReferenceGeometryOverlay.h
    src/ui/DesignSystem.h
    src/ui/DocumentTabs.cpp
    src/ui/DocumentTabs.h
    src/ui/FeaturePopup.cpp
    src/ui/FeaturePopup.h
    src/ui/PartStudioPanel.cpp
    src/ui/PartStudioPanel.h
    src/ui/ShortcutPalette.cpp
    src/ui/ShortcutPalette.h

    src/freecad/SpikeModel.cpp
    src/freecad/SpikeModel.h
)

target_include_directories(FreeShape PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(FreeShape PRIVATE
    FreeShape::FreeCADBridge
)

set_target_properties(FreeShape PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${FREESHAPE_FREECAD_BUILD}/bin"
    BUILD_RPATH "${FREESHAPE_FREECAD_BUILD}/lib"
)

target_compile_options(FreeShape PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
