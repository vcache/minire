#include "../common/testbed.hpp"

#include <minire/models/animations.hpp>
#include <minire/models/mesh.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class InlineAnimation
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
            enqueue<SceneNewMesh>("cube-mesh", ScenePath{"cube-node"},
                Mesh
                {
                    ._source = mkPath("Box.glb", path::Special::kMeshes, path::Index(0)),
                    ._defaultMaterial = nullptr,
                },
                true);

            enqueue<SceneInlineAnimation>(
                ScenePath{"cube-node"},
                AnimationTracks
                {
                    {
                        ScenePath{},
                        KeyframeAnimation
                        {
                            ._timeline = std::make_shared<std::vector<float>>(std::vector<float>{0, 5}),
                            ._translation = KeyframeAnimation::Track<glm::vec3>(
                                {
                                    glm::vec3(0, 0, 0), glm::vec3(5, 5, 5),
                                },
                                Interpolation::kLinear),
                            ._rotation = std::nullopt,
                            ._scale = std::nullopt,
                        },
                    }
                },
                ScenePlayAnimation::kInfinitely,
                1.0f);
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<InlineAnimation>("Inline animation");
}
