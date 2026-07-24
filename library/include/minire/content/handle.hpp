#pragma once

#include <minire/content/path.hpp>
#include <minire/errors.hpp>
#include <minire/utils/demangle.hpp>

#include <fmt/format.h>

#include <cassert>
#include <functional> // For std::hash
#include <string>
#include <type_traits>

namespace minire::content
{
    // A Handle is a synonym for Path. They are interchangeable,
    // but a Handle is supposed to be more efficient (implements
    // "string interning"-like approach, but for Path).
    // This class is explicitly thread safe.
    //
    // Although it is legal to convert Path to Handle on-fly,
    // the user should do it as rare as possible and cache Handles
    // on the application-side (for performace considerations).
    //
    // IMPORTANT NOTE: the internal storage is never freed, therefore,
    //                 the user should be careful with auto-generated
    //                 Paths (for example, for vertex-buffers).
    class Handle
    {
        static Path const kEmpty;

    public:
        Handle(): _path(&kEmpty) {}

        // intentionally non-explicit
        Handle(Path const &);

        template<typename... Args,
                 typename = std::enable_if_t<
                        (!std::same_as<Path, std::decay_t<Args>> && ...) &&
                        (!std::same_as<Handle, std::decay_t<Args>> && ...)>
        >
        explicit Handle(Args && ... args)
            : Handle(mkPath(std::forward<Args>(args)...))
        {}

    public:
        bool empty() const { return path().empty(); }
        size_t size() const { return path().size(); }

        template<typename T>
        T const & at(size_t index) const
        {
            Path const & p = path();
            MINIRE_INVARIANT(index < p.size(), "bad index ({}) for: \"{}\"",
                             index, p);
            path::Component const & c = p[index];
            MINIRE_INVARIANT(std::holds_alternative<T>(c),
                             "{}-th component of \"{}\" isn't {}",
                             index, p, utils::demangle<T>());
            return std::get<T>(c);
        }

    public:
        operator Path const & () const { return path(); }

        size_t hash() const { return std::hash<Path const *>{}(_path); }
        bool operator==(Handle const o) const { return _path == o._path; }

    private:
        explicit Handle(Path const * path)
            : _path(path)
        {}

        Path const & path() const { assert(_path); return *_path; }

    private:
        // Guaranteed to have same address for same path.
        Path const * _path = nullptr;

        friend Handle mkHandle(Path const &);
        friend std::string toString(Handle);
    };

    inline std::string toString(Handle h) { return toString(h.path()); }

    // Does the same as Path::Path. Thread safe.
    Handle mkHandle(Path const & path);
}

namespace std
{
    template<>
    struct hash<::minire::content::Handle>
    {
        size_t operator()(::minire::content::Handle h) const { return h.hash(); }
    };
}

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_same_v<T, ::minire::content::Handle>, char>>
    : fmt::formatter<std::string>
{
    template <typename FormatCtx>
    auto format(T const & value, FormatCtx & ctx) const
    {
        return fmt::formatter<std::string>::format(::minire::content::toString(value), ctx);
    }
};
