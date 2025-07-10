#pragma once

#include <minire/models/image.hpp>

#include <string>
#include <istream>

namespace minire::formats
{
    models::Image::Sptr loadImage(std::string const &);
}
