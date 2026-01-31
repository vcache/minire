#include "../common/testbed.hpp"

#include <minire/models/mesh.hpp>

namespace
{
    // TODO: add RecursiveSkeletons example from glTF-Sample-Assets
    class AnimatedSkinning
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

            for (size_t i = 1; i <= 7; ++i)
            {
                float const fi = static_cast<float>(i);
                std::string root = "foo-" + std::to_string(i);
                enqueue<SceneNewNode>(root, ScenePath(), Transform(glm::vec3(0, 0, 4.0f - fi)), true);
                enqueue<SceneNewFromSource>(
                    ScenePath{root}, mkPath("SimpleSkin2.gltf", path::Special::kScenes, path::Index(0)),
                    true);
                enqueue<ScenePlayAnimation>("Anim_0", ScenePath{root, "Armature"},
                                            ScenePlayAnimation::kInfinitely, 1.0f * fi);
            }
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<AnimatedSkinning>("Animated skinning");
}
