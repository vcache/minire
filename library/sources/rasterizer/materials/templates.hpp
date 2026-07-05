#pragma once

#include <string>

namespace minire::rasterizer::materials
{
    std::string const & preambleTemplate();
    std::string const & attributesTemplate();
    std::string const & transformTemplate();
    std::string const & uniformsTemplate();
    std::string const & uboTemplate();
}