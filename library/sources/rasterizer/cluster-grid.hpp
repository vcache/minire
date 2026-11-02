#pragma once

#include <minire/models/rendering-mode.hpp>
#include <minire/models/shadow-params.hpp>
#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <opengl/ssbo.hpp>
#include <opengl/ubo.hpp>
#include <rasterizer/binding-points.hpp>
#include <rasterizer/culled-objects.hpp>
#include <rasterizer/cube-shadow-map.hpp>
#include <rasterizer/shadow-params-helper.hpp>
#include <rasterizer/ubo/datablock.hpp>
#include <scene-impl/viewpoint.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace minire::rasterizer
{
    struct alignas(16) ClusterParamsDatablock
    {
        alignas(16) glm::uvec4 _gridDim = glm::uvec4(0);
        alignas(16) glm::vec4  _screenDim = glm::vec4(0);
        alignas(16) glm::mat4  _viewMatrix = glm::mat4(1.0f);
        alignas(4)  float      _nearK = 0.0f;
        alignas(4)  float      _logScale = 0.0f;
        alignas(4)  float      _linearScale = 0.0f;
        alignas(4)  float      _isPerspective = 1.0f;
    };
    static_assert(sizeof(ClusterParamsDatablock) % 16 == 0);

    struct ClusterCell
    {
        uint32_t _offset = 0;
        uint32_t _count = 0;
    };
    static_assert(sizeof(ClusterCell) == 8);

    namespace
    {
        static char const * kClusterCullComputeShader = R"(
            #version 430 core

            layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

            struct ClusterAABB
            {
                vec4 minVal;
                vec4 maxVal;
            };

            struct LightCullData
            {
                vec4 viewPosAndRadius;
                uint lightIndex;
                uint pad0;
                uint pad1;
                uint pad2;
            };

            struct ClusterCell
            {
                uint offset;
                uint count;
            };

            layout(std430, binding = 0) readonly buffer ClusterAabbBuffer
            {
                ClusterAABB b_clusterAabbs[];
            };

            layout(std430, binding = 1) readonly buffer LightCullDataBuffer
            {
                LightCullData b_lightCullData[];
            };

            layout(std430, binding = 2) writeonly buffer ClusterGridBuffer
            {
                ClusterCell b_clusterCells[];
            };

            layout(std430, binding = 3) writeonly buffer ClusterIndicesBuffer
            {
                uint b_clusterIndices[];
            };

            uniform uint u_numLights;
            uniform uint u_maxLightsPerCluster;
            uniform uint u_totalClusters;

            void main()
            {
                uint clusterIdx = gl_GlobalInvocationID.x;
                if (clusterIdx >= u_totalClusters)
                {
                    return;
                }

                vec3 boxMin = b_clusterAabbs[clusterIdx].minVal.xyz;
                vec3 boxMax = b_clusterAabbs[clusterIdx].maxVal.xyz;

                uint count = 0u;
                uint baseOffset = clusterIdx * u_maxLightsPerCluster;

                for (uint i = 0u; i < u_numLights; ++i)
                {
                    vec4 light = b_lightCullData[i].viewPosAndRadius;
                    vec3 lightPos = light.xyz;
                    float radius = light.w;

                    vec3 closest = clamp(lightPos, boxMin, boxMax);
                    vec3 diff = lightPos - closest;
                    if (dot(diff, diff) <= radius * radius)
                    {
                        if (count < u_maxLightsPerCluster)
                        {
                            b_clusterIndices[baseOffset + count] = b_lightCullData[i].lightIndex;
                            count++;
                        }
                    }
                }

                b_clusterCells[clusterIdx].offset = baseOffset;
                b_clusterCells[clusterIdx].count = count;
            }
        )";
    }

    class ClusterGrid
    {
        struct alignas(16) ClusterAABB
        {
            glm::vec4 _min = glm::vec4(0.0f);
            glm::vec4 _max = glm::vec4(0.0f);
        };
        static_assert(sizeof(ClusterAABB) == 32);

        struct alignas(16) LightCullData
        {
            glm::vec4 _viewPosAndRadius = glm::vec4(0.0f);
            uint32_t  _lightIndex = 0;
            uint32_t  _pad[3] = {0, 0, 0};
        };
        static_assert(sizeof(LightCullData) == 32);

    public:
        explicit ClusterGrid(models::rendering_mode::ClusteredForward const & config)
            : _config(config)
            , _cullProgram({
                std::make_shared<opengl::Shader>(GL_COMPUTE_SHADER, kClusterCullComputeShader)
            })
            , _numLightsUniform(_cullProgram.getUniformLocation("u_numLights"))
            , _maxLightsPerClusterUniform(_cullProgram.getUniformLocation("u_maxLightsPerCluster"))
            , _totalClustersUniform(_cullProgram.getUniformLocation("u_totalClusters"))
        {
        }

        void update(scene::Viewpoint const & viewpoint,
                    CulledPointLights const & culledPointLights)
        {
            glm::mat4 const & view = viewpoint.view();
            glm::mat4 const & proj = viewpoint.projection();
            float const screenW = static_cast<float>(viewpoint.width());
            float const screenH = static_cast<float>(viewpoint.height());

            bool const isPerspective = std::abs(proj[2][3] + 1.0f) < 0.001f;
            float zNear = 0.1f;
            float zFar = 1000.0f;

            if (isPerspective)
            {
                if (std::abs(proj[2][2] - 1.0f) > 0.0001f && std::abs(proj[2][2] + 1.0f) > 0.0001f)
                {
                    zNear = proj[3][2] / (proj[2][2] - 1.0f);
                    zFar = proj[3][2] / (proj[2][2] + 1.0f);
                }
            }
            else
            {
                if (std::abs(proj[2][2]) > 0.0001f)
                {
                    zNear = (proj[3][2] + 1.0f) / proj[2][2];
                    zFar = (proj[3][2] - 1.0f) / proj[2][2];
                }
            }

            if (zNear <= 0.0f || std::isnan(zNear)) zNear = 0.1f;
            if (zFar <= zNear || std::isnan(zFar)) zFar = zNear + 1000.0f;

            uint32_t const cx = _config._clustersX;
            uint32_t const cy = _config._clustersY;
            uint32_t const cz = _config._clustersZ;
            uint32_t const totalClusters = cx * cy * cz;

            if (_cachedProj != proj || _cachedWidth != screenW || _cachedHeight != screenH)
            {
                _cachedProj = proj;
                _cachedWidth = screenW;
                _cachedHeight = screenH;
                _clusterAabbs.resize(totalClusters);

                float const P00 = proj[0][0];
                float const P11 = proj[1][1];

                for (uint32_t iz = 0; iz < cz; ++iz)
                {
                    float dMin, dMax;
                    if (isPerspective)
                    {
                        dMin = zNear * std::pow(zFar / zNear, static_cast<float>(iz) / cz);
                        dMax = zNear * std::pow(zFar / zNear, static_cast<float>(iz + 1) / cz);
                    }
                    else
                    {
                        dMin = zNear + (zFar - zNear) * (static_cast<float>(iz) / cz);
                        dMax = zNear + (zFar - zNear) * (static_cast<float>(iz + 1) / cz);
                    }

                    for (uint32_t iy = 0; iy < cy; ++iy)
                    {
                        float const yNdc0 = 2.0f * static_cast<float>(iy) / cy - 1.0f;
                        float const yNdc1 = 2.0f * static_cast<float>(iy + 1) / cy - 1.0f;

                        for (uint32_t ix = 0; ix < cx; ++ix)
                        {
                            float const xNdc0 = 2.0f * static_cast<float>(ix) / cx - 1.0f;
                            float const xNdc1 = 2.0f * static_cast<float>(ix + 1) / cx - 1.0f;

                            uint32_t const clusterIdx = ix + iy * cx + iz * (cx * cy);
                            ClusterAABB & aabb = _clusterAabbs[clusterIdx];

                            if (isPerspective)
                            {
                                float const x0_near = xNdc0 * dMin / P00;
                                float const x1_near = xNdc1 * dMin / P00;
                                float const x0_far  = xNdc0 * dMax / P00;
                                float const x1_far  = xNdc1 * dMax / P00;

                                float const y0_near = yNdc0 * dMin / P11;
                                float const y1_near = yNdc1 * dMin / P11;
                                float const y0_far  = yNdc0 * dMax / P11;
                                float const y1_far  = yNdc1 * dMax / P11;

                                aabb._min = glm::vec4(std::min({x0_near, x1_near, x0_far, x1_far}),
                                                      std::min({y0_near, y1_near, y0_far, y1_far}),
                                                      -dMax, 0.0f);
                                aabb._max = glm::vec4(std::max({x0_near, x1_near, x0_far, x1_far}),
                                                      std::max({y0_near, y1_near, y0_far, y1_far}),
                                                      -dMin, 0.0f);
                            }
                            else
                            {
                                float const x0 = xNdc0 / P00;
                                float const x1 = xNdc1 / P00;
                                float const y0 = yNdc0 / P11;
                                float const y1 = yNdc1 / P11;

                                aabb._min = glm::vec4(std::min(x0, x1),
                                                      std::min(y0, y1),
                                                      -dMax, 0.0f);
                                aabb._max = glm::vec4(std::max(x0, x1),
                                                      std::max(y0, y1),
                                                      -dMin, 0.0f);
                            }
                        }
                    }
                }

                GLsizeiptr const aabbBytes = static_cast<GLsizeiptr>(_clusterAabbs.size() * sizeof(ClusterAABB));
                _clusterAabbSsbo.bufferData(aabbBytes, _clusterAabbs.data(), GL_STATIC_DRAW);
            }

            _pointLightsBuffer.clear();
            _pointLightsBuffer.reserve(culledPointLights.size());

            _lightCullData.clear();
            _lightCullData.reserve(culledPointLights.size());

            uint32_t shadowIdx = 0;
            for (size_t i = 0; i < culledPointLights.size(); ++i)
            {
                auto const & light = culledPointLights[i];
                ubo::PointLight gpuLight;
                gpuLight._position = glm::vec4(light._position, 1.0f);
                gpuLight._color = light._color;
                gpuLight._attenuation = light._attenuation;
                gpuLight._shadowMapFarPlane = light._shadowMapFarPlane;

                if (light._shadowMap)
                {
                    gpuLight._shadowMapIndex = shadowIdx++;
                    setupShadowParams(light._shadowMap->shadowParams(), gpuLight);
                }
                else
                {
                    gpuLight._shadowMapIndex = static_cast<uint32_t>(-1);
                    dropShadowParams(gpuLight);
                }
                _pointLightsBuffer.push_back(gpuLight);

                float const maxColor = std::max({light._color.r, light._color.g, light._color.b});
                float const intensity = light._color.a > 0.0f ? light._color.a : 1.0f;
                float const maxIntensity = maxColor * intensity;
                float const threshold = 5.0f / 256.0f;

                float radius = 50.0f;
                float const attC = light._attenuation.x;
                float const attL = light._attenuation.y;
                float const attQ = light._attenuation.z;

                if (maxIntensity > 0.0001f)
                {
                    float const c = attC - (maxIntensity / threshold);
                    if (c < 0.0f)
                    {
                        if (attQ > 0.00001f)
                        {
                            float const disc = attL * attL - 4.0f * attQ * c;
                            if (disc > 0.0f)
                            {
                                radius = (-attL + std::sqrt(disc)) / (2.0f * attQ);
                            }
                        }
                        else if (attL > 0.00001f)
                        {
                            radius = -c / attL;
                        }
                    }
                    else
                    {
                        radius = 0.1f;
                    }
                }
                if (radius <= 0.1f) radius = 0.1f;
                if (light._shadowMapFarPlane > 0.0f) radius = std::min(radius, light._shadowMapFarPlane);

                glm::vec3 const viewPos = glm::vec3(view * glm::vec4(light._position, 1.0f));
                float const depth = -viewPos.z;
                if (depth + radius < zNear || depth - radius > zFar)
                {
                    continue;
                }

                _lightCullData.push_back(LightCullData{
                    ._viewPosAndRadius = glm::vec4(viewPos, radius),
                    ._lightIndex = static_cast<uint32_t>(i),
                });
            }

            uint32_t const activeLightCount = static_cast<uint32_t>(_lightCullData.size());

            if (_pointLightsBuffer.empty())
            {
                _pointLightsBuffer.resize(1);
            }
            GLsizeiptr const pointLightsBytes = static_cast<GLsizeiptr>(_pointLightsBuffer.size() * sizeof(ubo::PointLight));
            if (_pointLightsSsbo.size() < pointLightsBytes)
            {
                _pointLightsSsbo.bufferData(pointLightsBytes, _pointLightsBuffer.data(), GL_DYNAMIC_DRAW);
            }
            else
            {
                _pointLightsSsbo.bufferSubData(0, pointLightsBytes, _pointLightsBuffer.data());
            }

            if (_lightCullData.empty())
            {
                _lightCullData.resize(1);
            }
            GLsizeiptr const lightCullBytes = static_cast<GLsizeiptr>(_lightCullData.size() * sizeof(LightCullData));
            if (_lightCullDataSsbo.size() < lightCullBytes)
            {
                _lightCullDataSsbo.bufferData(lightCullBytes, _lightCullData.data(), GL_DYNAMIC_DRAW);
            }
            else
            {
                _lightCullDataSsbo.bufferSubData(0, lightCullBytes, _lightCullData.data());
            }

            GLsizeiptr const clusterCellsBytes = static_cast<GLsizeiptr>(totalClusters * sizeof(ClusterCell));
            if (_clusterGridSsbo.size() < clusterCellsBytes)
            {
                _clusterGridSsbo.bufferData(clusterCellsBytes, nullptr, GL_DYNAMIC_DRAW);
            }

            uint32_t const maxIndices = totalClusters * _config._maxLightsPerCluster;
            GLsizeiptr const lightIndicesBytes = static_cast<GLsizeiptr>(std::max(1u, maxIndices) * sizeof(uint32_t));
            if (_clusterIndicesSsbo.size() < lightIndicesBytes)
            {
                _clusterIndicesSsbo.bufferData(lightIndicesBytes, nullptr, GL_DYNAMIC_DRAW);
            }

            ClusterParamsDatablock datablock;
            datablock._gridDim = glm::uvec4(cx, cy, cz, totalClusters);
            datablock._screenDim = glm::vec4(screenW, screenH, zNear, zFar);
            datablock._viewMatrix = view;
            datablock._nearK = zNear;
            datablock._logScale = isPerspective ? (static_cast<float>(cz) / std::log(zFar / zNear)) : 0.0f;
            datablock._linearScale = !isPerspective ? (static_cast<float>(cz) / (zFar - zNear)) : 0.0f;
            datablock._isPerspective = isPerspective ? 1.0f : 0.0f;

            _clusterParamsUbo.update(datablock);

            // Execute compute shader to assign lights to clusters
            _cullProgram.use();
            _cullProgram.setUniform(_numLightsUniform, activeLightCount);
            _cullProgram.setUniform(_maxLightsPerClusterUniform, _config._maxLightsPerCluster);
            _cullProgram.setUniform(_totalClustersUniform, totalClusters);

            _clusterAabbSsbo.bindBufferBase(0);
            _lightCullDataSsbo.bindBufferBase(1);
            _clusterGridSsbo.bindBufferBase(2);
            _clusterIndicesSsbo.bindBufferBase(3);

            GLuint const numGroupsX = (totalClusters + 63) / 64;
            MINIRE_GL(glDispatchCompute, numGroupsX, 1, 1);
            MINIRE_GL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void bind()
        {
            _pointLightsSsbo.bindBufferBase(kClusterPointLightsBindingPoint);
            _clusterGridSsbo.bindBufferBase(kClusterGridBindingPoint);
            _clusterIndicesSsbo.bindBufferBase(kClusterLightIndicesBindingPoint);
            _clusterParamsUbo.bindBufferBase(kClusterConfigBindingPoint);
        }

    private:
        models::rendering_mode::ClusteredForward _config;
        opengl::Program                          _cullProgram;
        GLint const                              _numLightsUniform;
        GLint const                              _maxLightsPerClusterUniform;
        GLint const                              _totalClustersUniform;

        opengl::SSBO                             _clusterAabbSsbo;
        opengl::SSBO                             _lightCullDataSsbo;
        opengl::SSBO                             _pointLightsSsbo;
        opengl::SSBO                             _clusterGridSsbo;
        opengl::SSBO                             _clusterIndicesSsbo;
        opengl::UBO<ClusterParamsDatablock>      _clusterParamsUbo;

        std::vector<ClusterAABB>                 _clusterAabbs;
        glm::mat4                                _cachedProj = glm::mat4(0.0f);
        float                                    _cachedWidth = 0.0f;
        float                                    _cachedHeight = 0.0f;

        std::vector<ubo::PointLight>             _pointLightsBuffer;
        std::vector<LightCullData>               _lightCullData;
    };
}
