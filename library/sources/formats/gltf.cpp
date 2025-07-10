#include <minire/formats/gltf.hpp>

#define TINYGLTF_IMPLEMENTATION

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <nlohmann/json.hpp>
#include <stb/stb_image.h>

#include <tinygltf/tiny_gltf.h>

namespace minire::formats
{
    GltfModelSptr loadGltf(std::string const & filename)
    {
        auto result = std::make_shared<::tinygltf::Model>();

        ::tinygltf::TinyGLTF loader;

        std::string err;
        std::string warn;

        bool const loaded = loader.LoadASCIIFromFile(result.get(), &err, &warn, filename);

        if (!warn.empty())
        {
            MINIRE_WARNING("gLTF loading warning (\"{}\"): {}", filename, warn);
        }

        if (!loaded)
        {
            MINIRE_THROW("failed to load gLTF file \"{}\": {}", filename, err);
        }

        return result;
    }

    GltfModelSptr loadGlb(std::string const & filename)
    {
        auto result = std::make_shared<::tinygltf::Model>();

        ::tinygltf::TinyGLTF loader;

        std::string err;
        std::string warn;

        bool const loaded = loader.LoadBinaryFromFile(result.get(), &err, &warn, filename);

        if (!warn.empty())
        {
            MINIRE_WARNING("gLB loading warning (\"{}\"): {}", filename, warn);
        }

        if (!loaded)
        {
            MINIRE_THROW("failed to load gLB file \"{}\": {}", filename, err);
        }

        return result;
    }
}