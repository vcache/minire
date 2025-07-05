#pragma once

#include <gpu/render/drawable.hpp>
#include <gpu/render/label.hpp>

#include <glm/mat4x4.hpp>

#include <memory>
#include <unordered_map>

namespace minire::gpu::render
{
    class Fonts;

    class Labels
    {
    public:
        explicit Labels(Fonts const &);

    public:
        Label & allocate(std::string, int z = 0, bool visible = true);

        void deallocate(std::string const &);

        Label & get(std::string const &);

        Label const & get(std::string const &) const;

        void predraw(Drawable::PtrsList & out) const;

    private:
        using LabelPtr = std::unique_ptr<Label>;
        using Store = std::unordered_map<std::string, LabelPtr>;

        Fonts const & _fonts;
        Store         _store;
    };
}
