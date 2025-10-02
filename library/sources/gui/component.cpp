#include <minire/gui/component.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/container.hpp>

#include <utils/overloaded.hpp>

#include <cassert>

namespace minire::gui
{

    Component::~Component() = default;

    void Component::setVisible(bool const visible)
    {
        if (visible == _visible)
            return;

        _visible = visible;
        onVisibleChanged();

        if (_visible)
            rearrange(true);
    }

    // NOTE: std::set cannot be re-ordered automatically just by
    //       changing result of Comp-function.
    //       Instead, it should be recreated.
    void Component::setZOrder(size_t const zOrder)
    {
        if (zOrder == _zOrder)
            return;

        auto parent = _parent.lock();
        auto sharedThis = shared_from_this();

        if (parent)
        {
            parent->_zOrderStore.erase(sharedThis);
        }

        _zOrder = zOrder;

        if (parent)
        {
            parent->_zOrderStore.insert(sharedThis);
        }

        invalidateZOrder();
    }

    void Component::invalidateZOrder()
    {
        _zOrderInvalidated = true;
        for(auto p = parent(); p && !p->_zOrderInvalidated; p = p->parent())
        {
            p->invalidateZOrder();
        }
    }

    void Component::setClientArea(Area clientArea)
    {
        _clientArea = clientArea; 
        rearrange();
    }

    void Component::setArrangers(Arrangers arrangers)
    {
        if (_arrangers == arrangers)
            return;

        _arrangers = std::move(arrangers);
        rearrange();
    }

    void Component::rearrange(bool force)
    {
        if (!_visible)
            return;

        std::optional<std::pair<float, float>> contentRealSize = measureContent();
        auto [left, width] = _arrangers._horizontal(_clientArea._left,
                                                    _clientArea._width,
                                                    contentRealSize ? std::optional<float>(contentRealSize->first)
                                                                    : std::nullopt);
        auto [top, height] = _arrangers._vertical(_clientArea._top,
                                                  _clientArea._height,
                                                  contentRealSize ? std::optional<float>(contentRealSize->second)
                                                                  : std::nullopt);
        Area const contentArea{._left = left,
                               ._top = top,
                               ._width = width,
                               ._height = height};
        if (contentArea != _contentArea || force)
        {
            _contentArea = contentArea;
            onContentAreaChanged();
        }
    }

    size_t Component::revalidateZOrder(size_t offset, ZOrderUpdates & labels,
                                       ZOrderUpdates & sprites)
    {
        if (_zOrderInvalidated || offset > _zOrderBoundaries.first)
        {
            _zOrderBoundaries.first = offset;
            _zOrderBoundaries.second = onZOrderChanged(offset, labels, sprites);
            _zOrderInvalidated = false;
        }
        return _zOrderBoundaries.second;
    }

    void Component::enqueueRaw(events::Controller && event)
    {
        _controller.enqueueRaw(std::move(event));
    }

    void Component::focus()
    {
        _controller.setFocus(shared_from_this());
    }

    void Component::unfocus()
    {
        _controller.setFocus();
    }

    std::unique_ptr<content::Lease>
    Component::borrow(content::Id const & id) const
    {
        return _controller.borrow(id);
    }

    glm::vec2 Component::measure(text::FormattedString const & text,
                                 content::Id const & id) const
    {
        return _controller.measure(text, id);
    }

    std::pair<glm::vec2, bool>
    Component::measure(utils::Patch const & patch,
                       content::Id const & texture) const
    {
        return std::visit(utils::Overloaded
        {
            [this, &texture](std::monostate const &)
            {
                auto lease = borrow(texture);
                assert(lease);
                models::Image::Sptr image = lease->as<models::Image::Sptr>();
                MINIRE_INVARIANT(image, "not a valid image: {}", texture);
                return std::make_pair(glm::vec2(image->_width, image->_height),
                                      false);
            },

            [this](utils::Rect const & tile)
            {
                return std::make_pair(glm::vec2(tile._right - tile._left + 1,
                                                tile._bottom - tile._top + 1),
                                      false);
            },

            [this](utils::NinePatch const & ninePatch)
            {
                return std::make_pair(utils::defaultSize(ninePatch), true);
            },
        }, patch);
    }

    std::optional<std::pair<float, float>>
    Component::measureContent() const
    {
        return std::nullopt;
    }
}
