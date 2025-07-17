#pragma once

namespace minire::models
{
    class MeshFeatures
    {
    public:
        explicit MeshFeatures(bool const hasUv,
                              bool const hasNormal,
                              bool const hasTangent)
            : _hasUv(hasUv)
            , _hasNormal(hasNormal)
            , _hasTangent(hasTangent)
        {}

    public:
        bool hasUv() const      { return _hasUv; }
        bool hasNormal() const  { return _hasNormal; }
        bool hasTangent() const { return _hasTangent; }

    private:
        bool const _hasUv;
        bool const _hasNormal;
        bool const _hasTangent;
    };
}
