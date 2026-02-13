#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/image.hpp>

#include <fmt/format.h>

#include <cassert>
#include <cmath>
#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static size_t const kMaxCtrlFps = 120;

    class LabelsUsage
        : public minire::BasicController
    {
        using BasicController::BasicController;

        struct LabelData
        {
            std::string _id;
            glm::vec2   _position{0, 0};
            glm::vec2   _size{0, 0};
        };

        LabelData makeLabel(minire::text::FormattedString const & text)
        {
            static size_t kCount = 0;
            LabelData result
            {
                ._id = fmt::format("label-{}", kCount++),
                ._position = glm::vec2(0),
                ._size = measure(text, "ucs-6x13-example"),
            };
            enqueue<minire::events::controller::CreateLabel>(
                result._id, text, "ucs-6x13-example", result._position, true, 0);
            return result;
        }

    protected:

        void handle(minire::events::application::OnResize const & onResize) override
        {
            _windowSize.x = onResize._width;
            _windowSize.y = onResize._height;

            _centeral._position = (_windowSize - _centeral._size) / 2.0f;
            enqueue<minire::events::controller::MoveLabel>(_centeral._id,
                                                           _centeral._position);

            _rightBottom._position = _windowSize - _rightBottom._size - glm::vec2(1);
            enqueue<minire::events::controller::MoveLabel>(_rightBottom._id,
                                                           _rightBottom._position);

            _clipping._position.x = (_windowSize.x - _clipping._size.x) / 2.0f;
            _clipping._position.y = 0;
            enqueue<minire::events::controller::MoveLabel>(_clipping._id,
                                                           _clipping._position);
        }

        void step()
        {
            minire::utils::Rect clipping(
                _clipping._position.x,
                _clipping._position.y,
                _clipping._position.x + _clipping._size.x * (1.0 + std::cos(_clipPhase)) / 2.0f,
                _clipping._position.y + _clipping._size.y * (1.0 + std::cos(_clipPhase / 2.0f)) / 2.0f);
            enqueue<minire::events::controller::SetLabelClippingWindow>(
                _clipping._id, clipping);
            _clipPhase += frameTime();
        }

        void start() override
        {
            _leftTop = makeLabel(L"<- Hello world");
            _rightBottom = makeLabel(L"Hello world ->");
            _centeral = makeLabel(
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
                _clipping = makeLabel(loremIpsum);
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
        minire::Application application(1280, 720, "Labels Usage", manager);
        application.setController<LabelsUsage>(kMaxCtrlFps);
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
