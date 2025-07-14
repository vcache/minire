#pragma once

#include <minire/models/scene-model.hpp>

#include <rasterizer/textures.hpp>
#include <opengl/vertex-buffer.hpp>

#include <glm/mat4x4.hpp>

#include <string>
#include <memory>

namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Ubo;

    class Model final
    {
    public:
        class Program
        {
        public:
            virtual ~Program() = default;
            virtual void use() const = 0;
            using Sptr = std::shared_ptr<Program>;
        };

    public:
        using Uptr = std::unique_ptr<Model>;

        explicit Model(content::Id const & id,
                       models::SceneModel const & sceneModel,
                       content::Manager & contentManager, 
                       Textures const &,
                       Ubo const &);

        // assuming that caller will "use" gl's program!
        void draw(Program const &,
                  glm::mat4 const &,
                  float const colorFactor) const;

        auto const & aabb() const { return _buffers.aabb(); }

    private:
        class Mapper
        {
        public:
            enum class Kind : size_t {
                kNone    = 0, // _must_ be zero
                kFloat   = 1,
                kVector3 = 2,
                kTexture = 3
                // _must_ be exactly 4 items
            };

            Mapper(size_t i): _kind(Kind::kNone), _index(i) {}
            Mapper(float v, size_t i): _kind(Kind::kFloat), _index(i), _float(v) {}
            Mapper(glm::vec3 const & v, size_t i): _kind(Kind::kVector3), _index(i), _vector3(v) {}
            Mapper(Textures::Texture::Sptr const & v, size_t i)
                : _kind(Kind::kTexture), _index(i), _texture(v) {}

            void setupUniforms(Program const &) const;

            Kind kind() const { return _kind; }

        private:
            Kind                    _kind;
            size_t                  _index;

            // TODO: these should be turned into a union or std::variant
            float                   _float;
            glm::vec3               _vector3;
            Textures::Texture::Sptr _texture;
        };

        static Mapper mapToMapper(Textures const & textures,
                                  models::SceneModel::Map const & map,
                                  size_t index);

    private:
        /*!
         * \note Guaranteeing that models w/ same flags()
         *       will generate same programs by makeProgram().
         */
        size_t flags() const { return _flags; }

        Program::Sptr makeProgram() const;

        size_t buildFlags() const;

    private:
        Ubo const &           _ubo;
        opengl::VertexBuffer  _buffers;

        Mapper                _albedo;
        Mapper                _metallic;
        Mapper                _roughness;
        Mapper                _ao;
        Mapper                _normals;

        size_t                _flags;

        friend class Models;
    };
}
