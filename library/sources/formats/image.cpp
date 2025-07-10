#include <minire/formats/image.hpp>

#include <minire/errors.hpp>
#include <stb/stb_image.h>

namespace minire::formats
{
    namespace
    {
        struct StbImage : public models::Image
        {
            explicit
            StbImage(std::string const & filename) try
            {
                int width = 0, height = 0;
                int channels = 0; // 8-bit components per pixel

                _data = ::stbi_load(filename.c_str(), &width, &height, &channels, 0);

                MINIRE_INVARIANT(_data, "no data loaded: {}", ::stbi_failure_reason());
                MINIRE_INVARIANT(width > 0 && height > 0,
                                 "bad image size = {}x{}", width, height);

                _width = static_cast<size_t>(width);
                _height = static_cast<size_t>(height);

                switch(channels)
                {
                    case 1: // grey
                        _format = Format::kGrayscale;
                        break;
                    
                    case 2: // grey, alpha
                        MINIRE_THROW("unsupported channels count: {}", channels);
                    
                    case 3: // red, green, blue
                        _format = Format::kRGB;
                        break;
                    
                    case 4: // red, green, blue, alpha
                        _format = Format::kRGBA;
                        break;

                    default:
                        MINIRE_THROW("unexpected channels count: {}", channels);
                }

                _depth = Depth::k8;
                _signed = false;
            }
            catch(std::exception & e)
            {
                free();
                MINIRE_THROW("failed to load image \"{}\": {}", filename, e.what());
            }
            catch(...)
            {
                free();
                MINIRE_THROW("failed to load image \"{}\": (unknown expection)", filename);
            }

            ~StbImage() override
            {
                free();
            }

        private:
            void free()
            {
                if (_data)
                {
                    ::stbi_image_free(_data);
                }
                _data = nullptr;
            }
        };
    }

    models::Image::Sptr loadImage(std::string const & filename)
    {
        return std::make_shared<StbImage>(filename);
    }
}
