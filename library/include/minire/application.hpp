#pragma once

// public headers
#include <minire/basic-controller.hpp>
#include <minire/errors.hpp>
#include <minire/events/application.hpp>
#include <minire/events/controller.hpp>
#include <minire/models/fps-camera.hpp>
#include <minire/sdl/gl-application.hpp>

// private headers
#include <gpu/render.hpp>
#include <scene.hpp>
#include <utils/epoch-interpolator.hpp>
#include <utils/lerpable.hpp>
#include <utils/viewpoint.hpp>

// STLs
#include <memory>
#include <string>
#include <type_traits>
#include <utility> // for std::forward

namespace minire::content { class Manager; }

namespace minire
{
    class Application : public sdl::GlApplication
    {
    public:
        Application(int width, int height,
                    std::string const & title,
                    content::Manager & contentManager);
        ~Application() override;

    public:
        template<typename Controller, typename... Args>
        Controller & setController(Args && ... args)
        {
            static_assert(std::is_base_of_v<BasicController, Controller>,
                          "Controller must be inherited from BasicController");
            MINIRE_INVARIANT(!_controller, "Controller cannot be re-set");

            auto controller = std::make_unique<Controller>(_applicationEvents);
            controller->run(events::application::OnResize{width(), height()},
                            std::forward<Args>(args)...);
            _controller = std::move(controller);
            return static_cast<Controller &>(*_controller);
        }
    private:
        void onResize(size_t width, size_t height) override;
        void onRender() override;

        void onMouseWheel(int dx, int dy) override;
        void onMouseMove(int absX, int absY, int relX, int relY,
                         bool left, bool middle, bool right,
                         bool x1, bool x2) override;
        void onMouseDown(int x, int y, bool doubleClick, models::MouseButton) override;
        void onMouseUp(int x, int y, bool doubleClick, models::MouseButton) override;
        void onKeyUp(::SDL_Keycode, ::SDL_Scancode,  uint16_t mod) override;
        void onKeyDown(::SDL_Keycode, ::SDL_Scancode, uint16_t mod) override;
        void onTextInput(std::string);

        void onFps(size_t fps, double mft) override;

    private:
        void handle(events::ControllerQueue::Store const &);

        void handle(events::controller::Quit const &);
        void handle(events::controller::NewEpoch const &);
        void handle(events::controller::MouseGrab const &);
        void handle(events::controller::DebugDrawsUpdate const &);
        void handle(events::controller::CreateSprite const &);
        void handle(events::controller::CreateNinePatch const &);
        void handle(events::controller::ResizeNinePatch const &);
        void handle(events::controller::MoveSprite const &);
        void handle(events::controller::VisibleSprite const &);
        void handle(events::controller::RemoveSprite const &);
        void handle(events::controller::BulkSetSpriteZOrders const &);
        void handle(events::controller::CreateLabel const &);
        void handle(events::controller::ResizeLabel const &);
        void handle(events::controller::MoveLabel const &);
        void handle(events::controller::SetCharLabel const &);
        void handle(events::controller::SetSymbolLabel const &);
        void handle(events::controller::UnsetCharLabel const &);
        void handle(events::controller::SetStringLabel const &);
        void handle(events::controller::SetLabelCursor const &);
        void handle(events::controller::UnsetLabelCursor const &);
        void handle(events::controller::SetLabelVisible const &);
        void handle(events::controller::SetLabelFonts const &);
        void handle(events::controller::RemoveLabel const &);
        void handle(events::controller::BulkSetLabelZOrders const &);
        void handle(events::controller::StartTextInput const &);
        void handle(events::controller::StopTextInput const &);
        void handle(events::controller::SceneReset const &);
        void handle(events::controller::SceneEmergeModel const &);
        void handle(events::controller::SceneEmergePointLight const &);
        void handle(events::controller::SceneUnmergeModel const &);
        void handle(events::controller::SceneUnmergePointLight const &);
        void handle(events::controller::SceneUpdateFpsCamera const &);
        void handle(events::controller::SceneUpdateModel const &);
        void handle(events::controller::SceneSetSelectedModels const &);
        void handle(events::controller::SceneUpdateLight const &);

        template<typename Event, typename... Args>
        void postEvent(Args && ...);

    private:
        using LerpableCamera = utils::Lerpable<models::FpsCamera>;
        
        content::Manager       & _contentManager;

        // render (view)
        gpu::Render              _gpuRender;
        Scene                    _scene;
        utils::Viewpoint         _viewpoint;
        LerpableCamera           _camera;
        bool                     _cameraActive = false;
        utils::EpochInterpolator _epochInterpolator;

        // controller (controller)
        BasicController::Uptr    _controller;

        // system
        events::ApplicationQueue _applicationEvents;
        size_t                   _frame = 0;
        size_t                   _frameBegin; // microseconds
        size_t                   _frameEnd;   // microseconds
    };
}
