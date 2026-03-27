#include "../common/testbed.hpp"

#include <glm/gtc/quaternion.hpp>

#include <cmath>

namespace
{
    class ComplexScene
        : public minire::examples::TestbedApplication
    {
        void attachCube(minire::scene::Node & node)
        {
            node.make("cube",
                minire::models::Mesh
                {
                    ._source = minire::content::mkPath("Box.glb",
                                                       minire::content::path::Special::kMeshes,
                                                       minire::content::path::Index(0)),
                    ._defaultMaterial = nullptr,
                    ._skin = std::nullopt,
                    ._visible = true,
                });
        }

    public:
        using TestbedApplication::TestbedApplication;

        void onStart() override
        {
            TestbedApplication::onStart();

            _joint0 = scene().root().make("joint-0",
                minire::models::Node{minire::models::Transform{}, true});
            attachCube(*_joint0);
            _joint0->inlineAnimation(
                minire::models::AnimationTracks
                {
                    {
                        minire::models::NodePointer(_joint0),
                        minire::models::KeyframeAnimation
                        {
                            ._timeline = std::make_shared<std::vector<float>>(std::vector<float>{0, 2.5, 5}),
                            ._translation = minire::models::KeyframeAnimation::Track<glm::vec3>(
                                {
                                    glm::vec3(-.5, 0, -.5),
                                    glm::vec3(.5, 0, .5),
                                    glm::vec3(-.5, 0, -.5),
                                },
                                minire::models::Interpolation::kLinear),
                            ._rotation = std::nullopt,
                            ._scale = std::nullopt,
                        },
                    }
                },
                minire::scene::Node::kInfinitely,
                1.0f);

            _joint1 = _joint0->make("joint-1",
                minire::models::Node{minire::models::Transform{glm::vec3{0, 1.5, 0}}, true});
            attachCube(*_joint1);
            _joint1->inlineAnimation(
                minire::models::AnimationTracks
                {
                    {
                        minire::models::NodePointer(_joint1),
                        minire::models::KeyframeAnimation
                        {
                            ._timeline = std::make_shared<std::vector<float>>(std::vector<float>{0, 1, 2}),
                            ._translation = std::nullopt,
                            ._rotation = std::nullopt,
                            ._scale = minire::models::KeyframeAnimation::Track<glm::vec3>(
                                {
                                    glm::vec3(1.0),
                                    glm::vec3(0.5),
                                    glm::vec3(1.0),
                                },
                                minire::models::Interpolation::kLinear),
                        },
                    }
                },
                minire::scene::Node::kInfinitely,
                1.0f);

            _joint2 = _joint1->make("joint-2",
                minire::models::Node{minire::models::Transform{glm::vec3{0, 1.5, 0}}, true});
            attachCube(*_joint2);
            glm::vec3 const rotationAxis = glm::normalize(glm::vec3(.5, .5, .5));
            _joint2->inlineAnimation(
                minire::models::AnimationTracks
                {
                    {
                        minire::models::NodePointer(_joint2),
                        minire::models::KeyframeAnimation
                        {
                            ._timeline = std::make_shared<std::vector<float>>(std::vector<float>{0, 2, 4}),
                            ._translation = std::nullopt,
                            ._rotation = minire::models::KeyframeAnimation::Track<glm::quat>(
                                {
                                    glm::angleAxis(glm::radians(0.0f), rotationAxis),
                                    glm::angleAxis(glm::radians(180.0f), rotationAxis),
                                    glm::angleAxis(glm::radians(360.0f), rotationAxis),
                                },
                                minire::models::Interpolation::kLinear),
                            ._scale = std::nullopt,
                        },
                    }
                },
                minire::scene::Node::kInfinitely,
                1.0f);
        }

        bool onStep() override
        {
            TestbedApplication::onStep();

            bool const flip = (std::lround(absoluteTime() / 1.0)) % 2;

            assert(_joint1);
            _joint1->setVisible(flip);

            return true;
        }

    private:
        minire::scene::Node::Sptr _joint0;
        minire::scene::Node::Sptr _joint1;
        minire::scene::Node::Sptr _joint2;
    };
}

int main(int, char **)
{
    return minire::examples::main<ComplexScene>("Complex Scene");
}