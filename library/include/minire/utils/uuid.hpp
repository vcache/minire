#pragma once

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

namespace minire::utils
{
    inline auto newUuid()
    {
        thread_local static boost::uuids::random_generator sGen;
        return boost::uuids::to_string(sGen());
    }
}
