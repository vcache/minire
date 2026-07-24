#include <minire/content/handle.hpp>

#include <minire/content/path.hpp>
#include <minire/errors.hpp>

#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>

namespace minire::content
{
    const Path Handle::kEmpty = Path();

    namespace
    {
        class HandleStorage
        {
        public:
            static HandleStorage & instance()
            {
                // NOTE: it cannot be thread_local becase different Handles
                //       that point to the same path must containt the same
                //       reference (_path), because it is used for hash and
                //       comparison operators!
                static HandleStorage kStorage;
                return kStorage;
            }

            Path const * resolve(Path const & path)
            {
                // Fast short-cut, read only
                {
                    std::shared_lock<std::shared_mutex> rdLock(_mutex);
                    auto it = _store.find(path);
                    if (it != _store.end()) return &(*it);
                }

                // Full-fledged version, upsert
                {
                    std::unique_lock<std::shared_mutex> rwLock(_mutex);
                    auto it = _store.find(path);
                    if (it != _store.end()) return &(*it);

                    it = _store.insert(path).first;
                    return &(*it);
                }
            }

        private:
            std::shared_mutex        _mutex;
            std::unordered_set<Path> _store;
        };
    }

    Handle mkHandle(Path const & path)
    {
        HandleStorage & handleStorage = HandleStorage::instance();
        Path const * persistentReference = handleStorage.resolve(path);
        return Handle(persistentReference);
    }

    Handle::Handle(Path const & path)
        : _path(HandleStorage::instance().resolve(path))
    {}
}