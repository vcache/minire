#pragma once

#include <minire/gui/content-view.hpp>

#include <optional>

namespace minire { class GuiController; }

namespace minire::gui_controller
{
    // TODO: Add various resize modes: kNonResizable, kStretch, kRepeat, etc
    class ImageViewImpl
        : public gui::ImageView
    {
    public:
        ImageViewImpl(content::Id const & textureId,
                      utils::Patch const & patch,
                      GuiController & controller);

        ~ImageViewImpl() override;

        void initialize() override;

        std::pair<glm::vec2, bool> measure() const override;

        void setContentPosition(float left, float top) override;

        void setContentSize(float width, float height) override;

        void setContentArea(gui::Area const & area) override;

        void setClippingWindow(gui::MaybeArea const &) override;

        size_t onZOrderChanged(size_t zOffset) override;

        void setVisible(bool visible) override;

        void setContent(content::Id const & textureId,
                        utils::Patch const & patch) override;

        void setPatch(utils::Patch const & patch) override;

        void setTexture(content::Id const & textureId) override;

        void commit() override;

    private:
        void enqueueUncommited();

    private:
        GuiController                 & _controller;

        // current state
        std::string                     _spriteId;
        content::Id                     _textureId;
        utils::Patch                    _patch;
        glm::vec2                       _spritePosition{0, 0};
        glm::vec2                       _spriteSize{0, 0};
        glm::vec2                       _imageSize{0, 0};
        size_t                          _zOrder = 0;
        utils::MaybeRect                _clippingWindow = std::nullopt;
        bool                            _resizable = false;
        bool                            _visible = true;

        // pended updates
        std::optional<utils::Patch>     _newPatch;
        std::optional<content::Id>      _newTextureId;
        std::optional<glm::vec2>        _newSpritePosition;
        std::optional<glm::vec2>        _newSpriteSize;
        std::optional<utils::MaybeRect> _newClippingWindow;
        std::optional<size_t>           _newZOrder;
        std::optional<bool>             _newVisible;
    };
}
