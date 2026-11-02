#pragma once

#include <minire/models/shadow-params.hpp>
#include <minire/utils/overloaded.hpp>

#include <cassert>
#include <variant>

namespace minire::rasterizer
{
    template<typename LightDatablock>
    void setupShadowParams(models::ShadowParams const & shadowParams,
                           LightDatablock & dst)
    {
        using namespace ::minire::models::shadow_params;

        dst._method = std::visit(utils::Overloaded
        {
            [&shadowParams](method::Standard const &)
            {
                assert(std::holds_alternative<std::monostate>(shadowParams._filter) ||
                       std::holds_alternative<filter::PCF>(shadowParams._filter));
                bool const pcf = std::holds_alternative<filter::PCF>(shadowParams._filter);
                return pcf ? 2 : 1;
            },
            [&dst](method::ESM const & esm)
            {
                dst._methodArg = esm._factor;
                return 3;
            },
            [&dst](method::LogESM const & logEsm)
            {
                dst._methodArg = logEsm._factor;
                return 4;
            },
        }, shadowParams._method);

        std::visit(utils::Overloaded
        {
            [&dst](bias::Constant const & bias)
            {
                dst._normalBiasMode = 0;
                dst._normalBiasBase = bias._biasBase;
                dst._normalBiasMax = bias._biasBase;
            },
            [&dst](bias::SlopScaled const & bias)
            {
                dst._normalBiasMode = 1;
                dst._normalBiasBase = bias._biasBase;
                dst._normalBiasMax = bias._maxBias;
            },
        }, shadowParams._normalBias);

        std::visit(utils::Overloaded
        {
            [&dst](bias::Constant const & bias)
            {
                dst._depthBiasMode = 0;
                dst._depthBiasBase = bias._biasBase;
                dst._depthBiasMax = bias._biasBase;
            },
            [&dst](bias::SlopScaled const & bias)
            {
                dst._depthBiasMode = 1;
                dst._depthBiasBase = bias._biasBase;
                dst._depthBiasMax = bias._maxBias;
            },
        }, shadowParams._depthBias);

        dst._smoothStepLeft = shadowParams._smoothStep.first;
        dst._smoothStepRight = shadowParams._smoothStep.second;
    }

    template<typename LightDatablock>
    void dropShadowParams(LightDatablock & dst)
    {
        dst._method = 0;
        dst._normalBiasMode = 0;
        dst._normalBiasBase = 0;
        dst._normalBiasMax = 0;
        dst._depthBiasMode = 0;
        dst._depthBiasBase = 0;
        dst._depthBiasMax = 0;
        dst._smoothStepLeft = 0;
        dst._smoothStepRight = 0;
    }
}
