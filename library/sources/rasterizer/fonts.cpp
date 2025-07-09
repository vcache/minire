#include <rasterizer/fonts.hpp>

#include <rasterizer/font.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/logging.hpp>

#include <cassert>

namespace minire::rasterizer
{
    Fonts::Fonts(content::Manager & contentManager,
                 content::Ids const & preload)
        : _contentManager(contentManager)
    {
        for(content::Id const & fontId : preload)
        {
            get(fontId);
        }
    }

    std::shared_ptr<Font const>
    Fonts::get(content::Id const & fontId) const
    {
        auto const it = _fonts.find(fontId);

        if (it == _fonts.cend())
        {
            auto lease = _contentManager.borrow(fontId);
            assert(lease);
            formats::Bdf::Sptr const & bdf = lease->as<formats::Bdf::Sptr>();
            if (!bdf) MINIRE_THROW("bdf is a nullptr: {}", fontId);
            bdf->fillChar(0, 0);
            auto nit = _fonts.emplace(fontId, std::make_shared<Font>(*bdf));
            MINIRE_INVARIANT(nit.second, "failed to save cached Font");
            MINIRE_INFO("Loading font: {}", fontId);
            assert(nit.first->second);
            return nit.first->second;
        }

        assert(it->second);
        return it->second;
    }
}
