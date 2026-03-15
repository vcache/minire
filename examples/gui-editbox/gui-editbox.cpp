#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/editbox.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>
#include <minire/models/font-face.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <cstdlib> // for EXIT_SUCCESS

using namespace minire::gui;
using namespace minire::gui::components;
using namespace minire::utils;

namespace
{
    class GuiEditbox
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

    protected:
        void onStart() override
        {
            using namespace minire::gui;

            GuiApplication::onStart();

            // base container

            auto layout = std::make_shared<layouts::Grid>(2, 2);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;

            Editbox::Sptr disabledEditBox;

            // regular editbox

            {
                // destination

                auto editbox1 = container->emplace<Editbox>("regular-1");
                editbox1->horizontal()  = Arranger(position::Center{}, dimension::Constant{200});
                editbox1->vertical()    = Arranger(position::Center{}, dimension::Constant{25});

                editbox1->setCallback(std::in_place_type<editbox::OnTextChanged>, "foobar",
                    [](Component const & component, editbox::OnTextChanged const & event)
                    {
                        MINIRE_INFO("A Text was changed for \"{}\": \"{}\"",
                                    component.id(), event._current.unformat());
                    });

                editbox1->setCallback(std::in_place_type<minire::application::OnKeyDown>, "keydown",
                    [this](Component const & component, minire::application::OnKeyDown const & event)
                    {
                        std::string kind = "UNKNOWN";
                        switch(event._key)
                        {
                            case SDLK_RETURN:
                                kind = "SUBMIT";
                                unfocus();
                                break;

                            case SDLK_ESCAPE:
                                kind = "CANCEL";
                                unfocus();
                                break;
                        }
                        MINIRE_INFO("A keyboard event for \"{}\": {}", component.id(), kind);
                    });

                layout->set(0, 0, editbox1->id());

                // source

                auto editbox2 = container->emplace<Editbox>("regular-2");

                editbox2->horizontal()  = Arranger(position::Center{}, dimension::Constant{200});
                editbox2->vertical()    = Arranger(position::Center{}, dimension::Constant{25});
                editbox2->setCallback(std::in_place_type<editbox::OnTextChanged>, "foobar",
                    [editbox1, container]
                    (Component const & component, editbox::OnTextChanged const & event)
                    {
                        MINIRE_INFO("A Text was changed for \"{}\": \"{}\"",
                                    component.id(), event._current.unformat());

                        editbox1->editText(
                            [&event](Property<minire::text::FormattedString> & text)
                            { text = event._current; });

                        container->at<Editbox>("disabled").editText(
                            [&event](Property<minire::text::FormattedString> & text)
                            { text = event._current; });
                    });

                layout->set(1, 0, editbox2->id());
            }

            // password box

            {

                auto editbox = container->emplace<Editbox>("password");

                editbox->horizontal()  = Arranger(position::Center{}, dimension::Constant{200});
                editbox->vertical()    = Arranger(position::Center{}, dimension::Constant{25});

                editbox->setCallback(std::in_place_type<editbox::OnTextChanged>, "foobar",
                    [](Component const & component, editbox::OnTextChanged const & event)
                    {
                        MINIRE_INFO("A Text was changed for \"{}\": \"{}\"",
                                    component.id(), event._current.unformat());
                    });

                editbox->passwordChar() = L'*';

                layout->set(0, 1, editbox->id());
            }

            // disabled editbox

            {
                auto editbox = container->emplace<Editbox>("disabled");
                editbox->horizontal()  = Arranger(position::Center{}, dimension::Constant{200});
                editbox->vertical()    = Arranger(position::Center{}, dimension::Constant{25});

                editbox->setCallback(std::in_place_type<editbox::OnTextChanged>, "foobar",
                    [](Component const & component, editbox::OnTextChanged const & event)
                    {
                        MINIRE_INFO("A Text was changed for \"{}\": \"{}\"",
                                    component.id(), event._current.unformat());
                    });

                editbox->enabled() = false;

                layout->set(1, 1, editbox->id());
            }
        }

    private:
        std::vector<std::vector<std::any>> const _cases;
    };
}

int main()
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);
        GuiEditbox application(1280, 720, "GUI Editbox", manager);
        application.setVsync(true);
        application.setGlDebug(false);

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
