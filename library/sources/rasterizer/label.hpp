#pragma once

#include <minire/content/id.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/text/symbol.hpp>
#include <minire/text/text-format.hpp>
#include <minire/utils/rect.hpp>

#include <rasterizer/drawable.hpp>

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include <cstddef>
#include <memory>
#include <optional>

namespace minire::content { class Manager; }
namespace minire::models { class FontFace; }
namespace minire::rasterizer::labels { class VertexBuffer; }

namespace minire::rasterizer
{
    class Fonts;
    class Font;

    class Label : public Drawable
    {
    public:
        explicit Label(Fonts const &,
                       text::FormattedString const & text,
                       size_t z, bool visible);

        ~Label(); // because of std::unique_ptr<Buffer>

    public:
        void setText(text::FormattedString const &);

        text::FormattedString const & text() const;

        void setVisible(bool v) { _visible = v; }

        bool visible() const { return _visible; }

    public:
        // in pixels, (0, 0) at left-bottom
        void setPosition(glm::vec2);

        void setFontFace(content::Id const & fontName,
                         content::Manager & contentManager);

        void setFontFace(models::FontFace const & fontData);

        void setClippingWindow(utils::MaybeRect const & clippingWindow);

    public:
        // TODO: consider to put projection into UBO
        void draw(glm::mat4 const & projection) const override;

    private:
        void revalidate() const;

    private:
        class Program;

        using FontPtr = std::shared_ptr<Font const>;
        using VertexBufferUptr = std::unique_ptr<labels::VertexBuffer>;

        Fonts const &            _fonts;
        FontPtr                  _fontRegular;
        FontPtr                  _fontBold;
        FontPtr                  _fontItalic;
        text::FormattedString    _text;
        glm::vec2                _position;
        utils::MaybeRect         _clippingWindow;

        Program const &          _program;
        mutable VertexBufferUptr _vertexBuffer;
        mutable bool             _invalidated = true;

        bool                     _visible;
    };
}
