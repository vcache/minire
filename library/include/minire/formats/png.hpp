#pragma once

#include <minire/models/image.hpp>

#include <string>
#include <istream>

namespace minire::formats
{
    models::Image::Sptr loadPng(std::string const &);
    models::Image::Sptr loadPng(char const *);
    models::Image::Sptr loadPng(std::istream &);
}
