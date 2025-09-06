#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/models/animations.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/scene-path.hpp>
#include <minire/models/scene-queries.hpp>
#include <minire/models/transform.hpp>

#include <limits>
#include <string>
#include <vector>

// TODO: these are more "commands" than "events"
namespace minire::events::controller
{
    // Generic scene control

    struct SceneReset {};

    struct SceneDispose
    {
        models::ScenePath _item;    // a node or a leaf
    };

    struct SceneActivateCamera
    {
        models::ScenePath _item;    // a node or a leaf
    };

    // Nodes and items builders

    struct SceneNewNode
    {
        std::string       _id;
        models::ScenePath _parent;
        models::Transform _origin;
        bool              _visible;
    };

    namespace impl
    {
        template<typename T>
        struct SceneNewLeaf
        {
            std::string       _id;
            models::ScenePath _parent;
            T                 _data;
            bool              _visible;
        };
    }

    using SceneNewMesh = impl::SceneNewLeaf<models::Mesh>;
    using SceneNewPointLight = impl::SceneNewLeaf<models::PointLight>;
    using SceneNewPerspectiveCamera = impl::SceneNewLeaf<models::PerspectiveCamera>;
    using SceneNewOrthographicCamera = impl::SceneNewLeaf<models::OrthographicCamera>;

    struct SceneNewFromSource
    {
        models::ScenePath _parent;
        content::Path     _source;
        bool              _visible;
    };

    // Attribute modifiers

    namespace impl
    {
        template<typename T>
        struct SceneItemModifier
        {
            models::ScenePath _item;
            T                 _attribute;
        };
    }

    using SceneSetParent = impl::SceneItemModifier<models::ScenePath>;
    using SceneSetVisibility = impl::SceneItemModifier<bool>;   // TODO: recursive version
    using SceneSetTransform = impl::SceneItemModifier<models::Transform>;
    using SceneSetPointLight = impl::SceneItemModifier<models::PointLight>;
    using SceneSetPerspectiveCamera = impl::SceneItemModifier<models::PerspectiveCamera>;
    using SceneSetOrthographicCamera = impl::SceneItemModifier<models::OrthographicCamera>;

    // Animations

    struct SceneNewAnimationSet
    {
        // NOTE: will replace any existing animations, i.e.
        //       this command has "assign" semantics.

        models::ScenePath    _containerNode;
        models::AnimationSet _animationSet;
    };

    struct ScenePlayAnimation
    {
        static constexpr size_t kInfinitely = std::numeric_limits<size_t>::max();

        models::AnimationId _animationId;
        models::ScenePath   _containerNode; // node
        size_t              _repeats;       // kInfinitely or a specific value,
                                            // for example, 1 for a single repeat
        float               _speedScale;    // 1 = normal
                                            // less than one = slowdown
                                            // more than one = speedup
    };

    struct SceneStopAnimation
    {
        models::ScenePath _containerNode;
    };

    // Scene queries

    struct SceneSetQuery
    {
        models::QueryEventFilter _eventFilter;
        models::QueryKind        _queryKind;
    };

    struct SceneUnsetQuery
    {
        models::QueryEventFilter _eventFilter;
        models::QueryKind        _queryKind;
    };

    struct SetRayCaster
    {
        bool _enabled;
    };
}
