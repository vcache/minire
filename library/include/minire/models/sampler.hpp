#pragma once

namespace minire::models
{
    struct Sampler
    {
        // TODO: shouldn't hardcode these values

        int _minFilter = 9987;  // GL_LINEAR_MIPMAP_LINEAR;
        int _magFilter = 9729;  // GL_LINEAR;
        int _wrapS = 10497;     // GL_REPEAT;
        int _wrapT = 10497;     // GL_REPEAT;

        bool operator==(Sampler const & o) const
        {
            return _minFilter == o._minFilter
                && _magFilter == o._magFilter
                && _wrapS == o._wrapS
                && _wrapT == o._wrapT;
        }
    };
}