#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/utils/aabb-tools.hpp>
#include <minire/utils/aabb.hpp>

#include <cstdlib> // for EXIT_SUCCESS
#include <vector>

// TODO: turn this example into a gtest

int main()
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);

        // Setup content manager
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

        using namespace minire::content;
        using namespace minire::utils;

        // Read AABBs
        std::vector<Path> paths
        {
            mkPath(Id("sample.gltf")),
            mkPath(Id("sample.gltf"), path::Special::kScenes, Id("Scene")),
            mkPath(Id("sample.gltf"), path::Special::kNodes, Id("Cube")),
            mkPath(Id("sample.gltf"), path::Special::kNodes, Id("Sphere")),
            mkPath(Id("sample.gltf"), path::Special::kMeshes, Id("Cube")),
            mkPath(Id("sample.gltf"), path::Special::kMeshes, Id("Sphere")),
        };

        for(Path const & path : paths)
        {
            Aabb aabb = buildAabb(manager, path);
            MINIRE_INFO("{}: {}", path, aabb);
        }

        manager.clear();

        // Finish
        return EXIT_SUCCESS;
    }
    catch(std::exception const & e)
    {
        MINIRE_ERROR("Fatal error:\n{}", e.what());
    }
    catch(...)
    {
        MINIRE_ERROR("Fatal error: (unknown error)");
    }

    return EXIT_FAILURE;
}
