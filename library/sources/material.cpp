#include <minire/material.hpp>

#include <minire/errors.hpp>

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
}
