#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/grips/orbiting.hpp>
#include <minire/grips/panning.hpp>
#include <minire/logging.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>

#include <cassert>
#include <cstdlib> // for EXIT_SUCCESS
#include <cstring> // for std::memcpy
#include <initializer_list>
#include <random>

namespace
{
    static size_t constexpr kCtrlFps = 60;

    class VertexBufferExample
        : public minire::BasicController
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
        explicit VertexBufferExample(minire::content::Manager & contentManager)
            : BasicController(contentManager, kCtrlFps)
            , _target(0.0f, 0.0f, 0.0f)
            , _orbiting(_target, 10, glm::radians(45.0), glm::radians(45.0))
            , _orthographicCamera{._xMag = _orbiting.distance() / 2,
                                  ._yMag = _orbiting.distance() / 2,
                                  ._zNear = 0.001f,
                                  ._zFar = 1000.0f}
            , _perspectiveCamera{._yFov = glm::radians(45.0f),
                                 ._zNear = 0.001f,
                                 ._zFar = 1000.0f,
                                 ._aspectRatio = std::nullopt}
            , _isPerspective(true)
        {}

        void start() override
        {
            using namespace minire::content;
            using namespace minire::events::controller;
            using namespace minire::models;

            _orbiting.evaluate(_cameraTransform);
            enqueue<SceneNewNode>("cam-node", ScenePath(), _cameraTransform, true);
            enqueue<SceneNewPerspectiveCamera>("persp-cam", ScenePath{"cam-node"}, _perspectiveCamera, true);
            enqueue<SceneNewOrthographicCamera>("ortho-cam", ScenePath{"cam-node"}, _orthographicCamera, true);
            enqueue<SceneActivateCamera>(ScenePath{"cam-node", "persp-cam"});

            enqueue<SceneNewNode>("light-node", ScenePath(), Transform(glm::vec3(2.0f,  2.0f, 2.0f)), true);
            enqueue<SceneNewPointLight>("bulb", ScenePath{"light-node"}, PointLight(glm::vec4(1, 1, 1, 500), 2), true);

            enqueue<CreateVertexBuffer>("quads-example", makeQuadVertexBuffer(), false);
            enqueue<CreateVertexBuffer>("sphere-example",
                                        makeUvSphere(kSphereRes, kSphereRes,
                                                     _sphereAnimationPhase,
                                                     _sphereAnimationSeed),
                                        false);

            auto material = makeMaterial();

            enqueue<SceneNewNode>("quad-node", ScenePath(), Transform(glm::vec3(0, -1, 0)), true);
            enqueue<SceneNewMesh>("quad", ScenePath{"quad-node"},
                Mesh
                {
                    ._source = mkPath(path::Special::kVertexBuffers, "quads-example"),
                    ._defaultMaterial = material,
                },
                true);

            enqueue<SceneNewNode>("sphere-node", ScenePath(), Transform(), true);
            enqueue<SceneNewMesh>("sphere", ScenePath{"sphere-node"},
                Mesh
                {
                    ._source = mkPath(path::Special::kVertexBuffers, "sphere-example"),
                    ._defaultMaterial = material,
                },
                true);
        }

        void step() override
        {
            using namespace minire::events::controller;

            _sphereAnimationPhase += frameTime();
            auto const phase = std::sin(_sphereAnimationPhase);
            enqueue<CreateVertexBuffer>("sphere-example",
                                        makeUvSphere(kSphereRes, kSphereRes,
                                                     phase, _sphereAnimationSeed),
                                        true);
            if (_sphereAnimationPhase >= 2 * M_PI)
            {
                _sphereAnimationSeed++;
                _sphereAnimationPhase -= 2 * M_PI;
            }
        }

        void handle(minire::events::application::OnMouseMove const & event) override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            bool updated = false;
            if (event._left)
            {
                _orbiting.updateAngles(0.01f * static_cast<float>(event._relX),
                                       0.01f * static_cast<float>(event._relY));
                updated = true;
            }
            else if (event._right && _panning)
            {
                if (_isPerspective)
                {
                    _orbiting.target() = _panning.update(event._absX, event._absY,
                                                         _cameraTransform.matrix(),
                                                         _target, _perspectiveCamera,
                                                         _windowSize, _cameraTransform._translation);
                }
                else
                {
                    _orbiting.target() = _panning.update(event._absX, event._absY,
                                                         _cameraTransform.matrix(),
                                                         _target, _orthographicCamera,
                                                         _windowSize, _cameraTransform._translation);
                }
                updated = true;
            }

            if (updated)
            {
                _orbiting.evaluate(_cameraTransform);
                enqueue<SceneSetTransform>(ScenePath{"cam-node"}, _cameraTransform);
            }
        }

        void handle(minire::events::application::OnMouseDown const & e) override
        {
            if (e._mouseButton == minire::models::MouseButton::kRight)
            {
                _panning.start(e._x, e._y);
            }
        }

        void handle(minire::events::application::OnMouseUp const &) override
        {
            if (_panning)
            {
                _panning.finish(_target);
            }
        }

        void handle(minire::events::application::OnMouseWheel const & event) override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            _orbiting.updateDistance(-0.5f * event._dy);
            _orbiting.evaluate(_cameraTransform);
            enqueue<SceneSetTransform>(ScenePath{"cam-node"}, _cameraTransform);

            _orthographicCamera._xMag = _orthographicCamera._yMag = _orbiting.distance() / 2.0f;
            enqueue<SceneSetOrthographicCamera>(ScenePath{"cam-node", "ortho-cam"},
                                                _orthographicCamera);
        }

        void handle(minire::events::application::OnResize const & e) override
        {
            _windowSize.x = static_cast<float>(e._width);
            _windowSize.y = static_cast<float>(e._height);
        }

        void handle(minire::events::application::OnKeyDown const & e)
        {
            if (e._key == SDLK_c)
            {
                _isPerspective = !_isPerspective;
                MINIRE_INFO("Camera is switched to {}", _isPerspective ? "PERSPECTIVE"
                                                                       : "ORTHOGRAPHIC");
                using namespace minire::events::controller;
                using namespace minire::models;
                enqueue<SceneActivateCamera>(ScenePath{"cam-node", _isPerspective ? "persp-cam" : "ortho-cam"});
            }
        }

    private:
        float                              _sphereAnimationPhase = 0;
        size_t                             _sphereAnimationSeed = 0;

        glm::vec3                          _target;
        minire::grips::Orbiting            _orbiting;
        minire::models::OrthographicCamera _orthographicCamera;
        minire::models::PerspectiveCamera  _perspectiveCamera;
        minire::models::Transform          _cameraTransform;
        minire::grips::Panning<false>      _panning;
        glm::vec2                          _windowSize;
        bool                               _isPerspective;
    };
}

int main(int, char **)
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);

        // Setup content manager
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

        // Create and run the Application and its Controller
        minire::Application application(1280, 720, "Vertex buffer", manager);
        application.setController<VertexBufferExample>();
        application.setVsync(true);
        application.setGlDebug(false);

        // Main loop
        application.run();

        // Finish
        return EXIT_SUCCESS;
    }
    catch(std::exception const & e)
    {
        MINIRE_ERROR("Fatal error:\n{}", e.what());
    }
    catch(...)
    {
        MINIRE_ERROR("Fatal error: (unknown error)");
    }

    return EXIT_FAILURE;
}
