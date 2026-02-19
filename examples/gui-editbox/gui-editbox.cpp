#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
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
    static std::string const kFontFace = "ucs-6x13-example";

    class GuiEditbox
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:
        void start() override
        {
            using namespace minire::gui;

            GuiController::start();

            // base container

            auto layout = std::make_shared<layouts::Grid>(2, 2);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;

            Editbox::Sptr disabledEditBox;

            // regular editbox

            {
                // destination

                auto editbox1 = container->emplace<Editbox>("regular-1",
                    [this](minire::text::FormattedString const & v) { return makeTextView(v, kFontFace); });

                editbox1->horizontal()  = Arranger(position::Center{}, dimension::Constant{200});
                editbox1->vertical()    = Arranger(position::Center{}, dimension::Constant{25});

                editbox1->setCallback(std::in_place_type<editbox::OnTextChanged>, "foobar",
                    [](Component const & component, editbox::OnTextChanged const & event)
                    {
                        MINIRE_INFO("A Text was changed for \"{}\": \"{}\"",
                                    component.id(), event._current.unformat());
                    });

                editbox1->setCallback(std::in_place_type<minire::events::application::OnKeyDown>, "keydown",
                    [this](Component const & component, minire::events::application::OnKeyDown const & event)
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
                editbox2->setTextBuilderCallback(
                    [this](minire::text::FormattedString const & v) { return makeTextView(v, kFontFace); });

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

                auto editbox = container->emplace<Editbox>("password",
                    [this](minire::text::FormattedString const & v) { return makeTextView(v, kFontFace); });

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
                auto editbox = container->emplace<Editbox>("disabled",
                    [this](minire::text::FormattedString const & v) { return makeTextView(v, kFontFace); });

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

        minire::Application application(1280, 720, "GUI Editbox", manager);
        application.setController<GuiEditbox>(kMaxCtrlFps);
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
