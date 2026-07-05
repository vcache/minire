#include <minire/material.hpp>

#include <minire/errors.hpp>

#include <algorithm>

namespace minire::material
{
    std::string_view toString(ShaderType shaderType)
    {
        switch(shaderType)
        {
            case ShaderType::kVertex:         return "vertex";
            case ShaderType::kFragment:       return "fragment";
            case ShaderType::kGeometry:       return "geometry";
            case ShaderType::kTessControl:    return "tess-control";
            case ShaderType::kTessEvaluation: return "tess-evaluation";
            case ShaderType::kCompute:        return "compute";
            case ShaderType::__kCount__:      MINIRE_THROW("__kCount__ is illegal shader type");
        }
        MINIRE_THROW("bad enum type: {}", static_cast<int>(shaderType));
    }

    UserUniform * UserUniforms::find(std::string_view name)
    {
        auto it = std::ranges::find_if(_uniforms,
            [name](UserUniform const & i) { return i.name() == name; });
        return it != _uniforms.cend() ? &*(it) : nullptr;
    }
}