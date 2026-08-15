#pragma once

#include <minire/application.hpp>
#include <minire/application/input-handler.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/hot-keys.hpp>
#include <minire/gui/overlay-controller.hpp>
#include <minire/gui/theme.hpp>
#include <minire/models/mouse-button.hpp>
#include <minire/models/system-cursor.hpp>

#include <list>
#include <optional>

namespace minire
{
    class GuiApplication
        : public Application
        , public gui::OverlayController
    {
    public:
        template<typename ... ApplicationArgs>
        GuiApplication(ApplicationArgs &&... applicationArgs)
            : Application(std::forward<ApplicationArgs>(applicationArgs)...)
            , _theme(makeBuiltinTheme())
        {
            guiPush("__base__");
        }

        template<typename ... ApplicationArgs>
        GuiApplication(std::unique_ptr<gui::Theme> && theme,
                       ApplicationArgs &&... applicationArgs)
            : Application(std::forward<ApplicationArgs>(applicationArgs)...)
            , _theme(std::move(theme))
        {
            guiPush("__base__");
        }

        ~GuiApplication() override;

        void setFocus(gui::Component::Sptr const & = {});

    protected:
        // NOTE: Descendant classes SHOULD not add "true" into the result,
        //       unless they really need to start a new epoch
        //       (since Gui don't need lerping);
        // NOTE: call GuiApplication::onStep() at the end of overrided method,
        //       so that, GuiApplication will revalide components immediately.
        bool onStep() override;

        // See onStep's notes.
        void onStart() override;

        // NOTE: Descendant classes SHOULD call GuiApplication::handle(*)
        //       before its own implementation.
        //       For details, see notes in minire/application/input-handler.hpp.
        bool handle(application::OnResize const &) override;
        bool handle(application::OnMouseWheel const & e) override;
        bool handle(application::OnMouseMove const & e) override;
        bool handle(application::OnMouseDown const & e) override;
        bool handle(application::OnMouseUp const & e) override;
        bool handle(application::OnKeyUp const & e) override;
        bool handle(application::OnKeyDown const & e) override;
        bool handle(application::OnTextInput const & e) override;

        gui::Component const & guiRoot() const;
        gui::Component & guiRoot();

        gui::Component const & guiTop() const;
        gui::Component & guiTop();
        std::string const & guiTopTag() const;

        gui::Component & guiPush(std::string const & tag,
                                 gui::OverlayInputMode const & = gui::overlay_input_mode::Active{});

        void guiPop();

        gui::Component & push(std::string const & tag,
                              gui::OverlayInputMode const & = gui::overlay_input_mode::Active{}) override;

        std::string const & topTag() const override;

        gui::Area const & topClientArea() const override;

        void pop() override;

        void startTextInput() override;
        void stopTextInput() override;

        void setClipboardText(std::string const &) override;
        void setPrimarySelection(std::string const &) override;
        std::string clipboardText() const override;
        std::string primarySelection() const override;

        void setSystemCursor(models::SystemCursor const) override;

        void unfocus() override;

        void set(::SDL_Scancode key, uint16_t mods,
                 gui::HotKeys::Handler handler);

        Sprite::Sptr make(minire::models::Sprite) override;
        Label::Sptr make(minire::models::Label) override;

        std::pair<glm::vec2 /* min size */, bool /* resizable */>
        measure(minire::models::sprite::Image const &) const override;

        glm::vec2 measure(text::FormattedString const &,
                          content::Id const &) const override;

        std::unique_ptr<utils::TextLayout> layout(text::FormattedString const &,
                                                  content::Id const &) const override;

        gui::Theme const & theme() const { assert(_theme); return *_theme; }

    private:
        struct Overlay
        {
            gui::Component::Sptr  _root;
            gui::Component::Wptr  _focused;
            gui::Component::Wptr  _hovered;
            gui::Component::Wptr  _toClick;
            models::MouseButton   _clickButton;
            std::string           _tag;
            gui::OverlayInputMode _inputMode;
        };

        Overlay & topOverlayForInput();

    private:
        void setHover(gui::Component::Sptr const &);

        gui::Component::Sptr hovered();

    private:
        std::unique_ptr<gui::Theme> makeBuiltinTheme();

    private:
        using MaybeOnMouseDown = std::optional<application::OnMouseDown>;

        std::unique_ptr<gui::Theme> _theme;

        // TODO: mouse-based events must be cancelled if some of
        //       SDL_WindowEvent have happened (alt+tab, minimization,
        //       unfocus, etc)
        std::list<Overlay>          _overlays;

        // TODO: hot keys must be cleared if some of SDL_WindowEvent
        //       have happened (alt+tab, minimization, unfocus, etc)
        // TODO: hot keys must fire an action not at KeyDown, but on
        //       a KeyUp and only if corresponding keys were pressed
        //       before (on an KeyDown event)
        gui::HotKeys                _hotKeys;

        MaybeOnMouseDown            _dragBegin;

        gui::Area                   _windowArea;
        float                       _mouseX = -1;
        float                       _mouseY = -1;
        bool                        _mouseUpdated = true;
        models::SystemCursor        _systemCursor = models::SystemCursor::kArrow;
    };
}
