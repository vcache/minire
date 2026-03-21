#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/gui/models/exclusive-group.hpp>
#include <minire/logging.hpp>
#include <minire/models/font-face.hpp>

#include <fmt/format.h>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static std::string const kAtlas = "gui-atlas.png";
    static std::string const kFontFace = "ucs-6x13-example";

    class GuiButton
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

    protected:
        void onStart() override
        {
            GuiApplication::onStart();

            using namespace minire::gui::components;
            using namespace minire::gui;
            using namespace minire::utils;

            // create test cases

            std::vector<std::pair<Arranger, Arranger>> const kArrangers
            {
                {
                    Arranger(position::Center{}, dimension::Constant{0}),
                    Arranger(position::Center{}, dimension::Constant{0}),
                },
                {
                    Arranger(position::Center{}, dimension::Constant{75}),
                    Arranger(position::Center{}, dimension::Constant{45}),
                },
                {
                    Arranger(position::Center{}, dimension::Constant{45}),
                    Arranger(position::Center{}, dimension::Constant{75}),
                },
                {
                    Arranger(position::Center{}, dimension::Fraction{.5f}),
                    Arranger(position::Center{}, dimension::Fraction{.5f}),
                },
                {
                    Arranger(position::Center{}, dimension::Fill{}),
                    Arranger(position::Center{}, dimension::Fill{}),
                },
                {
                    Arranger(position::Center{}, dimension::Content{}),
                    Arranger(position::Center{}, dimension::Content{}),
                },
                {
                    Arranger(position::Center{}, dimension::Fraction{.5}),
                    Arranger(position::Center{}, dimension::Fraction{.5}),
                },
                {
                    Arranger(position::Center{}, dimension::Fraction{.5}),
                    Arranger(position::Center{}, dimension::Fraction{.5}),
                },
                {
                    Arranger(position::Center{}, dimension::Fraction{.5}),
                    Arranger(position::Center{}, dimension::Fraction{.5}),
                },
            };

            // build grid container
            auto layout = std::make_shared<layouts::Grid>(3 + 4*2, kArrangers.size());
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;

            for(size_t row = 0; row < layout->rows(); row++)
            {
                for(size_t col = 0; col < layout->cols(); col++)
                {
                    auto subContainer = container->emplace<Component>(
                        fmt::format("{}x{}", row, col));

                    auto bg = subContainer->emplace<Image>(
                        fmt::format("bg-{}x{}", row, col),
                        minire::models::sprite::Image(kAtlas,
                            NinePatch
                            {
                                ._boundary = Rect(30, 36, 51, 57),
                                ._out = Rect(33, 39, 48, 54),
                                ._in = Rect(36, 42, 45, 51),
                            }));
                    bg->horizontal()->_dimension = dimension::Fill{};
                    bg->vertical()->_dimension = dimension::Fill{};

                    layout->set(row, col, subContainer->id());
                }
            }

            // create buttons

            minire::text::FormattedString caption(
                "New",
                minire::text::Format().background(glm::vec4(0, 0, 0, 0))
                                      .foreground(glm::vec4(0, 0, 0, 1)));

            Rect const kIconRect(46, 7, 56, 19);

            auto exclGroup1 = std::make_shared<models::ExclusiveGroup>(false);
            auto exclGroup2 = std::make_shared<models::ExclusiveGroup>(true);

            exclGroup1->setCallback(std::in_place_type<models::exclusive_group::OnChange>, "foo",
                [](models::ExclusiveGroup const &, models::exclusive_group::OnChange const & e)
                {
                    MINIRE_INFO("ExclusiveGroup 1 selection changed from {} to {}",
                                (void const *)e._previous, (void const *)e._current);
                });

            exclGroup2->setCallback(std::in_place_type<models::exclusive_group::OnChange>, "foo",
                [](models::ExclusiveGroup const &, models::exclusive_group::OnChange const & e)
                {
                    MINIRE_INFO("ExclusiveGroup 2 selection changed from {} to {}",
                                (void const *)e._previous, (void const *)e._current);
                });

            for(size_t i = 0; i < kArrangers.size(); ++i)
            {
                size_t row = 0;

                bool const isCheckable = i == 6 || i == 7 || i == 8;
                auto exclGroup = i == 7 ? exclGroup1 : (i == 8 ? exclGroup2 : nullptr);

                // empty content
                {
                    std::string cellId = fmt::format("{}x{}", row++, i);
                    auto button = container->at<Component>(cellId).emplace<Button>("btn", false, false);
                    button->horizontal() = kArrangers[i].first;
                    button->vertical() = kArrangers[i].second;
                    button->setCheckable(isCheckable);

                    button->setCallback(std::in_place_type<OnClick>, "foo",
                                        [cellId](Component &, OnClick const &)
                                        { MINIRE_INFO("Clicked at {}", cellId); } );

                    button->setCallback(std::in_place_type<models::checkable::OnCheckedChanged>, "foo",
                        [cellId](models::Checkable & c, models::checkable::OnCheckedChanged const & e)
                        { MINIRE_INFO("Check at {}: {}, {}", cellId, c.checked(), e._checked); } );

                    button->setExclusiveGroup(exclGroup);
                    _buttons.push_back(button);
                }

                // icon only
                {
                    std::string cellId = fmt::format("{}x{}", row++, i);
                    auto button = container->at<Component>(cellId).emplace<Button>("btn", false, true);
                    button->horizontal() = kArrangers[i].first;
                    button->vertical() = kArrangers[i].second;
                    button->icon() = minire::models::sprite::Image(kAtlas, kIconRect);
                    button->setCheckable(isCheckable);

                    button->setCallback(std::in_place_type<OnClick>, "foo",
                                        [cellId](Component &, OnClick const &)
                                        { MINIRE_INFO("Clicked at {}", cellId); } );

                    button->setCallback(std::in_place_type<models::checkable::OnCheckedChanged>, "foo",
                        [cellId](models::Checkable & c, models::checkable::OnCheckedChanged const & e)
                        { MINIRE_INFO("Check at {}: {}, {}", cellId, c.checked(), e._checked); } );

                    button->setExclusiveGroup(exclGroup);
                    _buttons.push_back(button);
                }

                // text only
                {
                    std::string cellId = fmt::format("{}x{}", row++, i);
                    auto button = container->at<Component>(cellId).emplace<Button>("btn", true, false);
                    button->horizontal() = kArrangers[i].first;
                    button->vertical() = kArrangers[i].second;
                    button->text() = caption;
                    button->fontFace() = kFontFace;
                    button->setCheckable(isCheckable);

                    button->setCallback(std::in_place_type<OnClick>, "foo",
                                        [cellId](Component &, OnClick const &)
                                        { MINIRE_INFO("Clicked at {}", cellId); } );

                    button->setCallback(std::in_place_type<models::checkable::OnCheckedChanged>, "foo",
                        [cellId](models::Checkable & c, models::checkable::OnCheckedChanged const & e)
                        { MINIRE_INFO("Check at {}: {}, {}", cellId, c.checked(), e._checked); } );

                    button->setExclusiveGroup(exclGroup);
                    _buttons.push_back(button);
                }

                // icon + text
                for(Theme::Location location : { Theme::Location::kLeft,
                                                 Theme::Location::kTop,
                                                 Theme::Location::kRight,
                                                 Theme::Location::kBottom})
                {
                    for(float const spacing : {0.0f, 10.0f})
                    {
                        std::string cellId = fmt::format("{}x{}", row++, i);
                        auto button = container->at<Component>(cellId).emplace<Button>("btn",
                            Button::Mode::kBoth,
                            options::Text{caption},
                            options::FontFace{kFontFace},
                            options::IconLocation{location},
                            options::IconSpacing{spacing},
                            options::Icon{minire::models::sprite::Image(kAtlas, kIconRect)},
                            options::Horizontal{kArrangers[i].first},
                            options::Vertical{kArrangers[i].second},
                            options::Checkable{isCheckable},
                            options::ExclusiveGroup{exclGroup});

                        button->setCallback(std::in_place_type<OnClick>, "foo",
                                            [cellId](Component &, OnClick const &)
                                            { MINIRE_INFO("Clicked at {}", cellId); } );

                        button->setCallback(std::in_place_type<models::checkable::OnCheckedChanged>, "foo",
                            [cellId](models::Checkable & c, models::checkable::OnCheckedChanged const & e)
                            { MINIRE_INFO("Check at {}: {}, {}", cellId, c.checked(), e._checked); } );

                        _buttons.push_back(button);
                    }
                }
            }

            // set hot keys

            set(SDL_SCANCODE_TAB, 0, [this](::SDL_Scancode, uint16_t) -> bool
            {
                for(auto const & button : _buttons)
                {
                    if (button->padding()->_left == 0)
                    {
                        button->padding() = minire::utils::Rect(10.0f);
                    }
                    else
                    {
                        button->padding() = minire::utils::Rect(0.0f);
                    }
                }
                return true;
            });
        }

    private:
        std::vector<minire::gui::components::Button::Sptr> _buttons;
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

        GuiButton application(1280, 720, "GUI Button", manager);
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
