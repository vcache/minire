#pragma once

#include <codegen/factory.hpp>
#include <rasterizer/constants.hpp>

#include <glm/mat4x4.hpp>
#include <minire/material.hpp>

namespace minire::codegen::plugins
{
    class Skinning
    {
        static bool getHasSkins(Traits const & traits);

    public:
        template<typename Base>
        class Instance
        {
        public:
            explicit Instance(NamesCoder const & uniformCoder,
                              Traits const & traits)
                : _bonesMatrices(uniformCoder.find("bznkBones"))
                , _modelMatrix(uniformCoder.find("bznkModel"))
                , _hasSkin(getHasSkins(traits))
            {}

            void setSkinningUniforms(glm::mat4 const & modelTransform,
                                     material::SkinningVector const & skinningVector) const
            {
                Base const & base = static_cast<Base const &>(*this);
                if (_hasSkin)
                {
                    assert(skinningVector.size() <= rasterizer::Constants::kMaxBones);
                    base.setUniformByCode(_bonesMatrices, skinningVector);
                }
                else
                {
                    base.setUniformByCode(_modelMatrix, modelTransform);
                }
            }

        private:
            size_t _bonesMatrices;
            size_t _modelMatrix;
            bool   _hasSkin;
        };

    public:
        std::vector<std::string> const & uniforms() const;

        void setup(Traits const &,
                   inja::Environment &,
                   nlohmann::json &,
                   opengl::Program::AttribLocations &) const;
    };
}