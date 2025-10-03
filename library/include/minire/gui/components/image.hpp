#pragma once

#include <minire/content/id.hpp>
#include <minire/gui/component.hpp>
#include <minire/utils/rect.hpp>

#include <string>

namespace minire::gui::components
{
    class Image final
        : public Component
    {
    public:
        using Sptr = std::shared_ptr<Image>;

        Image(GuiController & controller,
              std::string const & id,
              std::shared_ptr<Container> const & parent,
              content::Id const & texture,
              utils::Patch const & patch = std::monostate(),
              Arrangers arrangers = Arrangers());

        ~Image() override;

    private:
        void onVisibleChanged() override;
        void onContentAreaChanged() override;
        size_t onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                               ZOrderUpdates & sprites) override;
        std::optional<std::pair<float, float>> measureContent() const override;

    private:
        std::string _spriteId;
        float       _width = 0;
        float       _height = 0;
        bool        _isResizable = false;
    };
}
