#include "../common/testbed.hpp"

#include <minire/models/pbr-material.hpp>

#include <cassert>
#include <cstdlib> // for EXIT_SUCCESS
#include <cstring> // for std::memcpy
#include <initializer_list>
#include <random>

namespace
{
    class VertexBufferExample
        : public minire::examples::TestbedApplication
    {
        static size_t constexpr kSphereRes = 50;

        auto makeQuadVertexBuffer() const
        {
            using namespace minire::models;
            using namespace minire::utils;

            float const sz = 5.0f;

            auto elements = std::make_shared<std::vector<uint8_t>>(std::initializer_list<uint8_t>{0, 1, 2, 3});
            auto vertices = std::make_shared<std::vector<float>>(std::initializer_list<float>{-sz, 0.0f,  sz,
                                                                                               sz, 0.0f,  sz,
                                                                                               sz, 0.0f, -sz,
                                                                                              -sz, 0.0f, -sz,
            });
            auto normals = std::make_shared<std::vector<float>>(std::initializer_list<float>{0.0f, 1.0f, 0.0f,
                                                                                             0.0f, 1.0f, 0.0f,
                                                                                             0.0f, 1.0f, 0.0f,
                                                                                             0.0f, 1.0f, 0.0f,
            });
            return VertexBuffer
            {
                ._drawMode = VertexBuffer::DrawMode::kQuads,
                ._elements = VertexBuffer::ElementBuffer
                {
                    ._indecesBuffer = VertexBuffer::ElementBuffer::IndecesBuffer(elements),
                    ._usage = VertexBuffer::Usage::kStaticDraw,
                },
                ._vertices = VertexBuffer::TightlyPackedBuffer
                {
                    3, false, 0, VertexBuffer::Usage::kStaticDraw, vertices
                },
                ._normals = VertexBuffer::TightlyPackedBuffer
                {
                    3, false, 0, VertexBuffer::Usage::kStaticDraw, normals
                },
                ._tangents = std::monostate(),
                ._uvs = std::monostate(),
                ._aabb = Aabb(-1.0f, 0.0f, -1.0f,
                               1.0f, 0.0f, 1.0f),
                ._isDoubleSided = false,
            };
        }

        struct VertexData
        {
            glm::vec3 _vertex;
            glm::vec3 _normal;
        };

        static_assert(sizeof(glm::vec3) == sizeof(float) * 3,
                      "unexpected size of glm::vec3");
        static_assert(sizeof(VertexData) == sizeof(float) * 3 * 2,
                      "unexpected size of VertexData");

        // The algorithm was sneaked from here:
        //  https://danielsieger.com/blog/2021/03/27/generating-spheres.html
        auto makeUvSphere(int slices, int stacks, float phase, size_t seed = 0)
        {
            using namespace minire::models;
            using namespace minire::utils;

            assert(slices > 0);
            assert(stacks > 0);

            std::vector<VertexData> vertexArray;

            // Build vertices array

            VertexData v0{glm::vec3(0.0f,  1.0f, 0.0f), glm::vec3(0.0f,  1.0f, 0.0f)};
            VertexData v1{glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)};

            vertexArray.push_back(v0);
            for (int i = 0; i < stacks - 1; i++)
            {
                auto phi = M_PI * static_cast<float>(i + 1) / static_cast<float>(stacks);
                for (int j = 0; j < slices; j++)
                {
                    auto theta = 2.0f * M_PI * static_cast<float>(j) / static_cast<float>(slices);
                    auto x = std::sin(phi) * std::cos(theta);
                    auto y = std::cos(phi);
                    auto z = std::sin(phi) * std::sin(theta);
                    glm::vec3 v(x, y, z);
                    vertexArray.push_back(VertexData(v, glm::normalize(v)));
                }
            }
            vertexArray.push_back(v1);

            // Build trinagles for hats

            auto elements = std::make_shared<std::vector<uint32_t>>();
            for (int i = 0; i < slices; ++i)
            {
                auto i0 = i + 1;
                auto i1 = (i + 1) % slices + 1;

                elements->push_back(0);  // v0
                elements->push_back(i1);
                elements->push_back(i0);

                i0 = i + slices * (stacks - 2) + 1;
                i1 = (i + 1) % slices + slices * (stacks - 2) + 1;

                elements->push_back(vertexArray.size() - 1); // v1
                elements->push_back(i0);
                elements->push_back(i1);
            }

            // Build trinagles for body

            for (int j = 0; j < stacks - 2; j++)
            {
                auto j0 = j * slices + 1;
                auto j1 = (j + 1) * slices + 1;
                for (int i = 0; i < slices; i++)
                {
                    auto i0 = j0 + i;
                    auto i1 = j0 + (i + 1) % slices;
                    auto i2 = j1 + (i + 1) % slices;
                    auto i3 = j1 + i;

                    elements->push_back(i0);
                    elements->push_back(i1);
                    elements->push_back(i2);

                    elements->push_back(i0);
                    elements->push_back(i2);
                    elements->push_back(i3);
                }
            }

            // Add animation part

            std::mt19937 gen(seed);
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for(VertexData & v : vertexArray)
            {
                v._vertex += 0.3f * dist(gen) * v._normal * phase;
            }

            // Calc AABB (not required actually)

            Aabb aabb;
            for(VertexData const & v : vertexArray)
            {
                aabb.extend(v._vertex);
            }

            // Convert vertices array into a byte array

            size_t const bytesCount = vertexArray.size() * sizeof(VertexData);
            auto vertexBuffer = std::make_shared<std::vector<uint8_t>>(bytesCount, 0);
            std::memcpy(reinterpret_cast<void *>(vertexBuffer->data()),
                        reinterpret_cast<void const *>(vertexArray.data()),
                        bytesCount);

            // Build a VertexBuffer element

            size_t const stride = sizeof(VertexData);
            return VertexBuffer
            {
                ._drawMode = VertexBuffer::DrawMode::kTriangles,
                ._elements = VertexBuffer::ElementBuffer
                {
                    ._indecesBuffer = VertexBuffer::ElementBuffer::IndecesBuffer(elements),
                    ._usage = VertexBuffer::Usage::kStreamDraw,
                },
                ._vertices = VertexBuffer::StridedBuffer
                {
                    3 /* comonents */, false /* normalized */, 0 /* offset */,
                    VertexBuffer::Usage::kStreamDraw, vertexBuffer,
                    stride, VertexBuffer::DataType::kFloat,
                },
                ._normals = VertexBuffer::StridedBuffer
                {
                    3 /* comonents */, false /* normalized */, sizeof(VertexData::_vertex) /* offset */,
                    VertexBuffer::Usage::kStreamDraw, vertexBuffer,
                    stride, VertexBuffer::DataType::kFloat,
                },
                ._tangents = std::monostate(),
                ._uvs = std::monostate(),
                ._aabb = aabb,
                ._isDoubleSided = false,
            };
        }

