#include <utils/gltf-node-transform.hpp>

#include <minire/errors.hpp>
#include <minire/formats/gltf.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <cassert>

namespace minire::utils
{
    models::Transform getNodeTransform(::tinygltf::Node const & node)
    {
        models::Transform transform;

        if (node.matrix.empty())
        {
            assert(transform._rotation == glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            if (!node.rotation.empty())
            {
                MINIRE_INVARIANT(node.rotation.size() == 4,
                                 "expected 4 components of rotation, but got {}: {}",
                                 node.rotation.size(), node.name);
                transform._rotation = glm::quat(node.rotation[3],  // w
                                                node.rotation[0],  // x
                                                node.rotation[1],  // y
                                                node.rotation[2]); // z
            }

            assert(transform._scale == glm::vec3(1.0f));
            if (!node.scale.empty())
            {
                MINIRE_INVARIANT(node.scale.size() == 3,
                                 "expected 3 components of scale, but got {}: {}",
                                 node.scale.size(), node.name);
                transform._scale = glm::vec3(node.scale[0],    // x
                                             node.scale[1],    // y
                                             node.scale[2]);   // z
            }

            assert(transform._translation == glm::vec3(0.0f));
            if (!node.translation.empty())
            {
                MINIRE_INVARIANT(node.translation.size() == 3,
                                 "expected 3 components of translation, but got {}: {}",
                                 node.translation.size(), node.name);
                transform._translation = glm::vec3(node.translation[0],    // x
                                                   node.translation[1],    // y
                                                   node.translation[2]);   // z
            }
        }
        else
        {
            MINIRE_INVARIANT(node.matrix.size() == 16,
                             "expected 16 components of transform matrix, but got {}: {}",
                             node.matrix.size(), node.name);
            // a column-major order is both in glTF and GLM
            transform.loadFromMatrix(glm::make_mat4x4(node.matrix.data()));
        }

        return transform;
    }
}