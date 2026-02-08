#pragma once

#include <minire/basic-controller.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/content-view.hpp>
#include <minire/gui/hot-keys.hpp>
#include <minire/gui/overlay-controller.hpp>
#include <minire/gui/theme.hpp>
#include <minire/models/input-handler.hpp>
#include <minire/models/mouse-button.hpp>

#include <list>
#include <optional>
#include <utility>
#include <vector>

namespace minire::gui_controller { class ImageViewImpl; }
namespace minire::gui_controller { class TextViewImpl; }

namespace minire
{
    class GuiController
        : public BasicController
        , public gui::ContentViewFactory
        , public gui::OverlayController
    {
    public:
        template<typename ... Args>
        GuiController(Args &&... args)
            : BasicController(std::forward<Args>(args)...)
            , _theme(makeBuiltinTheme())
        {
            guiPush("__base__");
        }

        ~GuiController() override;

        void setFocus(gui::Component::Sptr const & = {});

        gui::ImageView::Sptr makeImageView(content::Id const & textureId,
                                            utils::Patch const & patch = std::monostate()) override;

        gui::TextView::Sptr makeTextView(text::FormattedString const & text,
                                          content::Id const & fontFace) override;

    protected:
        void step() override;

        void handle(events::application::OnResize const &)  override;

        // NOTE: Descendant classes SHOULD call GuiController::handle
        //       before its own implementation.
        //       For details, see notes in minire/models/input-handler.hpp.

        void handle(events::application::OnFps const &) override;
        bool handle(events::application::OnMouseWheel const &) override;
        bool handle(events::application::OnMouseMove const &) override;
        bool handle(events::application::OnMouseDown const &) override;
        bool handle(events::application::OnMouseUp const &) override;
        bool handle(events::application::OnKeyUp const &) override;
        bool handle(events::application::OnKeyDown const &) override;
        bool handle(events::application::OnTextInput const &) override;
        void handle(events::application::OnRayCaster const &) override;

        gui::Component const & guiRoot() const;
        gui::Component & guiRoot();

        gui::Component const & guiTop() const;
        gui::Component & guiTop();
        std::string const & guiTopTag() const;

        gui::Component & guiPush(
            std::string, models::InputHandler::Wptr fallbackHandler = {});

        void guiPop();

        gui::Component & push(std::string const & tag,
                               models::InputHandler::Wptr = {}) override;

        std::string const & topTag() const override;

        gui::Area const & topClientArea() const override;

        void pop() override;

        void set(::SDL_Scancode key, uint16_t mods,
                 gui::HotKeys::Handler handler);

    private:
        struct Overlay
        {
            gui::Component::Sptr      _root;
            gui::Component::Wptr      _focused;
            gui::Component::Wptr      _hovered;
            gui::Component::Wptr      _toClick;
            models::MouseButton        _clickButton;
            std::string                _tag;
            models::InputHandler::Wptr _fallbackHandler;
        };

        Overlay & topOverlay();

        Overlay const & topOverlay() const;

    private:
        void setHover(gui::Component::Sptr const &);

        gui::Component::Sptr hovered();

    private:
        void enqueueView(gui::ContentView::Sptr const &);

        void commitPendedViews();

        std::unique_ptr<gui::Theme> makeBuiltinTheme();

    private:
        using UncommittedViews = std::vector<gui::ContentView::Wptr>;

        std::unique_ptr<gui::Theme> _theme;
        UncommittedViews             _uncommittedViews;

        // TODO: mouse-based events must be cancelled if some of
        //       SDL_WindowEvent have happened (alt+tab, minimization,
        //       unfocus, etc)
        std::list<Overlay>           _overlays;

        // TODO: hot keys must be cleared if some of SDL_WindowEvent
        //       have happened (alt+tab, minimization, unfocus, etc)
        // TODO: hot keys must fire an action not at KeyDown, but on
        //       a KeyUp and only if corresponding keys were pressed
        //       before (on an KeyDown event)
        gui::HotKeys                _hotKeys;

        std::optional<events::application::OnMouseDown>
                                     _dragBegin;

        gui::Area                   _windowArea;
        float                        _mouseX = -1;
        float                        _mouseY = -1;
        bool                         _mouseUpdated = true;

        friend class gui_controller::ImageViewImpl;
        friend class gui_controller::TextViewImpl;
    };
}
