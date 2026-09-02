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
            StbImage(std::string const & filename)
            {
                int width = 0, height = 0;
                int channels = 0; // 8-bit components per pixel

                _data = ::stbi_load(filename.c_str(), &width, &height, &channels, 0);
                postprocess(width, height, channels, filename);
            }

            explicit
            StbImage(unsigned char const * buffer, size_t length)
            {
                int width = 0, height = 0;
                int channels = 0; // 8-bit components per pixel

                _data = ::stbi_load_from_memory(buffer, length, &width, &height, &channels, 0);
                postprocess(width, height, channels, "(binary data)");
            }

            explicit
            StbImage(std::istream & istream)
            {
                int width = 0, height = 0;
                int channels = 0; // 8-bit components per pixel

                static constexpr ::stbi_io_callbacks kCallbacks
                {
                    .read = [](void * user, char * data, int size) -> int
                    {
                        auto & stream = *static_cast<std::istream *>(user);
                        stream.read(data, size);
                        return static_cast<int>(stream.gcount());
                    },

                    .skip = [](void * user, int n)
                    {
                        auto & stream = *static_cast<std::istream *>(user);
                        stream.clear(); // Clear EOF/fail flags before seeking
                        stream.seekg(n, std::ios_base::cur);
                    },

                    .eof = [](void * user) -> int
                    {
                        auto & stream = *static_cast<std::istream *>(user);
                        return stream.eof() ? 1 : 0;
                    }
                };

                _data = ::stbi_load_from_callbacks(&kCallbacks, &istream, &width, &height, &channels, 0);
                postprocess(width, height, channels, "(stream)");
            }

            ~StbImage() override
            {
                free();
            }

        private:
            void postprocess(int width, int height, int channels,
                             std::string const & source) try
            {
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
                MINIRE_THROW("failed to load image \"{}\": {}", source, e.what());
            }
            catch(...)
            {
                free();
                MINIRE_THROW("failed to load image \"{}\": (unknown expection)", source);
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

    models::Image::Sptr loadImage(unsigned char const * buffer, size_t length)
    {
        return std::make_shared<StbImage>(buffer, length);
    }

    models::Image::Sptr loadImage(std::istream & istream)
    {
        return std::make_shared<StbImage>(istream);
    }
}