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

        std::pair<float, float> measure() const override;

        void setContentPosition(float left, float top) override;

        void setContentSize(float width, float height) override;

        void setContentArea(gui::Area const & area) override;

        size_t onZOrderChanged(size_t zOffset) override;

        void setVisible(bool visible) override;

        void commit() override;

    private:
        void enqueueUncommited();

    private:
        GuiController          & _controller;
        content::Id const        _textureId;
        utils::Patch const       _patch;

        // current state
        std::string              _spriteId;
        glm::vec2                _spritePosition{0, 0};
        glm::vec2                _spriteSize{0, 0};
        glm::vec2                _imageSize{0, 0};
        size_t                   _zOrder = 0;
        bool                     _resizable = false;
        bool                     _visible = true;

        // pended updates
        std::optional<glm::vec2> _newSpritePosition;
        std::optional<glm::vec2> _newSpriteSize;
        std::optional<size_t>    _newZOrder;
        std::optional<bool>      _newVisible;
    };
}
