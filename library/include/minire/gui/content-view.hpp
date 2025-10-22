#pragma once

#include <minire/content/id.hpp>
#include <minire/gui/area.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace minire::gui
{
    class ContentInvalidator
    {
    public:
        virtual ~ContentInvalidator() = default;

        virtual void invalidateContent() = 0;

        using Sptr = std::shared_ptr<ContentInvalidator>;
        using Wptr = std::weak_ptr<ContentInvalidator>;
    };

    class ContentView
        : public std::enable_shared_from_this<ContentView>
    {
    public:
        using Sptr = std::shared_ptr<ContentView>;
        using Wptr = std::weak_ptr<ContentView>;

        virtual ~ContentView() = default;

        virtual void initialize() = 0;

        virtual std::pair<glm::vec2, bool> measure() const = 0;

        virtual void setContentPosition(float left, float top) = 0;

        virtual void setContentSize(float width, float height) = 0;

        virtual void setContentArea(Area const &) = 0;

        virtual size_t onZOrderChanged(size_t zOffset) = 0;

        virtual void setVisible(bool) = 0;

        virtual void commit() = 0;

        void setContentInvalidator(ContentInvalidator::Sptr const & contentInvalidator)
        {
            _contentInvalidator = contentInvalidator;
        }

    protected:
        // NOTE: This must be called ONLY when the content itself is changed,
        //       for example, when changed its measurements.
        //       Definetely shouldn't be called in setVisible or setContent(Position/Size/Area),
        //       because it might cause infinite revalidation loop.
        void invalidate()
        {
            if (auto contentInvalidator = _contentInvalidator.lock();
                contentInvalidator)
            {
                contentInvalidator->invalidateContent();
            }
        }

    private:
        ContentInvalidator::Wptr _contentInvalidator;
    };

    class ImageView
        : public ContentView
    {
    public:
        using Sptr = std::shared_ptr<ImageView>;
        using Wptr = std::weak_ptr<ImageView>;
    };

    class TextView
        : public ContentView
    {
    public:
        using Sptr = std::shared_ptr<TextView>;
        using Wptr = std::weak_ptr<TextView>;

        virtual void setContent(content::Id const & fontFace,
                                text::FormattedString const & text,
                                bool enableClipping = false) = 0;

        virtual void setText(text::FormattedString const & text) = 0;

        virtual void setFontFace(content::Id const & fontFace) = 0;

        virtual void setEnableClipping(bool const enableClipping) = 0;
    };

    class ContentViewFactory
    {
    public:
        virtual ~ContentViewFactory() = default;

        virtual ImageView::Sptr makeImageView(content::Id const & textureId,
                                              utils::Patch const & patch) = 0;

        virtual TextView::Sptr makeTextView(text::FormattedString const & text,
                                            content::Id const & fontFace,
                                            bool const enableClipping = false) = 0;
    };
}
