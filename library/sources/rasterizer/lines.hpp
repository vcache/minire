#pragma once

#include <vector>
#include <memory>

namespace minire::rasterizer
{
    class Ubo;

    class Lines
    {
    public:
        explicit Lines(Ubo const &);

        void update(std::vector<float> const &);

        void draw() const;

    private:
        class Impl;

        std::shared_ptr<Impl> _impl;
    };
}