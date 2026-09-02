#include <minire/content/manager.hpp>

#include <minire/errors.hpp>
#include <minire/formats/audio-clip.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/formats/gltf.hpp>
#include <minire/formats/image.hpp>
#include <minire/formats/obj.hpp>
#include <minire/logging.hpp>

#include <boost/algorithm/string.hpp>

#ifdef MINIRE_HAS_PHYSFS
#   include <content/manager/physfs-istream.hpp>
#   include <physfs.h>
#endif

#include <cassert>
#include <cstdlib> // for std::abort
#include <filesystem>
#include <fstream>
#include <mutex>
#include <new> // for std::bac_alloc

namespace minire::content
{
    namespace
    {
        std::optional<std::streamsize> sizeOf(std::istream & istream)
        {
            std::streampos original_pos = istream.tellg();
            if (original_pos == std::streampos(-1))
            {
                return std::nullopt;
            }

            istream.seekg(0, std::ios::end);
            std::streampos end_pos = istream.tellg();

            istream.seekg(original_pos);
            return end_pos - original_pos;
        }

        Asset load(Id const & id, std::unique_ptr<std::istream> istream)
        {
            assert(istream);

            std::string ext = std::filesystem::path(id).extension().string();
            boost::algorithm::to_lower(ext);

            if (".png"  == ext ||
                ".jpg"  == ext ||
                ".jpeg" == ext ||
                ".tga"  == ext)
            {
                models::Image::Sptr image = formats::loadImage(*istream);
                MINIRE_INVARIANT(image, "an image wasn't loaded: \"{}\"", id);
                return image;
            }
            else if (".obj"  == ext)
            {
                return formats::loadObj(*istream);
            }
            else if (".gltf"  == ext)
            {
                // TODO: might not work correctly w/ separated .bin files
                //       Implement "::tinygltf::FsCallbacks" into the Manager
                return formats::loadGltf(*istream);
            }
            else if (".glb"  == ext)
            {
                return formats::loadGlb(*istream);
            }
            else if (".bdf"  == ext)
            {
                return std::make_shared<formats::Bdf>(*istream, id);
            }
            else if (".txt" == ext)
            {
                if (auto size = sizeOf(*istream); size)
                {
                    std::string content(*size, '\0');
                    istream->read(content.data(), content.size());
                    content.resize(static_cast<size_t>(istream->gcount()));
                    return content;
                }

                // fallback to byte-to-byte reading
                return std::string((std::istreambuf_iterator<char>(*istream)),
                                    std::istreambuf_iterator<char>());
            }
            else if (".wav" == ext || ".wave" == ext || ".aif" == ext || ".aiff" == ext ||
                     ".flac" == ext || ".fla" == ext ||
                     ".mod" == ext || ".s3m" == ext || ".xm" == ext || ".it" == ext ||
                        ".med" == ext || ".669" == ext || ".dsm" == ext || ".amf" == ext ||
                        ".psm" == ext ||
                     ".mp1" == ext || ".mp2" == ext || ".mp3" == ext ||
                     ".ogg" == ext || ".oga" == ext ||
                     ".mid" == ext || ".midi" == ext || ".rmi" == ext ||
                     ".opus" == ext ||
                     ".wv" == ext || ".wvc" == ext)
            {
                return std::make_shared<formats::AudioClip>(std::move(istream));
            }
            else
            {
                MINIRE_THROW("Unknown content type (\"{}\"): {}", ext, id);
            }
        }
    }

    Manager::Manager(size_t sizeLimit)
        : _sizeLimit(sizeLimit)
        , _sizeCurrent(0)
    {}

    Manager::~Manager()
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        auto checkStorage = [](Store const & store)
        {
            if (!store.empty())
            {
                for(auto & [id, assetBlock] : store)
                {
                    if (assetBlock._usage != 0)
                    {
                        // not throwing because it is a dtor
                        MINIRE_ERROR("Some Leases have outlived their Manager for \"{}\" ({})! "
                                     "This is very bad and some terrible things are likely to happen :(",
                                     id, assetBlock._usage);
                    }
                }

                // should't continue to work in a damaged environment
                MINIRE_ERROR("The program will be terminated due voilations of critical invariants. "
                             "Manager leases are left, or forgot to call Manager::clear()?");
                std::abort();
            }
        };

