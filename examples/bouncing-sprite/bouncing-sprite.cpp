#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/image.hpp>

#include <cassert>
#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static size_t const kMaxCtrlFps = 120;
    static float const kVelocity = 150.0f;
    static std::string const kSpriteId = "my-sprite";
    static std::string const kSpriteFile = "tux.png";

    class BouncingSprite
        : public minire::BasicController
    {
        using BasicController::BasicController;

        void handle(minire::events::application::OnResize const & onResize) override
        {
            _windowSize.x = onResize._width;
            _windowSize.y = onResize._height;
        }

        void start() override
        {
            auto lease = borrow(kSpriteFile);
            assert(lease);
            minire::models::Image::Sptr image = lease->as<minire::models::Image::Sptr>();
            MINIRE_INVARIANT(image, "not a valid image: {}", kSpriteFile);
            _imageSize = glm::vec2(image->_width, image->_height);

            enqueue<minire::events::controller::CreateSprite>(
                kSpriteId, kSpriteFile, std::monostate(), _position, glm::vec2(0), true, 0);
        }

        void step() override
        {
            float const delta = frameTime();

            _position += _direction * delta * kVelocity;
            _position = glm::clamp(_position, glm::vec2{0, 0}, _windowSize - _imageSize);

            enqueue<minire::events::controller::MoveSprite>(
                kSpriteId, _position);

            if (_position.x + _imageSize.x >= _windowSize.x)
            {
                _direction.x = -1.0f;
            }
            else if (_position.x <= 0)
            {
                _direction.x = 1.0f;
            }

            if (_position.y + _imageSize.y >= _windowSize.y)
            {
                _direction.y = -1.0f;
            }
            else if (_position.y <= 0)
            {
                _direction.y = 1.0f;
            }
        }

    private:
        glm::vec2 _windowSize{0, 0};
        glm::vec2 _position{0, 0};
        glm::vec2 _direction{1, 1};
        glm::vec2 _imageSize{0, 0};
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
        minire::Application application(1280, 720, "Bouncing sprite", manager);
        application.setController<BouncingSprite>(kMaxCtrlFps);
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
