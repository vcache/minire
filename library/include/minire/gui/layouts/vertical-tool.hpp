#pragma once

#include <minire/gui/layout.hpp>

#include <string>

namespace minire::gui::layouts
{
    class VerticalTool
        : public LinearLayout
    {
    public:
        using Sptr = std::shared_ptr<VerticalTool>;

        explicit VerticalTool(std::string const & contentId,
                              std::string const & toolId,
                              float const toolWidth,
                              bool const atLeft);

        std::string const & contentId() const { return _contentId; }
        void setContentId(std::string const & id) { _contentId = id; }

        std::string const & toolId() const { return _toolId; }
        void setToolId(std::string const & id) { _toolId = id; }

        float toolWidth() const { return _toolWidth; }
        void setToolWidth(float toolWidth) { _toolWidth = toolWidth; }

        bool atLeft() const { return _atLeft; }
        void setAtLeft(bool atLeft) { _atLeft = atLeft; }

        Area evaluate(Area const & client,
                      Component const &) const override;

    private:
        std::string _contentId;
        std::string _toolId;
        float       _toolWidth;
        bool        _atLeft;
    };
}
