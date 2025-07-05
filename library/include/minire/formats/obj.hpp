#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <iostream>
#include <vector>

namespace minire::formats
{
    // SoA-style Obj storage
    // Face are _only_ trinagles
    struct Obj
    {
    public:
        using Vertex = glm::vec3;
        using Normal = glm::vec3;
        using Uv = glm::vec2;
        using FacePoint = uint32_t;
        using Faces = std::vector<FacePoint>;

        static_assert(sizeof(Vertex) == 3 * sizeof(float));
        static_assert(sizeof(Normal) == 3 * sizeof(float));
        static_assert(sizeof(Uv) == 2 * sizeof(float));

    public:
        // TODO: make sure it is aligned
        std::vector<Vertex> _vertices;
        std::vector<Normal> _normals;
        std::vector<Uv> _uvs;

        Faces _faceVertices;
        Faces _faceNormals;
        Faces _faceUvs;

    public:
        bool validate() const;

        bool haveNormals() const { return !_faceNormals.empty(); }

        bool haveUvs() const { return !_faceUvs.empty(); }

    public:
        size_t verticesBytes() const { return _vertices.size() * sizeof(Vertex); }

        size_t normalsBytes() const { return _normals.size() * sizeof(Normal); }

        size_t uvsBytes() const { return _uvs.size() * sizeof(Uv); }

    public:
        void const * vertices() const { return _vertices.data(); }

        void const * normals() const { return _normals.data(); }

        void const * uvs() const { return _uvs.data(); }
    };

    /*!
     * \note May throw a runtime exception.
     * \note Only supports obj-files with following features:
     *       - v, vn, vt, f, #
     *       - 3d v and vn
     *       - 2d vt
     *       - unlimited amount of point for a face
     */
    Obj loadObj(std::string const &);
    Obj loadObj(std::istream &);
    
    void saveObj(Obj const &, std::string const &);
    void saveObj(Obj const &, std::ostream &);
}
