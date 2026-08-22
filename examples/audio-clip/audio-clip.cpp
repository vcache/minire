#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/layouts/array.hpp>
#include <minire/logging.hpp>
#include <minire/models/font-face.hpp>
#include <minire/sdl/audio-mixer.hpp>
#include <minire/text/unicode.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <array>
#include <cstdlib> // for EXIT_SUCCESS
#include <string>

using namespace minire::gui;
using namespace minire::gui::components;
using namespace minire::utils;

namespace
{
    static std::string const kFontFace = "ucs-6x13-example";

    class AudioClip
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

    protected:
        auto mkLabel(std::string const & text)
        {
            minire::text::FormattedString result(
                text,
                minire::text::Format().background(glm::vec4(0, 0, 0, 0))
                                      .foreground(glm::vec4(0, 0, 0, 1)));
            return result;
        }

        void onStart() override
        {
            GuiApplication::onStart();

            static const std::array<std::string, 5> kFiles
            {
                "button1.ogg", "button2.ogg", "complete.ogg", "off.ogg", "on.ogg"
            };

            // allocate AudioMixer Pool
            _audioMixerPool = audioMixer().makePool();

            // base layout
            auto sfxPanel = guiRoot().emplace<Component>("sfx");
            auto musicPanel = guiRoot().emplace<Component>("music");

            guiRoot().newLayout<layouts::Column>()->pushBack(sfxPanel, dimension::Fill{})
                                                   .pushBack(musicPanel, dimension::Fill{});

            // SFX panel
            auto sfxLayout = sfxPanel->newLayout<layouts::Row>();
            for(size_t i = 0; i < kFiles.size(); i++)
            {
                auto button = sfxPanel->emplace<Button>(
                    fmt::format("button-{}", i),
                    Button::Mode::kText,
                    options::Text{mkLabel(fmt::format("Play {}", kFiles[i]))},
                    options::FontFace{kFontFace},
                    options::Horizontal{Arranger{position::Center{}, dimension::Content{}}},
                    options::Vertical{Arranger{position::Center{}, dimension::Content{}}});

                sfxLayout->pushBack(button, dimension::Fill{});

                auto lease = contentManager().borrow(fmt::format("sounds/{}", kFiles[i]));
                assert(lease);
                auto clip = lease->as<minire::formats::AudioClip::Sptr>();

                button->setCallback(std::in_place_type<OnClick>, "foo",
                    [clip, i, this](Component &, OnClick const &)
                    {
                        assert(clip);
                        bool success = _audioMixerPool->play(*clip);
                        MINIRE_INFO("play \"{}\": {}", kFiles[i], success);
                    });
            }

            // Music panel
            {
                auto button = musicPanel->emplace<Button>(
                    "play",
                    Button::Mode::kText,
                    options::Text{mkLabel("Stream music")},
                    options::FontFace{kFontFace},
                    options::Horizontal{Arranger{position::Center{}, dimension::Content{}}},
                    options::Vertical{Arranger{position::Center{}, dimension::Content{}}});


                auto lease = contentManager().borrow("music/ObservingTheStar.ogg");
                assert(lease);
                auto clip = lease->as<minire::formats::AudioClip::Sptr>();

                button->setCallback(std::in_place_type<OnClick>, "foo",
                    [clip, this](Component &, OnClick const &)
                    {
                        MINIRE_INFO("streaming music");
                        assert(clip);
                        audioMixer().stream(*clip);
                    });
            }
        }

    private:
        minire::sdl::AudioMixer::Pool::Sptr _audioMixerPool;
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

        auto lease = manager.upload(kFontFace, minire::models::FontFace
            {
                ._regular = "../common/6x13.bdf",
                ._bold = "../common/6x13B.bdf",
                ._italic = "../common/6x13O.bdf",
                ._glyphWidth = 6,
                ._glyphHeight = 13,
            });

        AudioClip application(1280, 720, "Audio Clip", manager,
                              minire::models::MsaaParams(),
                              minire::models::MixerParams{._flags = MIX_INIT_OGG});
        application.setVsync(true);
        application.setGlDebug(false);

        // Main loop
        application.run();
        lease.reset(); // TODO: dirty hack to make Manager::clear() happy

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
