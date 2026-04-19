#include "../common/testbed.hpp"

#include <minire/models/animations.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class InlineAnimation
        : public minire::examples::TestbedApplication
    {
    public:
        using TestbedApplication::TestbedApplication;

        void onStart() override
        {
            using namespace minire;

            TestbedApplication::onStart();

            auto cubeNode = scene().root().make(models::Node{models::Transform()});
            cubeNode->make(
                models::Mesh
                {
                    ._source = content::mkPath(
                        "Box.glb", content::path::Special::kMeshes, content::path::Index(0)),
                    ._defaultMaterial = nullptr,
                    ._skin = std::nullopt,
                    ._visible = true,
                });

            cubeNode->inlineAnimation(
                models::AnimationTracks
                {
                    {
                        models::NodePointer(cubeNode),
                        models::KeyframeAnimation
                        {
                            ._timeline = std::make_shared<std::vector<float>>(std::vector<float>{0, 5}),
                            ._translation = models::KeyframeAnimation::Track<glm::vec3>(
                                {
                                    glm::vec3(0, 0, 0), glm::vec3(5, 5, 5),
                                },
                                models::Interpolation::kLinear),
                            ._rotation = std::nullopt,
                            ._scale = std::nullopt,
                        },
                    }
                },
                scene::Node::kInfinitely,
                1.0f);
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<InlineAnimation>("Inline animation");
}
