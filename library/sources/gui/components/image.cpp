#include <minire/gui/components/image.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/events/controller.hpp>
#include <minire/logging.hpp>
#include <minire/models/image.hpp>

#include <utils/uuid.hpp>

#include <cassert>
#include <variant>

namespace minire::gui::components
{
    Image::Image(GuiController & controller,
                 std::string const & id,
                 std::shared_ptr<components::Container> const & parent,
                 content::Id const & texture,
                 utils::Patch const & patch,
                 Arrangers arrangers)
        : Component(controller, id, parent)
    {
        auto [size, resizeable] = measure(patch, texture);
        _width = size.x;
        _height = size.y;
        _isResizable = resizeable;

        if (!_isResizable)
        {
            if (!std::holds_alternative<dimension::Content>(arrangers._horizontal.dimension()))
            {
                MINIRE_WARNING("image \"{}\" isn't resizable, "
                               "its horizontal arranger must be Content", id);
                arrangers._horizontal.setDimension(dimension::Content{});
            }

            if (!std::holds_alternative<dimension::Content>(arrangers._vertical.dimension()))
            {
                MINIRE_WARNING("image \"{}\" isn't resizable, "
                               "its vertical arranger must be Content", id);
                arrangers._vertical.setDimension(dimension::Content{});
            }
        }

        assert(_spriteId.empty());
        setArrangers(arrangers);

        // NOTE: setArrangers will call onContentAreaChanged, thus,
        //       spriteId should be empty() at that time.
        _spriteId = utils::newUuid();
        Area const & area = contentArea();
        enqueue<events::controller::CreateSprite>(
            _spriteId, texture, patch, glm::vec2(area._left, area._top),
            glm::vec2(area._width, area._height), visible(), zOrder());
    }

    Image::~Image()
    {
        enqueue<events::controller::RemoveSprite>(_spriteId);
    }

    void Image::onVisibleChanged()
    {
        if (_spriteId.empty())
            return;

        enqueue<events::controller::SetSpriteVisible>(_spriteId, visible());
    }

    void Image::onContentAreaChanged()
    {
        if (_spriteId.empty())
            return;

        Area const & area = contentArea();

        enqueue<events::controller::MoveSprite>(
            _spriteId, glm::vec2(area._left, area._top)
        );

        if (_isResizable)
        {
            enqueue<events::controller::ResizeSprite>(
                _spriteId, glm::vec2(area._width, area._height)
            );
        }
    }

    size_t Image::onZOrderChanged(size_t offset, ZOrderUpdates &,
                                  ZOrderUpdates & sprites)
    {
        MINIRE_INVARIANT(!_spriteId.empty(), "_spriteId isn't set");
        sprites.emplace_back(_spriteId, offset);
        return offset + 1;
    }

    std::optional<std::pair<float, float>> Image::measureContent() const
    {
        return std::make_pair(_width, _height);
    }
}
