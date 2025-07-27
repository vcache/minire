#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>

namespace minire::models
{
    struct Mesh
    {
        content::Path         _source;

        // TODO: should it be a default (i.e. fallback) material or
        //       an override-material?
        material::Model::Sptr _defaultMaterial;
    };
}