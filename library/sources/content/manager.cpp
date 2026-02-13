#include <minire/content/manager.hpp>

#include <minire/errors.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/formats/gltf.hpp>
#include <minire/formats/image.hpp>
#include <minire/formats/obj.hpp>
#include <minire/logging.hpp>

#include <boost/algorithm/string.hpp>

#include <cassert>
#include <cstdlib> // for std::abort
#include <filesystem>
#include <fstream>
#include <new> // for std::bac_alloc

namespace minire::content
{
    Manager::Manager(size_t sizeLimit)
        : _sizeLimit(sizeLimit)
        , _sizeCurrent(0)
    {}

    Manager::~Manager()
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        bool fatal = false;
        for(auto & [id, assetBlock] : _store)
        {
            if (assetBlock._usage != 0)
            {
                // not throwing because it is a dtor
                MINIRE_ERROR("Some Leases have outlived their Manager for \"{}\"! "
                             "This is very bad and some terrible things are likely to happen :(", id);
                fatal = true;
            }
        }

        if (fatal)
        {
            // should't continue to work in a damaged environment
            MINIRE_ERROR("The program will be terminated due voilations of critical invariants.");
            std::abort();
        }
    }

    void Manager::setReader(Reader::Uptr reader)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        _reader = std::move(reader);
    }

    void Manager::newLayer(LayerId const & layerId)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        _currentLayer = layerId;
    }

    void Manager::disposeLayer(LayerId const & layerId)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        auto range = _layers.equal_range(layerId);
        for (auto it = range.first; it != range.second; ++it)
        {
            cleanup(_garbage.find(it->second));

            // NOTE: can't really dispose items from _store,
            //       because they are currently in use.
            // TODO: this case might cause a memory leak,
            //       until the next call of cleanup()
            if (_store.contains(it->second))
            {
                MINIRE_WARNING(
                    "requested to dispose layer \"{}\", but the asset \"{}\" "
                    "can't be disposed, because it is currently in use. "
                    "Please schedule an explicitl call to ContentManagerCleanup.",
                    layerId, it->second);
            }
        }
        _layers.erase(layerId);
    }

    std::unique_ptr<Lease> Manager::borrow(Id const & id)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        try
        {
            return borrowImpl(id);
        }
        catch(std::bad_alloc const & e)
        {
            MINIRE_WARNING("bad_alloc caught: {}, forcing GC", e.what());
            cleanup(true);
            return borrowImpl(id);
        }
    }

    std::unique_ptr<Lease> Manager::borrowImpl(Id const & id)
    {
        auto it = _store.find(id);

        if (it == _store.cend())
        {
            if (auto garbage = _garbage.find(id);
                garbage != _garbage.cend())
            {
                auto recovery = _garbage.extract(garbage);
                auto irt = _store.insert(std::move(recovery));
                MINIRE_INVARIANT(irt.inserted,
                                 "failed to restore an Asset from garbage: {}", id);
                it = irt.position;
                assert(it->first == id);
                assert(it->second._usage == 0);
            }
            else
            {
                MINIRE_INVARIANT(_reader, "can't load an asset, no reader set: {}", id);
                auto asset = _reader->load(id);
                MINIRE_INVARIANT(hasData(asset), "failed to load asset: {}", id);
                auto result = _store.emplace(id, AssetBlock{._asset = std::move(asset),
                                                            ._layerId = _currentLayer,
                                                            ._usage = 0,
                                                            ._size = sizeOf(asset)});
                MINIRE_INVARIANT(result.second, "failed to insert an AssetBlock: {}", id);
                _layers.emplace(_currentLayer, id);

                it = result.first;
            }

            assert(it != _store.cend());
            _sizeCurrent += it->second._size;
            cleanup(false);
        }

        assert(it != _store.cend());
        assert(it->first == id);

        // My deep condolences on the usage of a "new" operator,
        // but I have to do it since Lease::Lease is private and
        // std::make_unique won't be able to call it.
        return std::unique_ptr<Lease>(new Lease(it, *this));
    }

    std::unique_ptr<Lease> Manager::upload(Id const & id, Asset asset)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        auto [it, inserted] = _store.emplace(id, AssetBlock{._asset = std::move(asset),
                                                            ._layerId = _currentLayer,
                                                            ._usage = 0,
                                                            ._size = sizeOf(asset)});
        MINIRE_INVARIANT(inserted, "failed to upload raw asset: {}", id);
        _layers.emplace(_currentLayer, id);
        return std::unique_ptr<Lease>(new Lease(it, *this));
    }

    void Manager::cleanup(bool force)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        if (0 == _sizeLimit && !force) return;

        while(_sizeCurrent > _sizeLimit &&
              !_garbage.empty())
        {
            cleanup(_garbage.begin());
        }
    }

    void Manager::cleanup(Store::iterator it)
    {
        assert(it != _garbage.end());
        assert(it->second._usage == 0);
        assert(it->second._size <= _sizeCurrent);

        _sizeCurrent -= it->second._size;
        _garbage.erase(it);
    }

    void Manager::incUsage(Store::iterator it) noexcept
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        assert(it != _store.cend());
        assert(it->second._usage != std::numeric_limits<size_t>::max());
        it->second._usage++;
    }

    void Manager::decUsage(Store::iterator it) noexcept
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        assert(it != _store.cend());
        assert(it->second._usage != 0);
        it->second._usage--;

        if (it->second._usage == 0)
        {
            auto misfit = _store.extract(it);
            auto res = _garbage.insert(std::move(misfit));
            if (!res.inserted)
            {
                MINIRE_WARNING("failed to garbage an AssetBlock");
            }
        }
    }
}

