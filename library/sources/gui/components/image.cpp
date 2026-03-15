#include <minire/gui/components/image.hpp>

#include <minire/gui/overlay-controller.hpp>
#include <minire/models/sprite.hpp>

#include <minire/logging.hpp> // TODO: [X]

#include <cassert>

namespace minire::gui::components
{
    Image::Image(std::string const & id,
                 Theme const & theme,
                 Theme::Style const & style,
                 OverlayController & overlayController,
                 models::sprite::MaybeImage image)
        : Component(id, theme, style, overlayController)
        , _image(*this, std::move(image))
    {}

    Image::~Image()
    {
        dropSprite();
    }

    void Image::dropSprite()
    {
        if (_sprite)
        {
            _sprite->detach();
            _sprite.reset();
        }
    }

    void Image::initialize()
    {
        if (_image.get())
        {
            if (auto [_, isResizable] = overlayController().measure(*_image.get());
                !isResizable)
            {
                horizontal()->_dimension = dimension::Content{};
                vertical()->_dimension = dimension::Content{};
            }
        }
    }

    std::optional<glm::vec2> Image::measureContent() const
    {
        if (!_contentSize || _image.isInvalidated())
        {
            if (_image.get())
            {
                auto [size, _] = overlayController().measure(*_image.get());
                _contentSize = size;
            }
            else
            {
                _contentSize = glm::vec2(0);
            }
        }
        return *_contentSize;
    }

    size_t Image::revalidateContent(size_t zOffset,
                                    bool const effectiveVisible,
                                    Area const & contentArea,
                                    Area const & clippingWindow)
    {
        // maybe drop a sprite if any
        if (!_image.get())
        {
            dropSprite();
            return zOffset + 1;
        }

        // there is an Image => create or update a Sprite
        glm::vec2 const newPosition(contentArea._left, contentArea._top);
        glm::vec2 const newDimensions(contentArea._width, contentArea._height);
        utils::Rect const newClippingWindow(
            clippingWindow._left,
            clippingWindow._top,
            clippingWindow._left + clippingWindow._width,     // TODO: +1 ?
            clippingWindow._top + clippingWindow._height);    // TODO: +1 ?

        if (!_sprite)
        {
            _sprite = overlayController().create(models::Sprite
            {
                ._image = *_image.get(),
                ._position = newPosition,
                ._dimensions = newDimensions,
                ._clippingWindow = newClippingWindow,
                ._zOrder = zOffset,
                ._visible = effectiveVisible,
            });
        }
        else
        {
            // TODO: don't update these parameters while in an invisible state

            if (_image.isInvalidated())
            {
                _sprite->setImage(*_image.get());
                invalidateContent();
                _contentSize.reset();
            }

            _sprite->setPosition(newPosition);
            _sprite->setDimensions(newDimensions);
            _sprite->setClippingWindow(newClippingWindow);
            _sprite->setZOrder(zOffset);
            _sprite->setVisible(effectiveVisible);
        }

        _image.revalidate();

        return zOffset + 1;
    }
}