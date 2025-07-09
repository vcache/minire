#pragma once

#include <memory>

namespace minire::rasterizer
{
    class Ubo;

    class Coordinates
    {
    public:
        explicit Coordinates(Ubo const &,
                             float length = 42.0);

        void draw() const;

    private:
        class Impl;

        std::shared_ptr<Impl> _impl;
    };
}