namespace minire::content::readers
{
    void InMemory::remove(content::Id const & id)
    {
        _store.erase(id);
    }

    bool InMemory::contains(content::Id const & id) const
    {
        return _store.contains(id);
    }

    void InMemory::store(content::Id const & id,
                         content::Asset asset)
    {
        _store[id] = std::move(asset);
    }

    Asset InMemory::load(Id const & id) const
    {
        auto it = _store.find(id);
        return it != _store.cend() ? it->second
                                   : Asset(std::monostate());
    }
}

namespace minire::content::readers
{
    Filesystem::Filesystem(std::string prefix)
        : _prefix(std::move(prefix))
    {
        MINIRE_INVARIANT(std::filesystem::exists(_prefix),
                         "prefix doesn't exist: {}", _prefix);
        MINIRE_INFO("readers::Filesystem prefix: {}", _prefix);
    }

    Asset Filesystem::load(Id const & id) const
    {
        std::filesystem::path path(_prefix);
        path /= id; // TODO: this is pretty dangerous due possible ".."'s
                    // TODO: it won't work on non-Posix OS (i.e. *indows)
        MINIRE_INVARIANT(std::filesystem::exists(path),
                         "file doesn't exist: {}", path.string());

        std::string ext = path.extension();
        boost::algorithm::to_lower(ext);

        MINIRE_INFO("Reading asset from file: {}", path.string());
        if (".png"  == ext ||
            ".jpg" == ext ||
            ".jpeg" == ext ||
            ".tga" == ext)
        {
            models::Image::Sptr image = formats::loadImage(path);
            MINIRE_INVARIANT(image, "image not loaded: {}", path.string());
            return image;
        }
        else if (".obj"  == ext)
        {
            return formats::loadObj(path);
        }
        else if (".gltf"  == ext)
        {
            // TODO: might not work correctly w/ separated .bin files
            //       Implement "::tinygltf::FsCallbacks" into the Manager
            return formats::loadGltf(path);
        }
        else if (".glb"  == ext)
        {
            return formats::loadGlb(path);
        }
        else if (".bdf"  == ext)
        {
            return std::make_shared<formats::Bdf>(path);
        }
        else if (".txt" == ext)
        {
            auto size = std::filesystem::file_size(path);
            std::string content(size, '\0');
            std::ifstream in(path);
            MINIRE_INVARIANT(in.is_open(), "failed to open: {}", path.string());
            in.read(content.data(), content.size());
            return content;
        }
        else
        {
            MINIRE_THROW("Unknown content type (\"{}\"): {}", ext, path.string());
        }
    }
}

namespace minire::content::readers
{
    Chained & Chained::append(Reader::Uptr reader)
    {
        _readers.push_back(std::move(reader));
        return *this;
    }

    Asset Chained::load(Id const & id) const
    {
        for(Reader::Uptr const & reader : _readers)
        {
            assert(reader);
            Asset asset = reader->load(id);
            if (hasData(asset))
                return asset;
        }
        return std::monostate();
    }
}
