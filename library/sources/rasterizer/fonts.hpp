#pragma once

#include <minire/content/id.hpp>

#include <memory>
#include <unordered_map>

namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Font;

    class Fonts
    {
    public:
        explicit Fonts(content::Manager & contentManager,
                       content::Ids const & preload);

        std::shared_ptr<Font const> get(content::Id const &) const;

    private:
        using Cache = std::unordered_map<content::Id,
                                         std::shared_ptr<Font>>;

        content::Manager & _contentManager;
        mutable Cache      _fonts;
    };
}
