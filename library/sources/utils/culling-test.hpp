#pragma once

#include <glm/mat4x4.hpp>

namespace minire::utils
{
    class Aabb;
    class ViewFrustum;

    bool cullingTest(Aabb const & aabb,
                     ViewFrustum const & viewFrustum,
                     glm::mat4 const & globalTransform);
}