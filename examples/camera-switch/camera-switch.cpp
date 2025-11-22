#include "../common/testbed.hpp"

#include <minire/models/mesh.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class CameraSwitch
        : public minire::examples::TestbedController
    {
    public:
        using TestbedController::TestbedController;

        void start() override
        {
            using namespace minire::content;
            using namespace minire::events::controller;
            using namespace minire::models;

            TestbedController::start();

            enqueue<SceneNewNode>("cube-node", ScenePath(), Transform(), true);
            enqueue<SceneNewMesh>("cube", ScenePath{"cube-node"},
                Mesh
                {
                    ._source = mkPath("Box.glb", path::Special::kMeshes, path::Index(0)),
                    ._defaultMaterial = nullptr,
                },
                true);
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<CameraSwitch>("Camera switch");
}
