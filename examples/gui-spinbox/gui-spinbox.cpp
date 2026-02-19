#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/spinbox.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <cstdlib> // for EXIT_SUCCESS

using namespace minire::gui;
using namespace minire::gui::components;
using namespace minire::utils;

namespace
{
    static std::string const kFontFace = "ucs-6x13-example";

    class GuiSpinbox
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:
        void start() override
        {
            GuiController::start();

            auto layout = std::make_shared<layouts::Grid>(2, 1);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;
            container->horizontal() = Arranger::fill();
            container->vertical() = Arranger::fill();

            {
                auto spinbox = container->emplace<SpinBox>("spinedit-0",
                    [this](minire::text::FormattedString const & v) { return makeTextView(v, kFontFace); });

                layout->set(0, 0, spinbox->id());
                spinbox->horizontal() = Arranger(position::Center{}, dimension::Constant{100});
                spinbox->vertical() = Arranger(position::Center{}, dimension::Constant{24});
                spinbox->setCallback(std::in_place_type<spinbox::OnValueChanged>, "foo",
                    [](Component & component, spinbox::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto spinbox = container->emplace<SpinBox>("spinedit-1",
                    [this](minire::text::FormattedString const & v) { return makeTextView(v, kFontFace); });

                layout->set(1, 0, spinbox->id());
                spinbox->horizontal() = Arranger(position::Center{}, dimension::Constant{150});
                spinbox->vertical() = Arranger(position::Center{}, dimension::Constant{24});
                spinbox->format() = "{:f}";
                spinbox->spacing() = 3;
                spinbox->setCallback(std::in_place_type<spinbox::OnValueChanged>, "foo",
                    [](Component & component, spinbox::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });

                spinbox->setValue(3.1415f);
            }
        }
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
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

        auto lease = manager.upload(kFontFace, minire::models::FontFace
            {
                ._regular = "../common/6x13.bdf",
                ._bold = "../common/6x13B.bdf",
                ._italic = "../common/6x13O.bdf",
                ._glyphWidth = 6,
                ._glyphHeight = 13,
            });

        minire::Application application(1280, 720, "GUI Spinbox", manager);
        application.setController<GuiSpinbox>(kMaxCtrlFps);
        application.setGlDebug(false);
        application.setVsync(true);

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
