#pragma once

#include <minire/events/application/base.hpp>
#include <minire/utils/ray-caster.hpp>
 
#include <cstddef>
 
namespace minire::events::application
{
    struct OnRayCaster : public Base
    {
        utils::RayCaster::Uptr _rayCaster;
 
        template<typename... BaseArgs>
        OnRayCaster(utils::RayCaster::Uptr rayCaster,
                    BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _rayCaster(std::move(rayCaster))
        {}
    };
}
