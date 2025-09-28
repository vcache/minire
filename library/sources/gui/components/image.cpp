#include <minire/gui/components/image.hpp>

#include <utils/uuid.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/events/controller.hpp>
#include <minire/models/image.hpp>

#include <cassert>

namespace minire::gui::components
{
    Image::Image(GuiController & controller,
                 std::string const & id,
                 std::shared_ptr<components::Container> const & parent,
                 content::Id const & texture,
                 utils::MaybeRect const & tile,
                 Position hPos, Position vPos)
        : Component(controller, id, parent)
    {
        if (tile)
        {
            _width = tile->_right - tile->_left + 1;
            _height = tile->_bottom - tile->_top + 1;
        }
        else
        {
            auto lease = borrow(texture);
            assert(lease);
            models::Image::Sptr image = lease->as<models::Image::Sptr>();
            MINIRE_INVARIANT(image, "not a valid image: {}", texture);
            _width = image->_width;
            _height = image->_height;
        }

        setArrangers(Arrangers
        {
            ._horizontal = Arranger(hPos, dimension::Constant{_width}),
            ._vertical = Arranger(vPos, dimension::Constant{_height}),
        });

        // NOTE: setArrangers will call onContentAreaChanged, thus,
        //       spriteId should be empty() at that time.
        _spriteId = utils::newUuid();
        enqueue<events::controller::CreateSprite>(
            _spriteId, texture, tile, glm::vec2(contentArea()._left, contentArea()._top),
            visible(), zOrder());
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

        // TODO: add stretching/shrinking depending on area._width and/or area._height
        Area const & area = contentArea();
        enqueue<events::controller::MoveSprite>(
            _spriteId, glm::vec2(area._left, area._top)
        );
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