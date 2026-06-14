#include "../common/testbed.hpp"

namespace
{
    // TODO: add RecursiveSkeletons example from glTF-Sample-Assets
    class AnimatedSkinning
        : public minire::examples::TestbedApplication
    {
    public:
        using TestbedApplication::TestbedApplication;

        void onStart() override
        {
            using namespace minire;
            using namespace minire::content;

            TestbedApplication::onStart();

            for (size_t i = 1; i <= 7; ++i)
            {
                float const fi = static_cast<float>(i);
                scene::Node::Sptr base = scene().root().make(
                    models::Node{models::Transform(glm::vec3(0, 0, 4.0f - fi))});

                base->makeFromSource(mkPath("SimpleSkin2.gltf", path::Special::kScenes, path::Index(0)),
                                     contentManager(), true);

                base->at<scene::Node>("Armature").playbackStack().push(
                    "Anim_0", scene::Node::kInfinitely, 1.0f * fi);
            }
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<AnimatedSkinning>("Animated skinning");
}
