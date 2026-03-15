#pragma once

#include <minire/errors.hpp>
#include <minire/models/sprite.hpp>
#include <minire/utils/object.hpp>

// TODO: maybe minire::scene?
namespace minire
{
    class Sprite
        : public utils::Object<Sprite, models::Sprite>
        , public utils::Detachable
    {
    protected:
        static constexpr size_t kTexture        = mkMask(0);
        static constexpr size_t kPatch          = mkMask(1);
        static constexpr size_t kPosition       = mkMask(2);
        static constexpr size_t kDimensions     = mkMask(3);
        static constexpr size_t kClippingWindow = mkMask(4);
        static constexpr size_t kZOrder         = mkMask(5);
        static constexpr size_t kVisible        = mkMask(6);

    public:
        using Object::Object;

        models::sprite::Image const & image() const { return model()._image; }
        void setImage(models::sprite::Image const & image)
        {
            if (this->image()._texture != image._texture)
            {
                model(kTexture)._image._texture = std::move(image._texture);
            }

            if (this->image()._patch != image._patch)
            {
                model(kPatch)._image._patch = std::move(image._patch);
            }
        }

        glm::vec2 position() const { return model()._position; }
        void setPosition(glm::vec2 position)
        {
            if (this->position() != position)
            {
                model(kPosition)._position = position;
            }
        }

        // NOTE: it will be used ONLY for a NinePatch,
        //       and will be ignored in all other cases.
        // TODO: support for a Rect
        glm::vec2 dimensions() const { return model()._dimensions; }
        void setDimensions(glm::vec2 dimensions)
        {
            if (this->dimensions() != dimensions)
            {
                MINIRE_INVARIANT(!std::holds_alternative<utils::Rect>(image()._patch),
                                 "should not set dimensions for a sprite!");
                model(kDimensions)._dimensions = dimensions;
            }
        }

        utils::MaybeRect const & clippingWindow() const { return model()._clippingWindow; }
        void setClippingWindow(utils::MaybeRect clippingWindow)
        {
            if (this->clippingWindow() != clippingWindow)
            {
                model(kClippingWindow)._clippingWindow = std::move(clippingWindow);
            }
        }

        // TODO: zOrder and visible aren't model data, they are more like instance data (as Node/Leaf in Scene)
        size_t zOrder() const { return model()._zOrder; }
        void setZOrder(size_t zOrder)
        {
            if (this->zOrder() != zOrder)
            {
                model(kZOrder)._zOrder = zOrder;
            }
        }

        bool visible() const { return model()._visible; }
        void setVisible(bool visible)
        {
            if (this->visible() != visible)
            {
                model(kVisible)._visible = visible;
            }
        }
    };
}
