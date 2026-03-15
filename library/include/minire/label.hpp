#pragma once

#include <minire/errors.hpp>
#include <minire/models/label.hpp>
#include <minire/utils/object.hpp>

// TODO: maybe minire::scene?
namespace minire
{
    class Label
        : public utils::Object<Label, models::Label>
    {
    protected:
        static constexpr size_t kText           = mkMask(0);
        static constexpr size_t kFontFace       = mkMask(1);
        static constexpr size_t kPosition       = mkMask(2);
        static constexpr size_t kClippingWindow = mkMask(3);
        static constexpr size_t kZOrder         = mkMask(4);
        static constexpr size_t kVisible        = mkMask(5);

    public:
        using Object::Object;

        text::FormattedString const & text() const { return model()._text; }
        void setText(text::FormattedString text)
        {
            if (this->text() != text)
            {
                model(kText)._text = std::move(text);
            }
        }

        content::Id const & fontFace() const { return model()._fontFace; }
        void setFontFace(content::Id fontFace)
        {
            if (this->fontFace() != fontFace)
            {
                model(kFontFace)._fontFace = std::move(fontFace);
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

        utils::MaybeRect const & clippingWindow() const { return model()._clippingWindow; }
        void setClippingWindow(utils::MaybeRect clippingWindow)
        {
            if (this->clippingWindow() != clippingWindow)
            {
                model(kClippingWindow)._clippingWindow = std::move(clippingWindow);
            }
        }

        // TODO: zOrder and visible aren't model data, they are more like instance data (as Node/Leaf in Scene)
        //       Maybe just use base model like ScreenModel <- Model2D <- (Label, Sprite)
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
