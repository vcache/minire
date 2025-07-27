#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transformation.hpp>

#include <string>
#include <vector>

// TODO: these are more "commands" than "events"
namespace minire::events::controller
{
    // Generic scene control

    // TODO: move it into some common area
    using ScenePath = std::vector<std::string>;

    struct SceneReset {};

    struct SceneDispose
    {
        ScenePath _item;    // a node or a leaf
    };

    struct SceneActivateCamera
    {
        ScenePath _item;    // a node or a leaf
    };

    // Nodes and items builders

    struct SceneNewNode
    {
        std::string            _id;
        ScenePath              _parent;
        models::Transformation _origin;
        bool                   _visible;
    };

    namespace impl
    {
        template<typename T>
        struct SceneNewLeaf
        {
            std::string _id;
            ScenePath   _parent;
            T           _data;
            bool        _visible;
        };
    }

    using SceneNewMesh = impl::SceneNewLeaf<models::Mesh>;
    using SceneNewPointLight = impl::SceneNewLeaf<models::PointLight>;
    using SceneNewPerspectiveCamera = impl::SceneNewLeaf<models::PerspectiveCamera>;
    using SceneNewOrthographicCamera = impl::SceneNewLeaf<models::OrthographicCamera>;

    struct SceneNewFromSource
    {
        ScenePath     _parent;
        content::Path _source;
        bool          _visible;
    };

    // Attribute modifiers

    namespace impl
    {
        template<typename T>
        struct SceneItemModifier
        {
            ScenePath _item;
            T         _attribute;
        };
    }

    using SceneSetParent = impl::SceneItemModifier<ScenePath>;
    using SceneSetVisibility = impl::SceneItemModifier<bool>;   // TODO: recursive version
    using SceneSetTransformation = impl::SceneItemModifier<models::Transformation>;
    using SceneSetPointLight = impl::SceneItemModifier<models::PointLight>;
    using SceneSetPerspectiveCamera = impl::SceneItemModifier<models::PerspectiveCamera>;
    using SceneSetOrthographicCamera = impl::SceneItemModifier<models::OrthographicCamera>;
}
