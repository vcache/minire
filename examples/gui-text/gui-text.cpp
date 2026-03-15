#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/text.hpp>
#include <minire/logging.hpp>
#include <minire/text/formatted-string.hpp>

#include <fmt/format.h>

#include <cassert>
#include <cmath>
#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static std::string const kFontFaceId = "u_vga16-example";

    class GuiText
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

        void onStart() override
        {
            GuiApplication::onStart();

            minire::text::FormattedString loremIpsum;
            loremIpsum.append("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod\n",
                              minire::text::Format().foreground(glm::vec4(0, 0, 1, 1)));
            loremIpsum.append("tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim\n",
                              minire::text::Format().foreground(glm::vec4(0, 1, 0, 1)));
            loremIpsum.append("veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea\n",
                              minire::text::Format().foreground(glm::vec4(0, 1, 1, 1)));
            loremIpsum.append("commodo consequat. Duis aute irure dolor in reprehenderit in voluptate\n",
                              minire::text::Format().foreground(glm::vec4(1, 0, 0, 1)));
            loremIpsum.append("velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat\n",
                              minire::text::Format().foreground(glm::vec4(1, 0, 1, 1)));
            loremIpsum.append("cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est\n",
                              minire::text::Format().foreground(glm::vec4(1, 1, 0, 1)));
            loremIpsum.append("laborum.", minire::text::Format().foreground(glm::vec4(1, 1, 1, 1)));

            glm::vec2 size = measure(loremIpsum, kFontFaceId);

            {
                auto comp = guiRoot().emplace<minire::gui::components::Text>(
                    "content-size", loremIpsum, kFontFaceId);
                comp->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                           minire::gui::dimension::Content{});
                comp->vertical() = minire::gui::Arranger(minire::gui::position::Center{},
                                                         minire::gui::dimension::Content{});
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Text>(
                    "content-clipping-up", loremIpsum, kFontFaceId);
                comp->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                           minire::gui::dimension::Constant{size.x / 2});
                comp->vertical() = minire::gui::Arranger(minire::gui::position::Begin{},
                                                         minire::gui::dimension::Constant{size.y / 2});
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Text>(
                    "content-clipping-down", loremIpsum, kFontFaceId);
                comp->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                           minire::gui::dimension::Constant{size.x / 2});
                comp->vertical() = minire::gui::Arranger(minire::gui::position::End{},
                                                         minire::gui::dimension::Constant{size.y / 2});
            }

            _animatedText = guiRoot().emplace<minire::gui::components::Text>(
                "animated-text", "adsfsadf", kFontFaceId);
            _animatedText->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                                minire::gui::dimension::Content{});
            _animatedText->vertical() = minire::gui::Arranger(minire::gui::position::Begin{},
                                                              minire::gui::dimension::Content{}, 200);
        }

        bool onStep() override
        {
            size_t barSize = std::lround(20.0 * (1.0 + std::sin(_phase)));

            if (barSize != _barSize && barSize > 0)
            {
                minire::text::FormattedString str;
                str.append(std::wstring(barSize, L'='));
                _animatedText->text() = str;
                _barSize = barSize;
            }

            _phase += frameTime();

            return GuiApplication::onStep();
        }

    private:
        minire::gui::components::Text::Sptr _animatedText;
        double                              _phase = 0;
        size_t                              _barSize = 0;
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
        auto lease = manager.upload(kFontFaceId, minire::models::FontFace
            {
                ._regular = "u_vga16.bdf",
                ._bold = "u_vga16.bdf",
                ._italic = "u_vga16.bdf",
                ._glyphWidth = 8,
                ._glyphHeight = 16,
            });
        GuiText application(1280, 720, "GUI Text", manager);
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
