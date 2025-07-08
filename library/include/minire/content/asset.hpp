#pragma once

#include <minire/content/id.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/formats/obj.hpp>
#include <minire/models/font.hpp>
#include <minire/models/image.hpp>
#include <minire/models/scene-model.hpp>

#include <variant>

namespace minire::content
{
    using Asset = std::variant<std::monostate,
                               std::string,
                               formats::Bdf::Sptr,
                               formats::Obj, // TODO: why not Sptr?
                               models::Image::Sptr,
                               models::Font,
                               models::SceneModel>;

    std::string demangle(Asset const &);

    size_t sizeOf(Asset const &);

    bool hasData(Asset const &);
}
