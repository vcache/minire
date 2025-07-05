#pragma once

#include <minire/errors.hpp>

#include <cstddef>
#include <memory>

namespace minire::models
{
    struct Image
    {
        using Sptr = std::shared_ptr<Image>;

        enum class Format
        {
            kUnset     = 0,
            kRGB       = 1,
            kRGBA      = 2,
            kGrayscale = 3,
        };

        // Bits per pixel for a single component
        enum class Depth
        {
            k8  = 0, // 1 byte
            k16 = 1, // 2 bytes
            k32 = 2, // 4 bytes
            k64 = 3, // 8 bytes
        };

        size_t    _width = 0;                ///!< \note Image width, in pixels
        size_t    _height = 0;               ///!< \note Image height, in pixels
        uint8_t * _data = nullptr;           ///!< \note Pointer to pixels data
        Format    _format = Format::kUnset;  ///!< \note Format of pixels data
        Depth     _depth = Depth::k8;        ///!< \note Size of color component, in bits
        bool      _signed = false;           ///!< \note Signed or not color component

        ~Image()
        {
            delete []_data;
        }

        size_t bytesInComponent() const
        {
            switch(_depth)
            {
                case Depth::k8:  return 1;
                case Depth::k16: return 2;
                case Depth::k32: return 4;
                case Depth::k64: return 8;
            }
            MINIRE_THROW("unknown depth: {}", static_cast<int>(_depth));
        }

        size_t componentsCount() const
        {
            switch(_format)
            {
                case Format::kUnset:     MINIRE_THROW("format of an image is unset");
                case Format::kRGB:       return 3;
                case Format::kRGBA:      return 4;
                case Format::kGrayscale: return 1;
            }
            MINIRE_THROW("unknown depth: {}", static_cast<int>(_format));
        }

        size_t bytesInPixel() const
        {
            return bytesInComponent() * componentsCount();
        }

        size_t bytesInLine() const
        {
            return _width * bytesInPixel();
        }

        /**
         * \return A pointer to begining of the pixel.
         * */
        uint8_t * pixel(size_t x, size_t y)
        {
            return &_data[x * bytesInPixel() + y * bytesInLine()];
        }
    };
}
