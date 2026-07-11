#pragma once

namespace minire::rasterizer
{
    class LocationsAllocator
    {
    public:
        LocationsAllocator(size_t initialLocation = 0)
            : _nextLocation(initialLocation)
        {}

        // float, int, vec2, vec3, vec4: 1 slot
        // mat2: 2 slots
        // mat3: 3 slots
        // mat4: 4 slots
        // Arrays (Type[N]): N * slots of (N)
        size_t allocate(size_t const slotsCount)
        {
            size_t const result = _nextLocation;
            _nextLocation += slotsCount;
            return result;
        }

    private:
        size_t _nextLocation = 0;
    };
}