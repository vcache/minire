#include <minire/gui/layouts/vertical-tool.hpp>

#include <minire/gui/component.hpp>
#include <minire/logging.hpp>

namespace minire::gui::layouts
{
    VerticalTool::VerticalTool(std::string const & contentId,
                               std::string const & toolId,
                               float const toolWidth,
                               bool const atLeft)
        : _contentId(contentId)
        , _toolId(toolId)
        , _toolWidth(toolWidth)
        , _atLeft(atLeft)
    {}

    Area VerticalTool::evaluate(Area const & clientArea,
                                Component const & component) const
    {
        if (component.id() == _contentId)
        {
            return Area
            {
                ._left = clientArea._left + (_atLeft ? _toolWidth : .0f),
                ._top = clientArea._top,
                ._width = clientArea._width - _toolWidth,
                ._height = clientArea._height,
            };
        }
        else if (component.id() == _toolId)
        {
            return Area
            {
                ._left = clientArea._left + (_atLeft ? .0f : clientArea._width - _toolWidth),
                ._top = clientArea._top,
                ._width = _toolWidth,
                ._height = clientArea._height,
            };
        }
        else
        {
            MINIRE_WARNING("unregistered Component to layout: {}",
                           component.id());
            return clientArea;
        }
    }
}
