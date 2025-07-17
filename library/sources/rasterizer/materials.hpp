#pragma once

#include <minire/material.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

namespace minire::rasterizer { class Ubo; }

namespace minire::rasterizer
{
    class Materials
    {
    public:
        void add(std::string const & materialKind,
                 material::Factory::Uptr);

    public:
        material::Program::Sptr build(material::Model const &,
                                      models::MeshFeatures const &,
                                      Ubo const &) const;

        material::Instance::Uptr instantiate(material::Model const &,
                                             models::MeshFeatures const &) const;

    private:
        material::Factory const & findFactory(std::string const &) const;

    private:
        using Factories = std::unordered_map<std::string,
                                             material::Factory::Uptr>;

        using Programs = std::unordered_map<std::string,
                                            material::Program::Sptr>;

        Factories        _factories;
        mutable Programs _programs; // TODO: should clean up it somehow
    };
}