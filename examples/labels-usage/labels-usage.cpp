#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/image.hpp>

#include <fmt/format.h>

#include <cassert>
#include <cmath>
#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class LabelsUsage
        : public minire::Application
    {
        using Application::Application;

        struct LabelData
        {
            minire::Label::Sptr _label;
            glm::vec2           _size{0, 0};
        };

        LabelData buildLabel(minire::text::FormattedString const & text)
        {
            minire::Label::Sptr label = makeLabel(minire::models::Label
            {
                ._text = text,
                ._fontFace = "ucs-6x13-example",
                ._position = glm::vec2(0),
                ._clippingWindow = std::nullopt,
                ._zOrder = 0,
                ._visible = true,
            });

            return LabelData
            {
                ._label = label,
                ._size = measure(text, label->fontFace()),
            };
        }

    protected:
        bool handle(minire::application::OnResize const & e) override
        {
            if (Application::handle(e))
                return true;

            _windowSize.x = e._width;
            _windowSize.y = e._height;

            _centeral._label->setPosition((_windowSize - _centeral._size) / 2.0f);
            _rightBottom._label->setPosition(_windowSize - _rightBottom._size - glm::vec2(1));
            _clipping._label->setPosition(glm::vec2{(_windowSize.x - _clipping._size.x) / 2.0f, 0.0f});

            return true;
        }

        bool onStep() override
        {
            minire::utils::Rect clipping(
                _clipping._label->position().x,
                _clipping._label->position().y,
                _clipping._label->position().x + _clipping._size.x * (1.0 + std::cos(_clipPhase)) / 2.0f,
                _clipping._label->position().y + _clipping._size.y * (1.0 + std::cos(_clipPhase / 2.0f)) / 2.0f);
            _clipping._label->setClippingWindow(clipping);
            _clipPhase += frameTime();

            return true;
        }

        void onStart() override
        {
            Application::onStart();

            _leftTop = buildLabel(L"<- Hello world");
            _rightBottom = buildLabel(L"Hello world ->");
            _centeral = buildLabel(
                L"        /\\       \n"
                L"<- Hello world ->\n"
                L"        \\/       \n"
            );

            {
                minire::text::FormattedString loremIpsum;
                loremIpsum.append("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod\n");
                loremIpsum.append("tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim\n",
                                  minire::text::Format().bold(true));
                loremIpsum.append("veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea\n",
                                  minire::text::Format().italic(true));
                loremIpsum.append("commodo consequat. Duis aute irure dolor in reprehenderit in voluptate\n",
                                  minire::text::Format().underline(true));
                loremIpsum.append("velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat\n",
                                  minire::text::Format().strikeout(true));
                loremIpsum.append("cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est\n",
                                  minire::text::Format().invertColors(true));
                loremIpsum.append("laborum.");
                _clipping = buildLabel(loremIpsum);
            }
        }

    private:
        LabelData _leftTop;
        LabelData _rightBottom;
        LabelData _centeral;
        LabelData _clipping;
        glm::vec2 _windowSize{0, 0};
        double    _clipPhase = 0;
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
        auto lease = manager.upload("ucs-6x13-example", minire::models::FontFace
            {
                ._regular = "../common/6x13.bdf",
                ._bold = "../common/6x13B.bdf",
                ._italic = "../common/6x13O.bdf",
                ._glyphWidth = 6,
                ._glyphHeight = 13,
            });
        LabelsUsage application(1280, 720, "Labels Usage", manager);
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