        auto makeMaterial() const
        {
            using namespace minire::models;
            auto result = std::make_shared<PbrMaterial>();
            result->_albedoFactor = glm::vec3(1.0f, 0.0f, 0.0f);
            result->_metallicFactor = 0.5f;
            result->_roughnessFactor = 0.6f;
            return result;
        }

    public:
        using TestbedApplication::TestbedApplication;

        void onStart() override
        {
            using namespace minire::content;
            using namespace minire::models;

            TestbedApplication::onStart();

            // Build customized vertex buffers
            createVertexBuffer("quads-example", makeQuadVertexBuffer(), false);
            createVertexBuffer("sphere-example", makeUvSphere(kSphereRes, kSphereRes,
                                                              _sphereAnimationPhase,
                                                              _sphereAnimationSeed),
                               false);

            // Build a material
            auto material = makeMaterial();

            // Attach them to a scene
            auto quadNode = scene().root().make(Node{Transform(glm::vec3(0, -1, 0))});
            quadNode->make("quad",
                Mesh
                {
                    ._source = mkPath(path::Special::kVertexBuffers, "quads-example"),
                    ._defaultMaterial = material,
                    ._skin = std::nullopt,
                    ._visible = true,
                });

            auto sphereNode = scene().root().make(Node{Transform()});
            sphereNode->make("sphere",
                Mesh
                {
                    ._source = mkPath(path::Special::kVertexBuffers, "sphere-example"),
                    ._defaultMaterial = material,
                    ._skin = std::nullopt,
                    ._visible = true,
                });
        }

        bool onStep() override
        {
            TestbedApplication::onStep(); // intentionally ignore the result

            _sphereAnimationPhase += frameTime();
            auto const phase = std::sin(_sphereAnimationPhase);
            createVertexBuffer("sphere-example",
                               makeUvSphere(kSphereRes, kSphereRes,
                                            phase, _sphereAnimationSeed),
                               true);
            if (_sphereAnimationPhase >= 2 * M_PI)
            {
                _sphereAnimationSeed++;
                _sphereAnimationPhase -= 2 * M_PI;
            }

            return true;
        }

    private:
        float  _sphereAnimationPhase = 0;
        size_t _sphereAnimationSeed = 0;
    };
}

int main(int, char **)
{
    return minire::examples::main<VertexBufferExample>("Vertex buffer");
}
