#include <minire/gui/components/image.hpp>

#include <utils/uuid.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/events/controller.hpp>
#include <minire/logging.hpp>
#include <minire/models/image.hpp>

#include <utils/overloaded.hpp>

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
        std::visit(utils::Overloaded
        {
            [this, &texture](std::monostate const &)
            {
                auto lease = borrow(texture);
                assert(lease);
                models::Image::Sptr image = lease->as<models::Image::Sptr>();
                MINIRE_INVARIANT(image, "not a valid image: {}", texture);
                _width = image->_width;
                _height = image->_height;
                _isResizable = false;
            },

            [this](utils::Rect const & tile)
            {
                _width = tile._right - tile._left + 1;
                _height = tile._bottom - tile._top + 1;
                _isResizable = false;
            },

            [this](utils::NinePatch const & ninePatch)
            {
                glm::vec2 sz = utils::defaultSize(ninePatch);
                _width = sz.x;
                _height = sz.y;
                _isResizable = true;
            },
        }, patch);

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

        enqueue<events::controller::VisibleSprite>(_spriteId, visible());
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
