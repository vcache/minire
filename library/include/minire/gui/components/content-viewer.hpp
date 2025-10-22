#pragma once

#include <minire/errors.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/content-view.hpp>

#include <cassert>

namespace minire::gui::components
{
    template<typename ContentView>
    class ContentViewer
        : public Component
    {
    public:
        using Sptr = std::shared_ptr<ContentViewer>;
        using Wptr = std::weak_ptr<ContentViewer>;

        explicit ContentViewer(std::string const & id,
                               Theme const & theme,
                               OverlayController & overlayController,
                               ContentView::Sptr const & content)
            : Component(id, theme, overlayController)
            , _content(content)
        {
            MINIRE_INVARIANT(_content, "no Content provided");

            if (auto [_, isResizable] = _content->measure();
                !isResizable)
            {
                horizontal()->_dimension = dimension::Content{};
                vertical()->_dimension = dimension::Content{};
            }
        }

        ContentView & content()
        {
            assert(_content);
            return *_content;
        }

        ContentView const & content() const
        {
            assert(_content);
            return *_content;
        }

        std::optional<std::pair<float, float>> measureContent() const override
        {
            assert(_content);
            auto [size, _] = _content->measure();
            return std::make_pair(size.x, size.y);
        }

    protected:
        void initialize() override
        {
            assert(_content);
            _content->setContentInvalidator(shared_from_this());
        }

        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & clientArea) override
        {
            assert(_content);

            _content->setVisible(effectiveVisible);
            _content->setContentArea(clientArea);

            return _content->onZOrderChanged(zOffset);
        }

    private:
        ContentView::Sptr _content;
    };
}
