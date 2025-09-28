#pragma once

#include <minire/content/id.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/text/symbol.hpp>
#include <minire/text/text-format.hpp>

#include <rasterizer/drawable.hpp>

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include <cstddef>
#include <memory>
#include <optional>

namespace minire::content { class Manager; }
namespace minire::models { class FontFace; }

namespace minire::rasterizer
{
    class Fonts;
    class Font;

    class Label : public Drawable
    {
    public:
        explicit Label(Fonts const &,
                       text::FormattedString const & text,
                       int z, bool visible);

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

        void setMaxSize(std::optional<glm::vec2> const & maxSize);

    public:
        // TODO: consider to put projection into UBO
        void draw(glm::mat4 const & projection) const override;

    private:
        void revalidate() const;

    private:
        class Program;
        class Buffer;

        friend class Buffer; // to access to Cursor

        using FontPtr = std::shared_ptr<Font const>;

        Fonts const &            _fonts;
        FontPtr                  _fontRegular;
        FontPtr                  _fontBold;
        FontPtr                  _fontItalic;
        text::FormattedString    _text;
        glm::vec2                _position;
        std::optional<glm::vec2> _maxSize;     

        Program const &          _program;
        std::unique_ptr<Buffer>  _buffer;
        mutable bool             _invalidated = true;

        bool                     _visible;
    };
}
