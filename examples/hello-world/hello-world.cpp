#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class DummyController
        : public minire::BasicController
    {
        using BasicController::BasicController;
    };
}

int main()
{
    static size_t const kMaxCtrlFps = 60;

    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);
        minire::content::Manager manager;
        minire::Application application(1280, 720, "Hello world", manager);
        application.setController<DummyController>(kMaxCtrlFps);

        application.setVsync(false); // Turn on to v-sync to benchmark max FPS

        // Main loop
        application.run();

        // Finish
        return EXIT_SUCCESS;
    }
    catch(std::exception const & e)
    {
        MINIRE_ERROR("Fatal error:\n{}", e.what());
    }
    catch(...)
    {
        MINIRE_ERROR("Fatal error: (unknown error)");
    }

    return EXIT_FAILURE;
}
