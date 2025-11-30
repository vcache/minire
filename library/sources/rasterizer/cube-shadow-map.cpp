#include <rasterizer/cube-shadow-map.hpp>

#include <opengl.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/mesh.hpp>
#include <scene.hpp>

#include <minire/errors.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>

#include <array>
#include <limits>

namespace minire::rasterizer
{
    namespace
    {
        static constexpr auto kVertShader =
        R"(
            #version 330 core

            // The location of this attrib is guaranteed by an internal convention.
            layout (location = 0) in vec3 bznkVertex;

            uniform mat4 bznkModelMatrix;

            void main()
            {
                gl_Position = bznkModelMatrix * vec4(bznkVertex, 1.0);
            }
        )";

        static constexpr auto kGeomShader =
        R"(
            #version 330 core

            layout (triangles) in;
            layout (triangle_strip, max_vertices=18) out;

            uniform mat4 bznkShadowMatrices[6];

            out vec4 FragPos; // FragPos from GS (output per emitvertex)

            void main()
            {
                for(int face = 0; face < 6; ++face)
                {
                    gl_Layer = face; // built-in variable that specifies to which face we render.
                    for(int i = 0; i < 3; ++i) // for each triangle vertex
                    {
                        FragPos = gl_in[i].gl_Position;
                        gl_Position = bznkShadowMatrices[face] * FragPos;
                        EmitVertex();
                    }
                    EndPrimitive();
                }
            }  
        )";

        static constexpr auto kFragShader =
        R"(
            #version 330 core

            in vec4 FragPos;

            uniform vec3 bznkLightPos;
            uniform float bznkFarPlane;

            void main()
            {
                // get distance between fragment and light source
                float lightDistance = length(FragPos.xyz - bznkLightPos);

                // map to [0;1] range by dividing by bznkFarPlane
                lightDistance = lightDistance / bznkFarPlane;

                // write this as modified depth
                gl_FragDepth = lightDistance;
            }
        )";
    }

    CubeShadowMap::CubeShadowMap(size_t size)
        : _size(size)
        , _texture(GL_TEXTURE_CUBE_MAP)
        , _program({std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
                    std::make_shared<opengl::Shader>(GL_GEOMETRY_SHADER, kGeomShader),
                    std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)})
        , _fbo()
        , _bznkModelMatrix(_program.getUniformLocation("bznkModelMatrix"))
        , _bznkShadowMatrices(_program.getUniformLocation("bznkShadowMatrices"))
        , _bznkLightPos(_program.getUniformLocation("bznkLightPos"))
        , _bznkFarPlane(_program.getUniformLocation("bznkFarPlane"))
    {
        MINIRE_INVARIANT(size <= std::numeric_limits<GLsizei>::max(),
                         "too huge size: {}", size);

        for (size_t i = 0; i < 6; ++i)
        {
            MINIRE_GL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                      _size, _size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }

        // TODO: GL_CLAMP_TO_BORDER ?
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    namespace
    {
        using CubeVPs = std::array<glm::mat4, 6>;

        CubeVPs buildVPs(glm::vec3 const & lightPosition,
                         float const near,
                         float const far)
        {
            // build projection matrix
            float const aspect = 1.0f; // width/height, but they are the same
            glm::mat4 const projection = glm::perspective(glm::radians(90.0f),
                                                          aspect, near, far);

            // build VP matrices
            return CubeVPs
            {
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(1.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(-1.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, 1.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, 1.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, -1.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, -1.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, 0.0f, 1.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, 0.0f, -1.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
            };
        }


        std::pair<float, float>
        calcNearFar(utils::FrustumVertices const & frustumVertices)
        {
            static float const kNear = 1.0f;

            glm::vec3 min = frustumVertices[0];
            glm::vec3 max = frustumVertices[0];
            for (glm::vec3 const & vertex : frustumVertices)
            {
                min = glm::min(min, vertex);
                max = glm::max(max, vertex);
            }

            float const far = glm::compMax(max - min);
            return std::make_pair(kNear, far);
        }
    }

    float CubeShadowMap::perform(Scene const & scene,
                                 glm::vec3 const & lightPosition,
                                 utils::FrustumVertices const & frustumVertices)
    {
        // setup GL mode flags
        MINIRE_GL(glEnable, GL_DEPTH_TEST);
        MINIRE_GL(glDepthFunc, GL_LESS);
        MINIRE_GL(glDepthMask, GL_TRUE);

        // bind cube map to the depth framebuffer
        _fbo.attach(_texture, GL_DEPTH_ATTACHMENT);
        MINIRE_GL(glDrawBuffer, GL_NONE);
        MINIRE_GL(glReadBuffer, GL_NONE);

        // setup canvas
        MINIRE_GL(glViewport, 0, 0, _size, _size);
        MINIRE_GL(glClear, GL_DEPTH_BUFFER_BIT);

        // setup program and it's uniforms
        auto const [near, far] = calcNearFar(frustumVertices);
        CubeVPs const lightVPs = buildVPs(lightPosition, near, far);
        _program.use();
        _program.setUniform(_bznkShadowMatrices, lightVPs);
        _program.setUniform(_bznkLightPos, lightPosition);
        _program.setUniform(_bznkFarPlane, far);

        MINIRE_GL(glActiveTexture, GL_TEXTURE0);
        _texture.bind();

        // perform drawing commands
        scene.cullModels(
            [this] (Mesh const & mesh, glm::vec3 const & /*emissiveFactor*/,
                    glm::mat4 const & transform)
            {
                _program.setUniform(_bznkModelMatrix, transform);
                mesh.drawBare();
            });

        // tidy up
        _fbo.unbind();
        return far;
    }
}