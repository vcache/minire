#include <minire/gui/components/text.hpp>

#include <minire/gui-controller.hpp>

#include <utils/uuid.hpp>

#include <cassert>

namespace minire::gui::components
{
    namespace
    {
        bool isContentDefined(Arranger const & arranger)
        {
            return std::holds_alternative<dimension::Content>(arranger.dimension());
        }
    }

    Text::Text(GuiController & controller,
               std::string const & id,
               std::shared_ptr<Container> const & parent,
               text::FormattedString const & text,
               content::Id const & fontFace,
               Arrangers arrangers)
        : Component(controller, id, parent)
        , _labelId(utils::newUuid())
        , _fontFace(fontFace)
        , _measurements(controller.measure(text, fontFace))
        , _clippingSet(false)
    {
        enqueue<events::controller::CreateLabel>(
            _labelId, text, fontFace, glm::vec2(0), visible(), zOrder());

        // forcing rearrange to calculate contentArea()
        setArrangers(arrangers);
    }

    Text::~Text()
    {
        assert(!_labelId.empty());
        enqueue<events::controller::RemoveLabel>(_labelId);
    }


    void Text::setText(text::FormattedString const & text)
    {
        assert(!_labelId.empty());
        _measurements = measure(text, _fontFace);
        enqueue<events::controller::SetLabelText>(_labelId, text);
        rearrange();
    }

    void Text::onVisibleChanged()
    {
        assert(!_labelId.empty());
        enqueue<events::controller::SetLabelVisible>(_labelId, visible());
    }

    void Text::onContentAreaChanged()
    {
        assert(!_labelId.empty());

        Area const & area = contentArea();
        enqueue<events::controller::MoveLabel>(
            _labelId, glm::vec2(area._left, area._top));

        Arrangers const & arrangers = Component::arrangers();
        if (!isContentDefined(arrangers._vertical) ||
            !isContentDefined(arrangers._horizontal))
        {
            enqueue<events::controller::SetLabelClipping>(
                _labelId, glm::vec2(area._width, area._height));
            _clippingSet = true;
        }
        else if (_clippingSet)
        {
            enqueue<events::controller::SetLabelClipping>(
                _labelId, std::nullopt);
            _clippingSet = false;
        }
    }

    size_t Text::onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                                 ZOrderUpdates &)
    {
        assert(!_labelId.empty());
        labels.emplace_back(_labelId, offset);
        return offset + 1;
    }

    std::optional<std::pair<float, float>> Text::measureContent() const
    {
        return std::make_pair(_measurements.x, _measurements.y);
    }
}
