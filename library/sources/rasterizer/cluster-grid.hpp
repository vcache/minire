#pragma once

#include <minire/models/rendering-mode.hpp>
#include <minire/models/shadow-params.hpp>
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

    class ClusterGrid
    {
        struct AABB
        {
            glm::vec3 _min = glm::vec3(0.0f);
            glm::vec3 _max = glm::vec3(0.0f);
        };

    public:
        explicit ClusterGrid(models::rendering_mode::ClusteredForward const & config)
            : _config(config)
        {
            uint32_t const totalClusters = _config._clustersX * _config._clustersY * _config._clustersZ;
            _clusterLights.resize(totalClusters);
            _clusterCells.resize(totalClusters);
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
                            AABB & aabb = _clusterAabbs[clusterIdx];

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

                                aabb._min.x = std::min({x0_near, x1_near, x0_far, x1_far});
                                aabb._max.x = std::max({x0_near, x1_near, x0_far, x1_far});
                                aabb._min.y = std::min({y0_near, y1_near, y0_far, y1_far});
                                aabb._max.y = std::max({y0_near, y1_near, y0_far, y1_far});
                                aabb._min.z = -dMax;
                                aabb._max.z = -dMin;
                            }
                            else
                            {
                                float const x0 = xNdc0 / P00;
                                float const x1 = xNdc1 / P00;
                                float const y0 = yNdc0 / P11;
                                float const y1 = yNdc1 / P11;

                                aabb._min.x = std::min(x0, x1);
                                aabb._max.x = std::max(x0, x1);
                                aabb._min.y = std::min(y0, y1);
                                aabb._max.y = std::max(y0, y1);
                                aabb._min.z = -dMax;
                                aabb._max.z = -dMin;
                            }
                        }
                    }
                }
            }

            _pointLightsBuffer.clear();
            _pointLightsBuffer.reserve(culledPointLights.size());

            _activeLights.clear();
            _activeLights.reserve(culledPointLights.size());

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
                if (radius <= 0.1f) radius = 0.1f;
                if (light._shadowMapFarPlane > 0.0f) radius = std::min(radius, light._shadowMapFarPlane);

                glm::vec3 const viewPos = glm::vec3(view * glm::vec4(light._position, 1.0f));
                _activeLights.push_back(LightInfo{
                    ._viewPos = viewPos,
                    ._radius = radius,
                    ._index = static_cast<uint32_t>(i),
                });
            }

            _clusterLights.resize(totalClusters);
            for (auto & cl : _clusterLights)
            {
                cl.clear();
            }

            auto sliceForDepth = [&](float depth) -> uint32_t
            {
                depth = std::clamp(depth, zNear, zFar);
                if (isPerspective)
                {
                    float const slice = std::floor(std::log(depth / zNear) * (cz / std::log(zFar / zNear)));
                    return std::clamp(static_cast<uint32_t>(std::max(0.0f, slice)), 0u, cz - 1);
                }
                else
                {
                    float const slice = std::floor((depth - zNear) / (zFar - zNear) * cz);
                    return std::clamp(static_cast<uint32_t>(std::max(0.0f, slice)), 0u, cz - 1);
                }
            };

            for (auto const & light : _activeLights)
            {
                float const depth = -light._viewPos.z;
                if (depth + light._radius < zNear || depth - light._radius > zFar)
                {
                    continue;
                }

                uint32_t const minSlice = sliceForDepth(depth - light._radius);
                uint32_t const maxSlice = sliceForDepth(depth + light._radius);

                float const rSq = light._radius * light._radius;

                for (uint32_t iz = minSlice; iz <= maxSlice; ++iz)
                {
                    uint32_t const sliceBase = iz * (cx * cy);
                    for (uint32_t iy = 0; iy < cy; ++iy)
                    {
                        uint32_t const rowBase = sliceBase + iy * cx;
                        for (uint32_t ix = 0; ix < cx; ++ix)
                        {
                            uint32_t const clusterIdx = rowBase + ix;
                            if (_clusterLights[clusterIdx].size() >= _config._maxLightsPerCluster)
                            {
                                continue;
                            }

                            AABB const & aabb = _clusterAabbs[clusterIdx];

                            float distSq = 0.0f;
                            if (light._viewPos.x < aabb._min.x)
                            {
                                float const d = aabb._min.x - light._viewPos.x;
                                distSq += d * d;
                            }
                            else if (light._viewPos.x > aabb._max.x)
                            {
                                float const d = light._viewPos.x - aabb._max.x;
                                distSq += d * d;
                            }

                            if (light._viewPos.y < aabb._min.y)
                            {
                                float const d = aabb._min.y - light._viewPos.y;
                                distSq += d * d;
                            }
                            else if (light._viewPos.y > aabb._max.y)
                            {
                                float const d = light._viewPos.y - aabb._max.y;
                                distSq += d * d;
                            }

                            if (light._viewPos.z < aabb._min.z)
                            {
                                float const d = aabb._min.z - light._viewPos.z;
                                distSq += d * d;
                            }
                            else if (light._viewPos.z > aabb._max.z)
                            {
                                float const d = light._viewPos.z - aabb._max.z;
                                distSq += d * d;
                            }

                            if (distSq <= rSq)
                            {
                                _clusterLights[clusterIdx].push_back(light._index);
                            }
                        }
                    }
                }
            }

            _clusterCells.resize(totalClusters);
            _lightIndices.clear();
            _lightIndices.reserve(totalClusters * 8);

            for (uint32_t c = 0; c < totalClusters; ++c)
            {
                _clusterCells[c]._offset = static_cast<uint32_t>(_lightIndices.size());
                _clusterCells[c]._count = static_cast<uint32_t>(_clusterLights[c].size());
                _lightIndices.insert(_lightIndices.end(), _clusterLights[c].begin(), _clusterLights[c].end());
            }

            if (_lightIndices.empty())
            {
                _lightIndices.push_back(0);
            }
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

            GLsizeiptr const clusterCellsBytes = static_cast<GLsizeiptr>(_clusterCells.size() * sizeof(ClusterCell));
            if (_clusterGridSsbo.size() < clusterCellsBytes)
            {
                _clusterGridSsbo.bufferData(clusterCellsBytes, _clusterCells.data(), GL_DYNAMIC_DRAW);
            }
            else
            {
                _clusterGridSsbo.bufferSubData(0, clusterCellsBytes, _clusterCells.data());
            }

            GLsizeiptr const lightIndicesBytes = static_cast<GLsizeiptr>(_lightIndices.size() * sizeof(uint32_t));
            if (_clusterIndicesSsbo.size() < lightIndicesBytes)
            {
                _clusterIndicesSsbo.bufferData(lightIndicesBytes, _lightIndices.data(), GL_DYNAMIC_DRAW);
            }
            else
            {
                _clusterIndicesSsbo.bufferSubData(0, lightIndicesBytes, _lightIndices.data());
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
        opengl::SSBO                             _pointLightsSsbo;
        opengl::SSBO                             _clusterGridSsbo;
        opengl::SSBO                             _clusterIndicesSsbo;
        opengl::UBO<ClusterParamsDatablock>      _clusterParamsUbo;

        std::vector<AABB>                        _clusterAabbs;
        glm::mat4                                _cachedProj = glm::mat4(0.0f);
        float                                    _cachedWidth = 0.0f;
        float                                    _cachedHeight = 0.0f;

        struct LightInfo
        {
            glm::vec3 _viewPos;
            float     _radius;
            uint32_t  _index;
        };

        std::vector<LightInfo>                   _activeLights;
        std::vector<ubo::PointLight>             _pointLightsBuffer;
        std::vector<std::vector<uint32_t>>       _clusterLights;
        std::vector<ClusterCell>                 _clusterCells;
        std::vector<uint32_t>                    _lightIndices;
    };
}
