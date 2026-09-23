#pragma once

#include <string>

namespace App {
class Document;
}

namespace freeshape::freecad {

// Creates the smallest useful parametric model for the embedding spike:
// PartDesign Body -> closed Sketch -> Pad.
App::Document* createSpikeDocument();

// Saves the document as FCStd, closes it through App::Application, reopens the
// saved file, recomputes it, and verifies that the semantic spike objects
// survived.  Returns the newly opened document.
App::Document* saveCloseReloadSpikeDocument(
    App::Document* document,
    const std::string& fileName
);


// Creates the blank visible Part Studio document used by FreeShape after
// the hidden engine/FCStd smoke round-trip has passed.
App::Document* createWorkspaceDocument();

}  // namespace freeshape::freecad
