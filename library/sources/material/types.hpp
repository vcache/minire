#pragma once

#include <boost/container_hash/hash.hpp> // for hash_combine
#include <glm/mat4x4.hpp>

#include <functional> // For std::hash
#include <vector>

namespace minire::opengl { class Texture; }

namespace minire::material
{
    using TextureRefs = std::vector<opengl::Texture const *>;
    using SkinningVector = std::vector<glm::mat4>;

    class Locations
    {
    public:
        explicit Locations(int vertexAttribute = -1,
                           int uvAttribute = -1,
                           int normalAttribute = -1,
                           int tangentAttribute = -1,
                           int jointsAttribute = -1,
                           int weightsAttribute = -1)
            : _vertexAttribute(vertexAttribute)
            , _uvAttribute(uvAttribute)
            , _normalAttribute(normalAttribute)
            , _tangentAttribute(tangentAttribute)
            , _jointsAttribute(jointsAttribute)
            , _weightsAttribute(weightsAttribute)
            , _hash(calcHash())
        {}

        int vertexAttribute() const  { return _vertexAttribute;  }
        int uvAttribute() const      { return _uvAttribute;      }
        int normalAttribute() const  { return _normalAttribute;  }
        int tangentAttribute() const { return _tangentAttribute; }
        int jointsAttribute() const  { return _jointsAttribute;  }
        int weightsAttribute() const { return _weightsAttribute; }

        size_t hash() const { return _hash; }

        bool operator==(Locations const & o) const
        {
            if (_hash != o._hash) return false;

            return std::tie(  _vertexAttribute,    _uvAttribute,       _normalAttribute,
                              _tangentAttribute,   _jointsAttribute,   _weightsAttribute)
                == std::tie(o._vertexAttribute,  o._uvAttribute,     o._normalAttribute,
                            o._tangentAttribute, o._jointsAttribute, o._weightsAttribute);
        }

    private:
        size_t calcHash() const
        {
            size_t result = 0;
            boost::hash_combine(result, _vertexAttribute);
            boost::hash_combine(result, _uvAttribute);
            boost::hash_combine(result, _normalAttribute);
            boost::hash_combine(result, _tangentAttribute);
            boost::hash_combine(result, _jointsAttribute);
            boost::hash_combine(result, _weightsAttribute);
            return result;
        }

    private:
        int const    _vertexAttribute  = -1;
        int const    _uvAttribute      = -1;
        int const    _normalAttribute  = -1;
        int const    _tangentAttribute = -1;
        int const    _jointsAttribute  = -1;
        int const    _weightsAttribute = -1;
        size_t const _hash = 0;
    };
}

namespace std
{
    template<>
    struct hash<::minire::material::Locations>
    {
        size_t operator()(::minire::material::Locations const & v) const
        {
            return v.hash();
        }
    };
}
