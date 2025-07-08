#pragma once

#include <minire/content/asset.hpp>
#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/utils/demangle.hpp>

#include <cassert>
#include <limits>
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

    class Manager
    {
        struct AssetBlock
        {
            Asset  _asset;
            size_t _usage = 0;
            size_t _size = 0; // bytes
        };

        using Store = std::unordered_map<Id, AssetBlock>;

    public:
        explicit Manager(size_t sizeLimit = 0);

        virtual ~Manager();

    public:
        void setReader(Reader::Uptr);

        template<typename T,
                 typename... Args>
        T & setReader(Args && ... args)
        {
            _reader = std::make_unique<T>(std::forward<Args>(args)...);
            return static_cast<T &>(*_reader);
        }

        std::unique_ptr<Lease> borrow(Id const &);

    private:
        void incUsage(Store::iterator it) noexcept;

        void decUsage(Store::iterator) noexcept;

        void cleanup();

    private:
        Reader::Uptr _reader;
        size_t const _sizeLimit = 0;
        size_t       _sizeCurrent = 0;
        Store        _store;
        Store        _garbage;

        friend class Lease;
    };

    class Lease
    {
        Lease(Lease const &) = delete;
        Lease(Lease &&) = delete;
        Lease& operator=(Lease const &) = delete;
        Lease& operator=(Lease &&) = delete;

    public:
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

namespace minire::content::readers
{
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
