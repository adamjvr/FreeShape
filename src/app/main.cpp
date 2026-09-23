#include <clocale>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <stdexcept>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/GuiApplication.h>
#include <Gui/MainWindow.h>
#include <Gui/StartupProcess.h>

#include <QCoreApplication>
#include <QEvent>
#include <QSurfaceFormat>

#include "app/FreeShapeWindow.h"
#include "freecad/SpikeModel.h"

namespace {

void configureSurfaceFormat()
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setOption(QSurfaceFormat::DeprecatedFunctions, true);

    if (std::getenv("WAYLAND_DISPLAY") != nullptr) {
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        format.setAlphaBufferSize(8);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
    }

    QSurfaceFormat::setDefaultFormat(format);
}

std::filesystem::path spikeRoundTripPath()
{
    const char* tempRoot = std::getenv("FREECAD_USER_TEMP");
    std::filesystem::path root =
        (tempRoot != nullptr && *tempRoot != '\0')
            ? std::filesystem::path(tempRoot)
            : std::filesystem::temp_directory_path();

    root /= "freeshape-spike";
    std::filesystem::create_directories(root);
    return root / "FreeShapeSpike.FCStd";
}

}  // namespace

int main(int argc, char** argv)
{
    try {
        App::Application::Config()["RunMode"] = "Gui";
        App::Application::Config()["Console"] = "1";
        App::Application::Config()["LoggingConsole"] = "1";
        App::Application::Config()["StartHidden"] = "1";

        App::Application::init(argc, argv);
        Gui::Application::initApplication();

        Gui::StartupProcess::setupApplication();
        configureSurfaceFormat();

        Gui::GUIApplication qtApplication(argc, argv);
        std::setlocale(LC_NUMERIC, "C");

        Gui::StartupProcess startup;
        startup.execute();

        Gui::Application guiApplication(true);

        Gui::MainWindow compatibilityMainWindow;
        compatibilityMainWindow.setProperty("QuitOnClosed", false);

        Gui::StartupPostProcess postProcess(
            &compatibilityMainWindow,
            guiApplication,
            &qtApplication
        );
        postProcess.execute();
        compatibilityMainWindow.hide();

        int returnCode = 0;
        {
            App::Document* document = freeshape::freecad::createSpikeDocument();
            if (document == nullptr) {
                throw std::runtime_error("failed to create spike document");
            }

            const std::filesystem::path roundTripPath = spikeRoundTripPath();
            document = freeshape::freecad::saveCloseReloadSpikeDocument(
                document,
                roundTripPath.string()
            );

            if (document == nullptr) {
                throw std::runtime_error("FCStd round trip returned a null document");
            }

            Base::Console().message(
                "FREESHAPE_ROUNDTRIP PASS path={} objects={}\n",
                roundTripPath.string(),
                document->countObjects()
            );

            // The smoke model proved persistence/engine behavior.  Do not
            // expose that artificial box as the user's Part Studio.
            if (!App::GetApplication().closeDocument(document)) {
                throw std::runtime_error("failed to close smoke-test document");
            }

            document = freeshape::freecad::createWorkspaceDocument();
            if (document == nullptr) {
                throw std::runtime_error("failed to create blank FreeShape Part Studio");
            }

            freeshape::app::FreeShapeWindow window(document);
            window.show();
            returnCode = qtApplication.exec();
        }

        App::GetApplication().closeAllDocuments();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        return returnCode;
    }
    catch (const std::exception& exception) {
        Base::Console().error("FreeShape fatal error: {}\n", exception.what());
        return 1;
    }
    catch (...) {
        Base::Console().error("FreeShape fatal error: unknown exception\n");
        return 1;
    }
}
