#include <rasterizer/materials.hpp>

#include <minire/errors.hpp>

#include <opengl/program.hpp>
#include <rasterizer/ubo.hpp>

#include <fmt/format.h>

#include <cassert>

namespace minire::rasterizer
{
    void Materials::add(std::string const & kind, material::Factory::Uptr factory)
    {
        MINIRE_INVARIANT(factory, "no material factory provided for \"{}\"", kind);
        auto [_, inserted] = _factories.emplace(kind, std::move(factory));
        MINIRE_INVARIANT(inserted, "failed to add material factory for \"{}\"", kind);
    }

    material::Factory const & Materials::findFactory(std::string const & materialKind) const
    {
        auto it = _factories.find(materialKind);
        MINIRE_INVARIANT(it != _factories.cend(), "no such material factory: \"{}\"",
                         materialKind);
        assert(it->second);
        return *it->second;
    }

    material::Program::Sptr Materials::build(material::Model const & model,
                                             models::MeshFeatures const & features,
                                             Ubo const & ubo) const
    {
        material::Factory const & factory = findFactory(model.materialKind());
        std::string signature = fmt::format("{}/{}", model.materialKind(),
                                                     factory.signature(model, features));
        auto programIt = _programs.find(signature);

        if (_programs.cend() == programIt)
        {
            material::Program::Sptr program = factory.build(model, features);
            MINIRE_INVARIANT(program, "material factory returned empty program: \"{}\"",
                             signature);

            // Bind UBO here because it shouldn't be exposed into the public API
            opengl::Program const & glProgram = program->glProgram();
            glProgram.use();
            ubo.bindBufferRange(glProgram);

            auto [newIt, inserted] = _programs.emplace(signature, program);
            MINIRE_INVARIANT(inserted, "failed to cache material program: \"{}\"",
                             signature);
            programIt = newIt;
        }

        assert(_programs.cend() != programIt);
        assert(programIt->second);
        return programIt->second;
    }

    material::Instance::Uptr Materials::instantiate(material::Model const & model,
                                                    models::MeshFeatures const & features) const
    {
        material::Factory const & factory = findFactory(model.materialKind());
        return factory.instantiate(model, features);
    }
}