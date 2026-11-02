#include "../common/testbed.hpp"

#include <minire/material/pbr.hpp>
#include <minire/models/rendering-mode.hpp>

#include <cmath>
#include <cstdlib>
#include <vector>

namespace
{
    class ClusteredForwardExample
        : public minire::examples::TestbedApplication
    {
        static constexpr size_t kPointLightsCount = 100;

        struct LightOrbiter
        {
            minire::scene::Node::Sptr _node;
            float _radius = 3.5f;
            float _speed = 1.0f;
            float _initialAngle = 0.0f;
            float _baseHeight = 0.0f;
            float _heightBob = 0.5f;
            float _bobFreq = 2.0f;
        };

        static glm::vec3 hueToRgb(float hue)
        {
            float const h = hue * 6.0f;
            float const c = 1.0f;
            float const x = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
            if (h < 1.0f) return {c, x, 0.0f};
            if (h < 2.0f) return {x, c, 0.0f};
            if (h < 3.0f) return {0.0f, c, x};
            if (h < 4.0f) return {0.0f, x, c};
            if (h < 5.0f) return {x, 0.0f, c};
            return {c, 0.0f, x};
        }

    public:
        ClusteredForwardExample(int width, int height,
                                std::string const & title,
                                minire::content::Manager & contentManager)
            : TestbedApplication(width, height, title, contentManager,
                                 //minire::models::rendering_mode::Forward{},
                                 minire::models::rendering_mode::ClusteredForward
                                 {
                                     ._clustersX = 16,
                                     ._clustersY = 9,
                                     ._clustersZ = 24,
                                     ._maxLightsPerCluster = 100,
                                     ._maxPointLights = 256,
                                 },
                                 false /* defaultLights */)
        {}

        void onStart() override
        {
            using namespace minire::content;
            using namespace minire::models;

            TestbedApplication::onStart();

            scene().root().make("cube",
                minire::models::Mesh
                {
                    ._source = minire::content::mkPath("Box.glb",
                                                       minire::content::path::Special::kMeshes,
                                                       minire::content::path::Index(0)),
                    ._defaultMaterial = nullptr,
                    ._skin = std::nullopt,
                    ._visible = true,
                });

            _orbiters.reserve(kPointLightsCount);
            for (size_t i = 0; i < kPointLightsCount; ++i)
            {
                float const frac = static_cast<float>(i) / static_cast<float>(kPointLightsCount);
                float const radius = 2.4f + 2.6f * std::sin(frac * static_cast<float>(M_PI) * 4.0f + 1.0f);
                float const speed = (i % 2 == 0 ? 1.0f : -1.0f) * (0.4f + 0.8f * (1.0f - frac));
                float const angle = frac * 2.0f * static_cast<float>(M_PI);
                float const baseHeight = -0.5f + 2.5f * frac;
                float const bobFreq = 1.0f + 3.0f * (static_cast<float>(i % 5) / 5.0f);

                glm::vec3 const color = hueToRgb(frac);
                float const intensity = 15.0f;
                float const range = 5.5f;

                glm::vec3 const initialPos(radius * std::cos(angle), baseHeight, radius * std::sin(angle));

                auto lightNode = scene().root().make(fmt::format("light-node-{}", i),
                                                     Node{Transform(initialPos)});

                ShadowParams pointlightShadowParams;
                pointlightShadowParams._depthBias = shadow_params::bias::Constant{0.05f};

                lightNode->make("point-light",
                                PointLight(glm::vec4(color, intensity),
                                           range,
                                           pointlightShadowParams,
                                           true));

                _orbiters.push_back(LightOrbiter
                {
                    ._node = lightNode,
                    ._radius = radius,
                    ._speed = speed,
                    ._initialAngle = angle,
                    ._baseHeight = baseHeight,
                    ._heightBob = 0.4f + 0.3f * std::cos(frac * 10.0f),
                    ._bobFreq = bobFreq,
                });
            }
        }

        bool onStep() override
        {
            TestbedApplication::onStep();

            float const t = static_cast<float>(absoluteTime());

            for (auto & orbiter : _orbiters)
            {
                float const angle = orbiter._initialAngle + t * orbiter._speed;
                float const x = orbiter._radius * std::cos(angle);
                float const z = orbiter._radius * std::sin(angle);
                float const y = orbiter._baseHeight + orbiter._heightBob * std::sin(t * orbiter._bobFreq + orbiter._initialAngle);

                orbiter._node->setOrigin(minire::models::Transform(glm::vec3(x, y, z)));
            }

            return true;
        }

    private:
        std::vector<LightOrbiter> _orbiters;
    };
}

int main(int, char **)
{
    return minire::examples::main<ClusteredForwardExample>("Clustered Forward");
}
