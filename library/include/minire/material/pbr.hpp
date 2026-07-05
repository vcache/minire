#pragma once

#include <minire/material.hpp>
#include <minire/models/pbr-material.hpp>

#include <memory>

namespace minire::material
{
    class Pbr
        : public Material
    {
    public:
        using Sptr = std::shared_ptr<Pbr>;
        using Wptr = std::weak_ptr<Pbr>;

        explicit Pbr(models::PbrMaterial const &);

        material::Program render() const override;
        void updateUserUniforms(material::UserUniforms &) const override;

    protected:
        std::string slugImpl() const override;

    protected:
        models::PbrMaterial const _model;
        mutable size_t            _revision;
    };
}
