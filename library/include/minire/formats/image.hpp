#pragma once

#include <minire/models/image.hpp>

#include <string>
#include <istream>

namespace minire::formats
{
    models::Image::Sptr loadImage(std::string const & filename);
    models::Image::Sptr loadImage(unsigned char const * buffer, size_t length);
    models::Image::Sptr loadImage(std::istream & istream);
}
