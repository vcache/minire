#pragma once

#include <minire/application/input-handler.hpp>
#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/label.hpp>
#include <minire/models/vertex-buffer.hpp>
#include <minire/scene.hpp>
#include <minire/sdl/gl-application.hpp>
#include <minire/sprite.hpp>

#include <glm/vec2.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility> // for std::forward

namespace minire::content { class Manager; }
namespace minire::utils { class RayCaster; }
namespace minire::utils { class TextLayout; }

namespace minire
{
    class Rasterizer;
    class SceneImpl;

    class Application
        : public sdl::GlApplication
        , public application::InputHandler
    {
    public:
        // TODO: set max FPS
        Application(int width, int height,
                    std::string const & title,
                    content::Manager & contentManager,
                    models::MsaaParams const & = {});
        ~Application() override;

    protected:
        // Descendants should call Application::on* at the begging of their
        // own overrides.
        void onStart() override;
        virtual bool onStep() { return true; }

    protected:
        using RayCasterSptr = std::shared_ptr<utils::RayCaster>;
        RayCasterSptr const & rayCaster() const;

        void setRayCaster(bool enabled);

        void debugDrawsUpdate(std::vector<float> const & linesBuffer);

        void newResourceLayer(std::string const & name);
        void disposeResourceLayer(std::string const & name);
        void contentManagerCleanup(bool const force);

        // Note that this command won't perform inter-frame lerping,
        // therefore, animated meshes might appear jerky if Controller's FPS
        // is lower that Raterizer FPS.
        // TODO: allow anonymouse VertexBuffer
        void createVertexBuffer(content::Id const & id,             // The VertexBuffer will be available by a path:
                                                                    //  content::path::Special::kVertexBuffers/{_id}
                                models::VertexBuffer vertexBuffer,  // Controller MUST NOT modify provided buffers,
                                                                    // otherwise thread-safety will be broken.
                                bool const override);               // If true, an existing one will be rewritten,
                                                                    // otherwise, a runtime-error will be generated.
        void disposeVertexBuffer(content::Id const & id);

        // TODO: sprites and labels are also require some kind of "2D Scene"
        //       (for a tree of transforms/visibility/etc). Gui should be built upon this "2D Scene".

        // A 'name' can be optional. If a 'name' is empty(), a sprite will anonymouse.
        Sprite::Sptr make(models::Sprite model) { return make({}, std::move(model)); }
        Sprite::Sptr make(std::string const & name, models::Sprite);
        Sprite::Sptr findSprite(std::string const & name);
        Sprite::Sptr detachSprite(std::string const & name); // detached Sprite cannot be re-attached

        // A 'name' can be optional. If a 'name' is empty(), a sprite will anonymouse.
        Label::Sptr make(models::Label model) { return make({}, std::move(model)); }
        Label::Sptr make(std::string const & name, models::Label);
        Label::Sptr findLabel(std::string const & name);
        Label::Sptr detachLabel(std::string const & name);

        glm::vec2 measure(text::FormattedString const & text,
                          content::Id const & fontFace) const;

        std::pair<glm::vec2 /* min size */, bool /* resizable */>
        measure(models::sprite::Image const & image) const;

        std::unique_ptr<utils::TextLayout> layout(text::FormattedString const & text,
                                                  content::Id const & fontFace) const;

        utils::Aabb measure(content::Path const & path) const;

        // TODO: a Scene should be detachable. So that user might have several prepared scenes
        //       which can be switched to an active.
        // TODO: and maybe several scenes can be active?
        Scene & scene() const;

        content::Manager & contentManager() const { return _contentManager; }

        size_t frame() const { return _frame; }
        double frameTime() const { return _frameTime; }
        double absoluteTime() const { return _absoluteTime; }

    private:
        void startEpoch(); // for lerping

        void onRender() override;

        void onResize(size_t width, size_t height) override;
        void onMouseWheel(int dx, int dy, uint32_t dir, ::SDL_Keymod) override;
        void onMouseMove(int absX, int absY, int relX, int relY,
                         bool left, bool middle, bool right, bool x1, bool x2) override;
        void onMouseDown(int x, int y, bool doubleClick, models::MouseButton) override;
        void onMouseUp(int x, int y, bool doubleClick, models::MouseButton) override;
        void onTextInput(std::string const &) override;
        void onKeyUp(::SDL_Keycode, ::SDL_Scancode, uint16_t mod) override;
        void onKeyDown(::SDL_Keycode, ::SDL_Scancode, uint16_t mod) override;

    private:
        content::Manager          & _contentManager;

        // render (view)
        std::unique_ptr<Rasterizer> _rasterizer;
        std::unique_ptr<SceneImpl>  _scene;

        // controller (controller)
        double                      _epochTime = 0;     // seconds
        double                      _epochDuration = 0; // seconds
        size_t                      _epochNumber = 0;
        size_t                      _epochBegin = 0;    // microseconds

        // system
        size_t                      _frame = 0;
        size_t                      _frameBegin;        // microseconds
        double                      _frameTime = 0;
        double                      _absoluteTime = 0;  // seconds

        // scene queries
        bool                        _rayCasterEnabled = false;
        mutable size_t              _rayCasterRevision = 0;
        mutable size_t              _rayCasterLastEpoch = 0;
        mutable RayCasterSptr       _rayCaster;

        // instrumentations
        size_t                      _pedanticGlCounter = 0;
    };
}
