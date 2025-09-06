#pragma once

#include <minire/errors.hpp>
#include <minire/utils/geometry.hpp>

#include <bitset>
#include <cassert>
#include <optional>

namespace minire::models
{
    enum class QueryEventFilter
    {
        kOnFps,
        kOnResize,
        kOnMouseWheel,
        kOnMouseMove,
        kOnMouseDown,
        kOnMouseUp,
        kOnKeyUp,
        kOnKeyDown,
        kOnTextInput,
    };

    enum class QueryKind
    {
        kRayLeftTop     = 0,
        kRayRightTop    = 1,
        kRayLeftBottom  = 2,
        kRayRightBottom = 3,
        kRayCenter      = 4,
        kRayCursor      = 5,

        __count__       = 6,
    };

    class QueryFlags
    {
    public:
        bool test(QueryKind queryKind) const
        {
            assert(queryKind != QueryKind::__count__);
            return _store.test(static_cast<size_t>(queryKind));
        }

        void set(QueryKind queryKind)
        {
            MINIRE_INVARIANT(queryKind != QueryKind::__count__,
                             "QueryKind::__count__ cannot be used as a flag");
            _store.set(static_cast<size_t>(queryKind), true);
        }

        void unset(QueryKind queryKind)
        {
            MINIRE_INVARIANT(queryKind != QueryKind::__count__,
                             "QueryKind::__count__ cannot be used as a flag");
            _store.set(static_cast<size_t>(queryKind), false);
        }

        bool none() const { return _store.none(); }

    private:
        using Store = std::bitset<static_cast<size_t>(QueryKind::__count__)>;

        Store _store;
    };

    struct SceneTraits
    {
        // TODO: consider use unique_ptr to decrease struct's size
        using MaybeRay = std::optional<utils::Ray>;

        MaybeRay _rayLeftTop;
        MaybeRay _rayRightTop;
        MaybeRay _rayLeftBottom;
        MaybeRay _rayRightBottom;
        MaybeRay _rayCenter;
        MaybeRay _rayCursor;
    };
}