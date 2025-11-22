#include "../common/testbed.hpp"

#include <minire/models/mesh.hpp>

#include <fmt/format.h>

#include <cstdlib> // for EXIT_SUCCESS
#include <vector>

namespace
{
    class Billboards
        : public minire::examples::TestbedController
    {
    public:
        using TestbedController::TestbedController;

        auto makeText() const
        {
            minire::text::FormattedString text;
            // TODO: alpha channel
            // TODO: different styles
            text.append(L"Hello world").background(glm::vec4(0, 0, 0, 0))
                                       .foreground(glm::vec4(1, 0, 0, 0));
            return text;
        }

        void start() override
        {
            using namespace minire::content;
            using namespace minire::events::controller;
            using namespace minire::models;

            TestbedController::start();

            size_t counter = 0;
            auto mkSample = [&](glm::vec3 origin,
                                glm::vec3 billboardNodeOrigin,
                                std::vector<Billboard> billboards)
            {
                std::string rootNode = fmt::format("cube-node-{}", counter++);
                enqueue<SceneNewNode>(rootNode, ScenePath(), Transform(origin), true);
                enqueue<SceneNewMesh>("cube", ScenePath{rootNode},
                    Mesh
                    {
                        ._source = mkPath("Box.glb", path::Special::kMeshes, path::Index(0)),
                        ._defaultMaterial = nullptr,
                    },
                    true);

                enqueue<SceneNewNode>("billboard-node", ScenePath{rootNode}, Transform(billboardNodeOrigin), true);
                for(Billboard const & billboard : billboards)
                {
                    enqueue<SceneNewBillboard>(fmt::format("billboard-{}", (void const *)&billboard),
                                               ScenePath{rootNode, "billboard-node"},
                                               billboard, true);
                }
            };

            static minire::utils::NinePatch const kNinePatch
            {
                ._boundary = minire::utils::Rect(55, 49, 158, 152),
                ._out = minire::utils::Rect(69, 63, 144, 138),
                ._in = minire::utils::Rect(72, 66, 141, 135),
            };

            static minire::utils::Rect const kRect(8, 6, 39, 37);

            // World placement //

            {
                // Whole image

                mkSample(glm::vec3(-4, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                mkSample(glm::vec3(-2, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-rectangular.png", std::monostate(), {}},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // Part of image

                mkSample(glm::vec3(0, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // NinePatch
                mkSample(glm::vec3(2, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // Z-Ordering
                mkSample(glm::vec3(4, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::World({0, .25}, {.5, .5}), 0},
                          Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::World({0, .25}, {.4, .4}), 1},
                          Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::World({0, .25}, {.3, .3}), 2}});

                // Text

                mkSample(glm::vec3(6, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Label{makeText(), minire::examples::kFontFace},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // TODO: Animations (scale, rotate, translate, update image)
            }

            // Screen placement //

            {
                // Whole image

                mkSample(glm::vec3(-4, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::Screen({0, 16}), 0}});

                mkSample(glm::vec3(-2, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-rectangular.png", std::monostate(), {}},
                                    Billboard::Screen({0, 16}), 0}});

                // Part of image

                mkSample(glm::vec3(0, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::Screen({0, 16}), 0}});

                // NinePatch

                mkSample(glm::vec3(2, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::Screen({0, 100}), 0}});

                // Z-Ordering

                mkSample(glm::vec3(4, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::Screen({0, 100}), 0},
                          Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::Screen({0, 50}), 1},
                          Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::Screen({10, 60}), 2}});

                // Text

                mkSample(glm::vec3(6, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Label{makeText(), minire::examples::kFontFace},
                                    Billboard::Screen({0, 10}), 0}});

                // TODO: Animations (scale, rotate, translate, update image)
            }

            // TODO: Screen + World mixed placement
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<Billboards>("Billboards");
}
