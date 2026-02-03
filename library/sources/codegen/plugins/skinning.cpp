#include <codegen/plugins/skinning.hpp>

#include <minire/errors.hpp>

#include <cassert>

namespace minire::codegen::plugins
{
    bool Skinning::getHasSkins(Traits const & traits)
    {
        assert((traits._attribLocations.jointsAttribute() >= 0) ==
               (traits._attribLocations.weightsAttribute() >= 0));
        return traits._attribLocations.jointsAttribute() >= 0;
    }

    std::vector<std::string> const & Skinning::uniforms() const
    {
        static std::vector<std::string> const kUniforms
        {
            "bznkBones",
            "bznkModel",
        };
        return kUniforms;
    }

    void Skinning::setup(Traits const & traits,
                         inja::Environment & env,
                         nlohmann::json & vars,
                         opengl::Program::AttribLocations & attribLocations) const
    {
        env.include_template("shaders/model-skinning-kit.incl",
                             env.parse(rasterizer::Constants::kModelSkinningKit));

        bool const hasSkins = getHasSkins(traits);

        {
            auto [_, inserted] = vars.emplace("kHasSkins", hasSkins);
            MINIRE_INVARIANT(inserted, "key already exists: kHasSkins");
        }

        {
            auto [_, inserted] = vars.emplace("kMaxBones", rasterizer::Constants::kMaxBones);
            MINIRE_INVARIANT(inserted, "key already exists: kMaxBones");
        }

        if (hasSkins)
        {
            attribLocations.emplace("bznkJoints", traits._attribLocations.jointsAttribute());
            attribLocations.emplace("bznkWeights", traits._attribLocations.weightsAttribute());
        }
    }
}
