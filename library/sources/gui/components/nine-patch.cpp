#include <minire/gui/components/nine-patch.hpp>

#include <utils/uuid.hpp>

#include <minire/basic-controller.hpp>
#include <minire/errors.hpp>
#include <minire/events/controller.hpp>

#include <cassert>

namespace minire::gui::components
{
    NinePatchImage::NinePatchImage(GuiController & controller,
                                   std::string const & id,
                                   std::shared_ptr<components::Container> const & parent,
                                   content::Id const & texture,
                                   utils::NinePatch const & tile,
                                   Arrangers arrangers)
        : Component(controller, id, parent)
    {
        // forcing rearrange to calculate contentArea()
        assert(_spriteId.empty());
        setArrangers(arrangers);

        // NOTE: setArrangers will call onContentAreaChanged, thus,
        //       spriteId should be empty() at that time.
        _spriteId = utils::newUuid();
        enqueue<events::controller::CreateNinePatch>(
            _spriteId, texture, tile,
            glm::vec2(contentArea()._left, contentArea()._top),
            glm::vec2(contentArea()._width, contentArea()._height),
            visible(), zOrder());
    }

    NinePatchImage::~NinePatchImage()
    {
        enqueue<events::controller::RemoveSprite>(_spriteId);
    }

    void NinePatchImage::onVisibleChanged()
    {
        if (_spriteId.empty())
            return;

        enqueue<events::controller::VisibleSprite>(_spriteId, visible());
    }

    void NinePatchImage::onContentAreaChanged()
    {
        if (_spriteId.empty())
            return;

        Area const & area = contentArea();

        enqueue<events::controller::MoveSprite>(
            _spriteId, glm::vec2(area._left, area._top)
        );

        enqueue<events::controller::ResizeNinePatch>(
            _spriteId, glm::vec2(area._width, area._height)
        );
    }

    size_t NinePatchImage::onZOrderChanged(size_t offset, ZOrderUpdates &,
                                           ZOrderUpdates & sprites)
    {
        MINIRE_INVARIANT(!_spriteId.empty(), "_spriteId isn't set");
        sprites.emplace_back(_spriteId, offset);
        return offset + 1;
    }
}