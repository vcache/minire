#pragma once

#include <minire/content/id.hpp>
#include <minire/gui/component.hpp>
#include <minire/sprite.hpp>
#include <minire/utils/rect.hpp>

namespace minire::gui::components
{
    class Image // TODO: Picture (Image is too broad)
        : public Component
    {
    public:
        using Sptr = std::shared_ptr<Image>;
        using Wptr = std::weak_ptr<Image>;

        static constexpr std::string kName = "Image";

        explicit Image(std::string const & id,
                       Theme const & theme,
                       Theme::Style const & style,
                       OverlayController & overlayController,
                       models::sprite::MaybeImage image);

        ~Image() override;

        Property<models::sprite::MaybeImage> const & image() const { return _image; }
        Property<models::sprite::MaybeImage> & image() { return _image; }

        std::optional<glm::vec2> measureContent() const override;

    protected:
        void initialize() override;
        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

        void dropSprite();

    private:
        Property<models::sprite::MaybeImage> _image;
        Sprite::Sptr                         _sprite;
        mutable std::optional<glm::vec2>     _contentSize;
    };
}
