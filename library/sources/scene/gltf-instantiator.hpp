#pragma once

namespace minire { class Scene; }
namespace minire::content { class Manager; }
namespace minire::events::controller { class SceneNewFromSource; }

namespace minire::scene
{
    void instantiateGltf(Scene & scene,
                         events::controller::SceneNewFromSource const &,
                         content::Manager &);
}
