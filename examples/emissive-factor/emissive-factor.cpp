#include "../common/testbed.hpp"

#include <glm/gtc/quaternion.hpp>

#include <cmath>

namespace
{
    class MeshEmissiveFactor
        : public minire::examples::TestbedApplication
    {
    private:
        minire::models::Transform cameraTransform(float dist = 5.0f)
        {
            minire::models::Transform result(glm::vec3(dist, dist, dist));
            result._rotation = glm::rotate(result._rotation,
                                           glm::radians(45.0f),
                                           glm::vec3(0, 1, 0));
            // see Rectangular cuboid side (an angle between side's diagonal and main diagonal)
            float const innerAngle =
                std::asin(std::sqrt(2.0*dist) / std::sqrt(3.0*dist)) * 180.0 / M_PI;
            result._rotation = glm::rotate(result._rotation,
                                           -glm::radians(180.0f - (90.0f + innerAngle)),
                                           glm::vec3(1, 0, 0));
            return result;
        }

    public:
        template<typename... Args>
        explicit MeshEmissiveFactor(Args && ... args)
            : TestbedApplication(std::forward<Args>(args)...)
            , _emissiveFactors
            {
                glm::vec3(0, 0, 0),
                glm::vec3(0, 0, 1),
                glm::vec3(0, 1, 0),
                glm::vec3(0, 1, 1),
                glm::vec3(1, 0, 0),
                glm::vec3(1, 0, 1),
                glm::vec3(1, 1, 0),
                glm::vec3(1, 1, 1),
            }
        {}

        void onStart() override
        {
            using namespace minire::content;
            using namespace minire::models;

            TestbedApplication::onStart();

            _cubeNode = scene().root().make(Node{_cubeTransform});
            _cubeMesh = _cubeNode->make(
                Mesh
                {
                    ._source = mkPath("cube.gltf", path::Special::kMeshes, path::Index(0)),
                    ._defaultMaterial = [this]
                    {
                        auto result = std::make_shared<PbrMaterial>();
                        result->_albedoTexture =  "uv-color.png";
                        result->_metallicFactor = 0.5f;
                        result->_roughnessFactor = 0.6f;
                        return result;
                    }(),
                    ._skin = std::nullopt,
                    ._visible = true,
                });
        }

        bool handle(minire::application::OnKeyDown const & e) override
        {
            if (TestbedApplication::handle(e))
                return true;

            if (e._key == SDLK_TAB)
            {
                _emissiveFactorsIndex = (_emissiveFactorsIndex + 1) % _emissiveFactors.size();
                MINIRE_INFO("Mesh emissive factor is set to ({}): {}",
                            _emissiveFactorsIndex, _emissiveFactors[_emissiveFactorsIndex]);
            }

            return true;
        }

        bool onStep() override
        {
            using namespace minire::models;

            float const delta = frameTime();
            _absoluteTime += delta;
            _cubeTransform._rotation = glm::rotate(_cubeTransform._rotation,
                                                   delta * 0.5f,
                                                   glm::vec3{0, 1, 0});
            _cubeNode->setOrigin(_cubeTransform);

            float const w = (1.0f + std::sin(_absoluteTime * 10.0f)) / 2.0f;
            _cubeMesh->setEmissiveFactor(_emissiveFactors[_emissiveFactorsIndex] * w);

            return true;
        }

    private:
        minire::scene::Node::Sptr    _cubeNode;
        minire::scene::Mesh::Sptr    _cubeMesh;
        size_t                       _emissiveFactorsIndex = 0;
        std::vector<glm::vec3> const _emissiveFactors;
        minire::models::Transform    _cubeTransform;
        float                        _absoluteTime = 0;
    };
}

int main(int, char **)
{
    return minire::examples::main<MeshEmissiveFactor>("Emissive Factor");
}
