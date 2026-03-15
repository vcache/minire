#pragma once

#include <minire/label.hpp>

#include <rasterizer/drawable.hpp>

#include <glm/mat4x4.hpp>

#include <list>
#include <memory>
#include <unordered_map>

namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Fonts;

    class Labels
    {
    public:
        explicit Labels(Fonts const &,
                        content::Manager &);

        ~Labels();

        Label::Sptr make(std::string const & name,
                         models::Label model);

        Label::Sptr const & find(std::string const & name) const;

    public:
        void predraw(Drawable::PtrsList & out) const;

    private:
        class LabelImpl;
        class Program;

        using LabelImplSptr = std::shared_ptr<LabelImpl>;
        using Heap = std::list<LabelImplSptr>;
        using Index = std::unordered_map<std::string, LabelImplSptr>;

        Fonts const            & _fonts;
        content::Manager       & _contentManager;
        std::unique_ptr<Program> _program;
        mutable Heap             _heap;
        mutable Index            _index;
    };
}
