#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/image.hpp>
#include <minire/sdl/audio-mixer.hpp>

#include <cassert>
#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static float const kVelocity = 150.0f;
    static std::string const kSpriteFile = "tux.png";

    class Archive
        : public minire::Application
    {
        using Application::Application;

        bool handle(minire::application::OnResize const & e) override
        {
            _windowSize.x = e._width;
            _windowSize.y = e._height;

            return Application::handle(e);
        }

        void onStart() override
        {
            Application::onStart();

            auto lease = contentManager().borrow(kSpriteFile);
            assert(lease);
            minire::models::Image::Sptr image = lease->as<minire::models::Image::Sptr>();
            MINIRE_INVARIANT(image, "not a valid image: {}", kSpriteFile);
            _imageSize = glm::vec2(image->_width, image->_height);

            _sprite = make(minire::models::Sprite
            {
                ._image = minire::models::sprite::Image{kSpriteFile, std::monostate()},
                ._position = _position,
                ._dimensions = glm::vec2(0),
                ._clippingWindow = std::nullopt,
                ._zOrder = 0,
                ._visible = true,
            });

            _sfxLease = contentManager().borrow("sounds/on.ogg");
            assert(_sfxLease);
        }

        bool onStep() override
        {
            float const delta = frameTime();

            _position += _direction * delta * kVelocity;
            _position = glm::clamp(_position, glm::vec2{0, 0}, _windowSize - _imageSize);

            assert(_sprite);
            _sprite->setPosition(_position);

            auto audioMixerPool = audioMixer().makePool();

            if (_position.x + _imageSize.x >= _windowSize.x)
            {
                _direction.x = -1.0f;
                beep();
            }
            else if (_position.x <= 0)
            {
                _direction.x = 1.0f;
                beep();
            }

            if (_position.y + _imageSize.y >= _windowSize.y)
            {
                _direction.y = -1.0f;
                beep();
            }
            else if (_position.y <= 0)
            {
                _direction.y = 1.0f;
                beep();
            }

            return true;
        }

        void beep()
        {
            assert(_sfxLease);
            auto clip = _sfxLease->as<minire::formats::AudioClip::Sptr>();

            assert(clip);
            audioMixer().stream(*clip);
        }

    private:
        minire::Sprite::Sptr         _sprite;
        minire::content::Lease::Uptr _sfxLease;
        glm::vec2                    _windowSize{0, 0};
        glm::vec2                    _position{0, 0};
        glm::vec2                    _direction{1, 1};
        glm::vec2                    _imageSize{0, 0};
    };
}

int main(int /*argc*/, char *argv[])
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);
        minire::content::Manager manager;

        auto & physFs = manager.setReader<minire::content::readers::PhysFS>(argv[0]);
        physFs.mount(MINIRE_EXAMPLE_PREFIX "/archive.zip", "", true);

        Archive application(1280, 720, "Archive File", manager);
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
