#pragma once

#include <minire/content/asset.hpp>
#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/utils/demangle.hpp>

#include <cassert>
#include <filesystem>
#include <limits>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace minire::content
{
    class Lease;

    // TODO: cover with tests

    class Reader
    {
    public:
        using Uptr = std::unique_ptr<Reader>;

        virtual ~Reader() = default;

        virtual Asset load(Id const & id) const = 0;
    };

    // The manager manages only RAM-allocated items.
    // This class is thread-safe and can be used directly or from a Controller.
    class Manager
    {
    public:
        using LayerId = std::string;

    private:
        struct AssetBlock
        {
            Asset   _asset;
            LayerId _layerId;
            size_t  _usage = 0;
            size_t  _size = 0; // bytes
        };

        using Store = std::unordered_map<Id, AssetBlock>;
        using Layers = std::unordered_multimap<LayerId, Id>;

    public:
        explicit Manager(size_t sizeLimit = 0);

        virtual ~Manager();

    public:
        // See rasterizer/resources.hpp for details,
        // Manager implements the semantics.
        void newLayer(LayerId const &);

        void disposeLayer(LayerId const &);

        LayerId const & current() const { return _currentLayer; }

    public:
        void setReader(Reader::Uptr);

        template<typename T,
                 typename... Args>
        T & setReader(Args && ... args)
        {
            std::lock_guard<std::recursive_mutex> guard(_mutex);
            _reader = std::make_unique<T>(std::forward<Args>(args)...);
            return static_cast<T &>(*_reader);
        }

        // TODO: maybe it should be const?
        // May forcefully run GC in case of std::bad_alloc caught.
        std::unique_ptr<Lease> borrow(Id const &);

    public:
        // TODO: add "shadow" flag into a Store key to avoid Id's namespace cluttering
        std::unique_ptr<Lease> upload(Id const &, Asset);

    public:
        void clear();
        void cleanup(bool force = false);

    private:
        std::unique_ptr<Lease> borrowImpl(Id const &); // thread-unsafe

        void incUsage(Store::iterator it) noexcept;

        void decUsage(Store::iterator) noexcept;

        void cleanup(Store::iterator); // thread-unsafe

    private:
        std::recursive_mutex _mutex;
        LayerId              _currentLayer;
        Reader::Uptr         _reader;
        size_t const         _sizeLimit = 0;
        size_t               _sizeCurrent = 0;
        Store                _store;
        Store                _garbage;
        Layers               _layers;

        friend class Lease;
    };

    class Lease
    {
        Lease(Lease const &) = delete;
        Lease(Lease &&) = delete;
        Lease& operator=(Lease const &) = delete;
        Lease& operator=(Lease &&) = delete;

    public:
        using Uptr = std::unique_ptr<Lease>;

        Id const & id() const
        {
            assert(_asset != _manager._store.cend());
            assert(_asset->second._usage != 0);
            return _asset->first;
        }

        Asset const & operator*() const
        {
            assert(_asset != _manager._store.cend());
            assert(_asset->second._usage != 0);
            return _asset->second._asset;
        }

        template<typename T>
        T const & as() const
        {
            Asset const & asset = operator*();
            T const * item = std::get_if<T>(&asset);
            if (!item)
            {
                MINIRE_THROW("bad asset type, required {}, but got {}: {}",
                             utils::demangle<T>(), demangle(asset), id());
            }
            return *item;
        }

        template<typename T>
        T const * tryAs() const
        {
            Asset const & asset = operator*();
            return std::get_if<T>(&asset);
        }

        template<typename Visitor>
        constexpr auto visit(Visitor && visitor)
        {
            Asset const & asset = operator*();
            return std::visit(std::forward<Visitor>(visitor), asset);
        }

        ~Lease()
        {
            _manager.decUsage(_asset);
        }

    private:
        Lease(Manager::Store::iterator asset,
              Manager & manager)
            : _asset(asset)
            , _manager(manager)
        {
            _manager.incUsage(_asset);
        }

    private:
        Manager::Store::iterator _asset;
        Manager                & _manager;

        friend class Manager;
    };
}

// NOTE: Readers don't have to be thread-safe, because
//       thread-safety is guaranteed by a Manager.

namespace minire::content::readers
{
    /**
     * NOTE: Note a difference with a Manager::upload():
     *       an InMemory store is a persistent store while
     *       assets uploaded through the Manager::upload()
     *       might be garbage collected and purged.
     * */
    class InMemory : public Reader
    {
    public:
        void remove(content::Id const &);

        bool contains(content::Id const &) const;

        void store(content::Id const &, content::Asset);

    public:
        Asset load(Id const &) const override;

    private:
        std::unordered_map<content::Id, content::Asset> _store;
    };
}

// TODO: class Generator: public Reader

namespace minire::content::readers
{
    class Filesystem : public Reader
    {
    public:
        explicit Filesystem(std::string prefix);

    public:
        Asset load(Id const &) const override;

    private:
        std::string _prefix;
    };
}

namespace minire::content::readers
{
    class Chained : public Reader
    {
    public:
        Asset load(Id const &) const override;

        Chained & append(Reader::Uptr);

        template<typename T,
                 typename... Args>
        Chained & append(Args && ... args)
        {
            append(std::make_unique<T>(std::forward<Args>(args)...));
            return *this;
        }

    private:
        std::vector<Reader::Uptr> _readers;
    };
}

namespace minire::content::readers
{

#ifdef MINIRE_HAS_PHYSFS

    // NOTE: PhysicsFS will call PHYSFS_init/PHYSFS_deinit automatically,
    //       therefore, you should avoid calling them manually, because
    //       it will break init/deinit symmetry.
    // See PhysicsFS3 documentation for details about PhysicsFS behaviour:
    //  https://wiki.icculus.org/PhysicsFS3/CategoryPhysicsFS
    class PhysFS : public Reader
    {
        PhysFS(PhysFS const &) = delete;
        PhysFS & operator=(PhysFS const &) = delete;

    public:
        // Usually should just pass argv[0].
        // Assuming argv0 is immutable. Must not pass
        // different argv0 when creating several
        // instances of PhysFS.
        explicit PhysFS(char const * argv0);
        ~PhysFS() override;

        Asset load(Id const &) const override;

    public:
        // Will throw in case of error.
        void mount(std::filesystem::path const & newDir,    // platform-independent (differs from PhysFS!)
                   std::string const & mountPoint,          // NULL or "" is equivalent to "/".
                   bool appendToPath = true);               // append to search path (or to prepend)
    };
#else

    class PhysFS
    {
    public:
        template <typename T = void>
        PhysFS(...)
        {
            static_assert(sizeof(T) == 0,
                "(!!!) PhysFS isn't available (libminire is built w/o PhysicsFS support)");
        }

        template <typename T = void>
        operator T()
        {
            static_assert(sizeof(T) == 0,
                "(!!!) PhysFS isn't available (libminire is built w/o PhysicsFS support)");
            return {};
        }
    };

#endif // MINIRE_HAS_PHYSFS

}
