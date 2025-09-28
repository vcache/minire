#pragma once

#include <minire/content/id.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/formats/gltf.hpp>
#include <minire/formats/obj.hpp>
#include <minire/models/font-face.hpp>
#include <minire/models/image.hpp>

#include <variant>

namespace minire::content
{
    using Asset = std::variant<std::monostate,
                               std::string,
                               formats::Bdf::Sptr,
                               formats::Obj, // TODO: why not Sptr?
                               formats::GltfModelSptr,
                               models::Image::Sptr,
                               models::FontFace>;

    std::string demangle(Asset const &);

    size_t sizeOf(Asset const &);

    bool hasData(Asset const &);
}
