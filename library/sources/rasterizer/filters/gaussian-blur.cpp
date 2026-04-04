#include <rasterizer/filters/gaussian-blur.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>

#include <cassert>

// TODO: this is very sketchy implementation,
//       it needs to be refactored and tested,
//       SHOULDN'T be use in any productiom code

namespace minire::rasterizer::filters
{
    class GaussianBlur::Program
    {
        // TODO: generate to shaders for horizontal and vertical
        //       (avoid branching in the shader)
        static constexpr auto kComputeShader =
        R"(
            #version 430 core // TODO: maybe decrease minial version?

            layout(local_size_x = 128, local_size_y = 1) in;
            layout(binding = 0, r32f) uniform image2D inputImage;
            layout(binding = 1, r32f) uniform image2D outputImage;
            layout(location = 0) uniform bool horizontal;

            const int kRadius = 4;
            shared float cache[128 + kRadius * 2];
            const float w[5] = float[](0.20236, 0.179044, 0.124009, 0.067234, 0.028532);

            void main()
            {
                ivec2 dims = imageSize(inputImage);
                ivec2 gID = ivec2(gl_GlobalInvocationID.xy);
                uint lID = gl_LocalInvocationID.x;
                
                ivec2 coord = horizontal ? gID : gID.yx;

                cache[lID + kRadius] = imageLoad(inputImage, clamp(coord, ivec2(0), dims - 1)).r;

                if (lID < uint(kRadius))
                {
                    ivec2 dir = horizontal ? ivec2(1, 0) : ivec2(0, 1);
                    cache[lID] = imageLoad(inputImage, clamp(coord - kRadius * dir, ivec2(0), dims - 1)).r;
                    cache[lID + kRadius + 128] = imageLoad(
                        inputImage,
                        clamp(coord + 128 * dir, ivec2(0), dims - 1)).r;
                }

                memoryBarrierShared();
                barrier();

                float result = cache[lID + kRadius] * w[0];
                for (int i = 1; i <= kRadius; ++i)
                {
                    result += cache[lID + kRadius + i] * w[i];
                    result += cache[lID + kRadius - i] * w[i];
                }

                imageStore(outputImage, coord, vec4(result));
            }
        )";

    public:
        explicit Program()
            : _program({
                std::make_shared<opengl::Shader>(GL_COMPUTE_SHADER, kComputeShader)
            })
            , _horizontal(_program.getUniformLocation("horizontal"))
        {}

        void use(bool horizontal)
        {
            assert(_horizontal != -1);

            _program.use();
            _program.setUniform(_horizontal, horizontal);
        }

    private:
        opengl::Program _program;
        GLint const     _horizontal = -1;
    };

    GaussianBlur::GaussianBlur(size_t width,
                               size_t height,
                               GLint textureFormat)
        : _width(width)
        , _height(height)
        , _textureFormat(textureFormat)
        , _program(std::make_unique<Program>())
        , _temp(GL_TEXTURE_2D)
    {
        _temp.bind();

        // TODO: move glTexImage2D into Texture
        MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0, textureFormat,
                  _width, _height, 0, GL_RED, GL_FLOAT, nullptr);

        _temp.parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        _temp.parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    GaussianBlur::~GaussianBlur() = default; // because of forward decl of unique_ptr

    void GaussianBlur::perform(opengl::Texture const & target) const
    {
        assert(target.size() == std::make_pair(_width, _height));
        assert(target.internalFormat() == _textureFormat);
        assert(_program);

        _program->use(true);
        MINIRE_GL(glBindImageTexture, 0, target.id(), 0, GL_FALSE, 0, GL_READ_ONLY, _textureFormat);
        MINIRE_GL(glBindImageTexture, 1, _temp.id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, _textureFormat);
        MINIRE_GL(glDispatchCompute, (_width + 127) / 128, _height, 1);
        MINIRE_GL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        _program->use(false);
        MINIRE_GL(glBindImageTexture, 0, _temp.id(), 0, GL_FALSE, 0, GL_READ_ONLY, _textureFormat);
        MINIRE_GL(glBindImageTexture, 1, target.id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, _textureFormat);
        MINIRE_GL(glDispatchCompute, (_height + 127) / 128, _width, 1);
        MINIRE_GL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}