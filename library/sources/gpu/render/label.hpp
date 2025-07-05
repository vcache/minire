#pragma once

#include <minire/content/id.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/text/symbol.hpp>
#include <minire/text/text-format.hpp>

#include <utils/grid.hpp>
#include <gpu/render/drawable.hpp>

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include <cstddef>
#include <memory>
#include <vector>
#include <array>

namespace minire::content { class Manager; }

namespace minire::models { class Font; }

namespace minire::gpu::render
{
    class Fonts;
    class Font;

    class Label : public Drawable
    {
    public:
        explicit Label(Fonts const &,
                       int z, bool visible);

        ~Label(); // because of std::unique_ptr<Buffer>

    public:
        text::Symbol const & at(size_t row, size_t column) const
        {
            return _symbols.at(row, column);
        }

        // TODO: don't invalidate symbols that are not really changed
        text::Symbol & at(size_t row, size_t column)
        {
            _invalidated = true;
            _dirty.emplace_back(row, column);
            return _symbols.at(row, column);
        }

        void set(size_t row, size_t column,
                 text::FormattedString const &);

        size_t rows() const { return _symbols.rows(); }

        size_t cols() const { return _symbols.cols(); }

        void setVisible(bool v) { _visible = v; }

        bool visible() const { return _visible; }

    public:
        // in pixels, (0, 0) at left-bottom
        void setPosition(float x, float y);

        void resize(size_t rows, size_t cols);

        void setFont(content::Id const & fontName,
                     content::Manager & contentManager);

        void setFont(models::Font const & fontData);

    public:
        void setCursor(size_t column, size_t row);

        void unsetCursor();

    public:
        // TODO: consider to put projection into UBO
        void draw(glm::mat4 const & projection) const override;

    private:
        void revalidate() const;

    private:
        class Program;
        class Buffer;

        friend class Buffer; // to access to Cursor

        struct Cursor
        {
            size_t _column = 0;
            size_t _row = 0;
            bool   _shown = false;
        };

        using Symbols = utils::Grid<text::Symbol>;
        // [(row, col)]
        using Dirty = std::vector<std::pair<size_t, size_t>>;

        using FontPtr = std::shared_ptr<Font const>;

        Fonts const &           _fonts;
        FontPtr                 _fontRegular;
        FontPtr                 _fontBold;
        FontPtr                 _fontItalic;
        Symbols                 _symbols;
        glm::vec2               _position;
        glm::vec2               _glyphSize;
        Cursor                  _cursor;

        Program const &         _program;
        std::unique_ptr<Buffer> _buffer;
        mutable Dirty           _dirty;
        mutable bool            _invalidated = true;

        bool                    _visible;
    };
}
