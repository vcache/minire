#include <minire/formats/png.hpp>

#include <minire/errors.hpp>

#include <png.h>

#include <cassert>
#include <cstdio>
#include <cerrno>

namespace minire::formats
{
/*
    TODO: avoid setjmp:

        If you would rather avoid the complexity of setjmp/longjmp issues,
        you can compile libpng with PNG_NO_SETJMP, in which case
        errors will result in a call to PNG_ABORT() which defaults to abort().

        You can #define PNG_ABORT() to a function that does something
        more useful than abort(), as long as your function does not
        return.

        TODO: maybe put throw routines into define PNG_ABORT
*/

    namespace
    {
        /*!
         * A RAII stdio FILE wrapper
         * */
        class File
        {
        public:
            File(char const *filename, char const *mode)
                : f(fopen(filename, mode))
            {}
        
            FILE * operator*() const
            {
                return f;
            }

            operator bool() const
            {
                return f != nullptr;
            }

            ~File() 
            {
                if (f) 
                { 
                    fclose(f);
                }
            }

            std::string error() const
            {
                return strerror(errno);
            }

        private:
            FILE * f;
        };

        void userReadData(const png_structp png_ptr,
                          png_bytep data,
                          size_t length) noexcept
        {
            try
            {
                png_voidp read_io_ptr = png_get_io_ptr(png_ptr);
                std::istream * is = reinterpret_cast<std::istream *>(read_io_ptr);
                if (!is) png_error(png_ptr, "std::istream is nullptr");
                if (!is->good()) png_error(png_ptr, "std::istream is not good");

                is->read(reinterpret_cast<char*>(data), length);
                std::streamsize const readen = is->gcount();
                if (readen == std::numeric_limits<std::streamsize>::max())
                {
                    png_error(png_ptr, "gcount return non-representable value");
                }
                if (readen < 0)
                {
                    png_error(png_ptr, "negative gcount");
                }
                if (!is->eof() && static_cast<size_t>(readen) != length)
                {
                    png_error(png_ptr, "failed to read from stream");
                }
            }
            catch(...)
            {
                png_error(png_ptr, "unknown exception while reading from istream");
            }
        }

        template<typename InitIoFunc>
        models::Image::Sptr loadPngGeneric(std::string sourceName,
                                           size_t headerReaded,
                                           InitIoFunc initIoFunc)
        {
            models::Image::Sptr result = std::make_shared<models::Image>();

            // Create png_struct
            png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                         nullptr,
                                                         nullptr,
                                                         nullptr);
            if (!png_ptr)
            {
                MINIRE_THROW("png_create_read_struct failed");
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr)
            {
                png_destroy_read_struct(&png_ptr, nullptr, nullptr);
                MINIRE_THROW("png_create_info_struct failed");
            }

            if (setjmp(png_jmpbuf(png_ptr)))
            {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                MINIRE_THROW("png loading failed from longjmp");
            }

            initIoFunc(png_ptr);
            png_set_sig_bytes(png_ptr, headerReaded);

            png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

            png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
            if (!row_pointers)
            {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                MINIRE_THROW("png_get_rows returned null");
            }

            auto interlace_type =  png_get_interlace_type(png_ptr, info_ptr);
            if (interlace_type != PNG_INTERLACE_NONE)
            {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                MINIRE_THROW("interlacing not supported"); // see pngpixel.c
            }

            result->_width      = png_get_image_width(png_ptr, info_ptr);
            result->_height     = png_get_image_height(png_ptr, info_ptr);
            png_byte color_type = png_get_color_type(png_ptr, info_ptr);
            
            auto bitDepth = png_get_bit_depth(png_ptr, info_ptr);
            switch(bitDepth)
            {
                case 8: result->_depth = models::Image::Depth::k8; break;
                case 16: result->_depth = models::Image::Depth::k16; break;
                case 32: result->_depth = models::Image::Depth::k32; break;
                case 64: result->_depth = models::Image::Depth::k64; break;
                default: MINIRE_THROW("unknown bit depth of \"{}\": {}",
                                      sourceName, bitDepth);
            }

            int componentCount = 0;
            switch(color_type)
            {
                case PNG_COLOR_TYPE_RGB:
                    result->_format = models::Image::Format::kRGB;
                    componentCount = 3;
                    break;

                case PNG_COLOR_TYPE_RGB_ALPHA:
                    result->_format = models::Image::Format::kRGBA;
                    componentCount = 4;
                    break;

                case PNG_COLOR_TYPE_GRAY:
                    result->_format = models::Image::Format::kGrayscale;
                    componentCount = 1;
                    break;

                case PNG_COLOR_TYPE_GRAY_ALPHA:
                case PNG_COLOR_TYPE_PALETTE:
                default:
                    MINIRE_THROW("unsupported color type: {}", int(color_type));
                    break;
            }

            int lineLength = result->_width * componentCount * result->bytesInComponent();
            int bytesCount = lineLength * result->_height;

            if (bytesCount > 0)
            {
                result->_data = new uint8_t[bytesCount];
                for(std::size_t line = 0; line < result->_height; ++line)
                {
                    png_bytep linePtr = row_pointers[line];
                    memcpy(result->_data + line * lineLength, linePtr, lineLength);
                }
            }
            else
            {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
                MINIRE_THROW("no pixels to load");
            }

            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

            return result;
        }
    }

    models::Image::Sptr loadPng(std::string const & filename)
    {
        return loadPng(filename.c_str());
    }

    models::Image::Sptr loadPng(std::istream & is)
    {
        // Read the header
        png_byte header[8];
        is.read(reinterpret_cast<char*>(header), sizeof header);
        auto const headerReaded = is.gcount();

        // Check format
        if (0 != png_sig_cmp(header, 0, headerReaded))
        {
            MINIRE_THROW("not a png readen from std::istream");
        }

        // Run the parser
        return loadPngGeneric(
            "(anon std::istream)",
            headerReaded,
            [&is](png_structp png_ptr)
            {
                assert(png_ptr);
                png_set_read_fn(png_ptr,
                                reinterpret_cast<png_voidp>(&is),
                                &userReadData);
            });
    }

    models::Image::Sptr loadPng(char const *filename)
    {
        // Open file
        File file(filename, "rb");
        if (!file)
        {
            auto __errno = errno;
            MINIRE_THROW("failed to open file '{}': {}", filename, strerror(__errno));
        }

        // Read header
        png_byte header[8];
        auto res = fread(header, 1, sizeof header, *file);
        if (res != sizeof header)
        {
            MINIRE_THROW("too short file: {}", filename);
        }

        // Check format
        bool is_png = 0 == png_sig_cmp(header, 0, sizeof header);
        if (!is_png)
        {
            MINIRE_THROW("not a png: {}", filename);
        }

        // Run the parser
        return loadPngGeneric(
            filename,
            sizeof header,
            [&file](png_structp png_ptr)
            {
                assert(png_ptr);
                png_init_io(png_ptr, *file);
            });
    }
}
