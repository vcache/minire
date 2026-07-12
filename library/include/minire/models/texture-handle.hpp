#pragma once

#include <minire/models/sampler.hpp>
#include <minire/content/id.hpp>

#include <memory>

// implemetation details, should be ignored
namespace minire::rasterizer { class Materials; }

namespace minire::models
{
    class TextureHandle
    {
    public:
        using Sptr = std::shared_ptr<TextureHandle>;

        TextureHandle() = default;
        virtual ~TextureHandle() = default;

    private:
        virtual void bind() const = 0;

        friend ::minire::rasterizer::Materials;
    };

    class TextureResolver
    {
    public:
        TextureResolver() = default;
        virtual ~TextureResolver() = default;

        // Will return non-empty pointer or throw an exception.
        // Non-null result guarantee.
        virtual TextureHandle::Sptr resolve(content::Id const & textureId,
                                            models::Sampler const & sampler) const = 0;
    };
}