        checkStorage(_store);
        checkStorage(_garbage);
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

    void Manager::clear()
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        assert(_store.empty());
        while(!_garbage.empty())
        {
            cleanup(_garbage.begin());
        }
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
                         "a file doesn't exist: {}", path.string());
        MINIRE_INFO("Reading an asset from a file: {}", path.string());
        auto istream = std::make_unique<std::ifstream>(path, std::ios::in | std::ios::binary);
        MINIRE_INVARIANT(istream->is_open(), "failed to open: \"{}\"", id);
        return ::minire::content::load(id, std::move(istream));
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

#ifdef MINIRE_HAS_PHYSFS

namespace minire::content::readers
{
    namespace
    {
        class PhysFSGuard
        {
        public:
            static PhysFSGuard & instance()
            {
                static PhysFSGuard gPhysFSGuard;
                return gPhysFSGuard;
            }

            void increase(char const * argv0)
            {
                std::unique_lock<std::mutex> lock(_initMutex);

                MINIRE_INVARIANT(_argv0.empty() || argv0 == _argv0,
                                 "argv0 has changed: {} -> {}",
                                 _argv0, argv0);

                _usages++;
                if (_usages > 0 && 0 == ::PHYSFS_isInit())
                {
                    MINIRE_INVARIANT(0 != ::PHYSFS_init(argv0), "PHYSFS_init failed: {}",
                                     getLastError());
                    MINIRE_INFO("PhysFS is initialized");
                    _argv0 = argv0;
                }
            }

            void decrease()
            {
                std::unique_lock<std::mutex> lock(_initMutex);

                assert(_usages != 0);
                _usages--;
                if (_usages == 0 && 0 != ::PHYSFS_isInit())
                {
                    if (0 == ::PHYSFS_deinit())
                    {
                        MINIRE_ERROR("PHYSFS_deinit failed: {}", getLastError());
                    }
                    MINIRE_INFO("PhysFS is de-initialized");
                }
            }

        public:
            static char const * getLastError()
            {
                ::PHYSFS_ErrorCode const errCode = ::PHYSFS_getLastErrorCode();
                return ::PHYSFS_getErrorByCode(errCode);
            }

        private:
            size_t      _usages = 0;
            std::mutex  _initMutex;
            std::string _argv0;
        };
    }

    PhysFS::PhysFS(char const * argv0)
    {
        PhysFSGuard::instance().increase(argv0);
    }

    PhysFS::~PhysFS()
    {
        // NOTE: must be safe in case of PhysFS's move,
        //       because a new instance will be created
        //       before destruction of an old one.
        PhysFSGuard::instance().decrease();
    }

    Asset PhysFS::load(Id const & id) const
    {
        MINIRE_INVARIANT(0 != ::PHYSFS_exists(id.c_str()),
                         "Asset doesn't exist: {}", id);

        MINIRE_INFO("Reading an asset from a PhysFs: {}", id);

        auto istream = std::make_unique<manager::PhysFSIStream>(id);
        return ::minire::content::load(id, std::move(istream));
    }

    void PhysFS::mount(std::filesystem::path const & newDir,
                       std::string const & mountPoint,
                       bool appendToPath)
    {
        // NOTE: newDir.string().c_str() returns UTF8 of platform-specific path
        MINIRE_INVARIANT(0 != ::PHYSFS_mount(newDir.string().c_str(), mountPoint.c_str(), appendToPath),
                         "PHYSFS_mount failed (\"{}\", \"{}\", {}): {}",
                         newDir.string().c_str(), mountPoint, appendToPath,
                         PhysFSGuard::getLastError());
    }
}

#endif // MINIRE_HAS_PHYSFS
