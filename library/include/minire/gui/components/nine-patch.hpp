#pragma once

#include <minire/content/id.hpp>
#include <minire/gui/component.hpp>
#include <minire/utils/rect.hpp>

#include <string>

namespace minire::gui::components
{
    class NinePatchImage final
        : public Component
    {
    public:
        NinePatchImage(GuiController & controller,
                       std::string const & id,
                       std::shared_ptr<components::Container> const & parent,
                       content::Id const & texture,
                       utils::NinePatch const & tile,
                       Arrangers arranger = Arrangers());

        ~NinePatchImage() override;

    private:
        void onVisibleChanged() override;
        void onContentAreaChanged() override;
        size_t onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                               ZOrderUpdates & sprites) override;

    private:
        std::string _spriteId;
    };
}