#include "freecad/SpikeModel.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>

namespace freeshape::freecad {

namespace {

void verifySpikeDocument(App::Document* document, const char* stage)
{
    if (document == nullptr) {
        throw std::runtime_error(std::string(stage) + ": null document");
    }

    bool hasError = false;
    document->recompute({}, false, &hasError);
    if (hasError) {
        throw std::runtime_error(std::string(stage) + ": recompute reported errors");
    }

    for (const char* objectName : {"Body", "Sketch", "Pad"}) {
        if (document->getObject(objectName) == nullptr) {
            throw std::runtime_error(
                std::string(stage) + ": required object missing: " + objectName
            );
        }
    }

    Base::Console().message(
        "FREESHAPE_MODEL_VERIFY PASS stage={} objects={}\n",
        stage,
        document->countObjects()
    );
}

}  // namespace

App::Document* createSpikeDocument()
{
    App::Document* document = App::GetApplication().newDocument("FreeShapeSpike");
    if (document == nullptr) {
        return nullptr;
    }

    constexpr const char* bootstrapModel = R"PY(
import FreeCAD as App
import Part
import Sketcher

doc = App.ActiveDocument
doc.openTransaction("FreeShape bootstrap model")

body = doc.addObject("PartDesign::Body", "Body")
sketch = body.newObject("Sketcher::SketchObject", "Sketch")

geometry = [
    Part.LineSegment(App.Vector(-20, -12, 0), App.Vector(20, -12, 0)),
    Part.LineSegment(App.Vector(20, -12, 0), App.Vector(20, 12, 0)),
    Part.LineSegment(App.Vector(20, 12, 0), App.Vector(-20, 12, 0)),
    Part.LineSegment(App.Vector(-20, 12, 0), App.Vector(-20, -12, 0)),
]
sketch.addGeometry(geometry, False)

pad = body.newObject("PartDesign::Pad", "Pad")
pad.Profile = sketch
pad.Length = 10.0

doc.recompute()
doc.commitTransaction()
)PY";

    Base::Interpreter().runString(bootstrapModel);
    verifySpikeDocument(document, "created");
    return document;
}

App::Document* saveCloseReloadSpikeDocument(
    App::Document* document,
    const std::string& fileName
)
{
    verifySpikeDocument(document, "pre-save");

    const std::filesystem::path path(fileName);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::error_code removeError;
    std::filesystem::remove(path, removeError);

    if (!document->saveAs(fileName.c_str())) {
        throw std::runtime_error("saveAs() failed for " + fileName);
    }

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("FCStd file does not exist after save: " + fileName);
    }

    const auto size = std::filesystem::file_size(path);
    if (size == 0) {
        throw std::runtime_error("FCStd file is empty after save: " + fileName);
    }

    Base::Console().message(
        "FREESHAPE_SAVE PASS path={} bytes={}\n",
        fileName,
        size
    );

    if (!App::GetApplication().closeDocument(document)) {
        throw std::runtime_error("closeDocument() failed during FCStd round trip");
    }

    Base::Console().message("FREESHAPE_CLOSE PASS\n");

    App::Document* reopened = App::GetApplication().openDocument(fileName.c_str());
    if (reopened == nullptr) {
        throw std::runtime_error("openDocument() failed during FCStd round trip");
    }

    verifySpikeDocument(reopened, "reloaded");

    Base::Console().message(
        "FREESHAPE_RELOAD PASS document={}\n",
        reopened->getName()
    );

    return reopened;
}


App::Document* createWorkspaceDocument()
{
    App::Document* document = App::GetApplication().newDocument("FreeShape");
    if (document == nullptr) {
        return nullptr;
    }

    constexpr const char* bootstrapWorkspace = R"PY(
import FreeCAD as App

doc = App.ActiveDocument
doc.openTransaction("FreeShape blank Part Studio")
body = doc.addObject("PartDesign::Body", "Body")
body.Label = "Part Studio 1"
doc.recompute()

# Preserve stable internal names used by FreeCAD while presenting the same
# semantic default-geometry vocabulary as FreeShape's Part Studio UI.
doc.getObject("Origin").Label = "Origin"
doc.getObject("XY_Plane").Label = "Top"
doc.getObject("XZ_Plane").Label = "Front"
doc.getObject("YZ_Plane").Label = "Right"
doc.recompute()
doc.commitTransaction()
)PY";

    Base::Interpreter().runString(bootstrapWorkspace);
    document->recompute();

    Base::Console().message(
        "FREESHAPE_WORKSPACE PASS document={} objects={}\n",
        document->getName(),
        document->countObjects()
    );

    return document;
}

}  // namespace freeshape::freecad
