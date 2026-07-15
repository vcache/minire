#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace minire::utils { struct FrustumPlanes; }
namespace minire::utils { struct Aabb; }

namespace minire::scene
{
    using IndexableId = size_t;
    static constexpr IndexableId kNoIndexableId = std::numeric_limits<IndexableId>::max();

    // Index implementations MUST NOT try to guess an actual type
    using IndexPayload = void;
    using IndexLayer = uint8_t;

    // Private implementation details, avoid using it
    namespace impl
    {
        template<typename T>
        class IndexableIdManager
        {
        public:
            IndexableId allocate(T * payload,
                                 IndexLayer indexLayer)
            {
                if (!_freeIds.empty())
                {
                    IndexableId const id = _freeIds.back();
                    _freeIds.pop_back();
                    assert(id < _payloads.size());
                    assert(id < _payloadLayers.size());
                    assert(_payloads[id] == nullptr);
                    _payloads[id] = payload;
                    _payloadLayers[id] = indexLayer;
                    return id;
                }

                _payloads.emplace_back(payload);
                _payloadLayers.emplace_back(indexLayer);
                assert(_payloads.size() == _payloadLayers.size());
                return _payloads.size() - 1;
            }

            void release(IndexableId indexableId)
            {
                assert(indexableId < _payloads.size());
                assert(_payloads[indexableId] != nullptr);
                _payloads[indexableId] = nullptr;
                _freeIds.push_back(indexableId);
            }

            bool isAllocated(IndexableId indexableId) const
            {
                return indexableId < _payloads.size()
                    && _payloads[indexableId] != nullptr;
            }

            T * payload(IndexableId indexableId) const
            {
                assert(indexableId < _payloads.size());
                assert(_payloads[indexableId] != nullptr);
                return _payloads[indexableId];
            }

            IndexLayer layer(IndexableId indexableId) const
            {
                assert(indexableId < _payloadLayers.size());
                return _payloadLayers[indexableId];
            }

            template<typename Callback>
            void iterate(IndexLayer indexLayer, Callback callback) const
            {
                assert(_payloads.size() == _payloadLayers.size());
                for(IndexableId i = 0; i < _payloads.size(); ++i)
                {
                    if (_payloads[i] && indexLayer == _payloadLayers[i])
                        callback(i, _payloads[i], _payloadLayers[i]);
                }
            }

        private:
            std::vector<IndexableId> _freeIds;
            std::vector<T *>         _payloads;
            std::vector<IndexLayer>  _payloadLayers;
        };
    }

    class SpatialIndex
    {
    public:
        using Uptr = std::unique_ptr<SpatialIndex>;

        SpatialIndex() = default;
        virtual ~SpatialIndex() = default;

    private:
        // Should be used only by SpatialHandler
        IndexableId create(IndexPayload * payload,
                           IndexLayer indexLayer,
                           utils::Aabb const & aabb)
        {
            assert(payload);
            IndexableId const id = _idManager.allocate(payload, indexLayer);
            try
            {
                createImpl(id, aabb);
                return id;
            }
            catch(...)
            {
                _idManager.release(id);
                throw;
            }
        }

        void update(IndexableId indexableId, utils::Aabb const & aabb)
        {
            assert(_idManager.isAllocated(indexableId));
            updateImpl(indexableId, aabb);
        }

        void erase(IndexableId indexableId)
        {
            assert(_idManager.isAllocated(indexableId));
            eraseImpl(indexableId); // if it throws, deallocation won't happen,
                                    // because an internal state is unclear and
                                    // the whole index should be rebuilt.
            _idManager.release(indexableId);
        }

    public:
        // The call is resposible to clean up the 'output' by themselves.
        virtual void cull(utils::FrustumPlanes const &,
                          IndexLayer indexLayer,
                          std::vector<IndexPayload *> & output) const = 0;

    protected:
        // Implemetations can assert that IndexableIds are tightly-allocated and valid.
        // IndexableId can be re-used after calling eraseImpl(), therefore
        // implementations must poperly clean their internal states.
        virtual void createImpl(IndexableId, utils::Aabb const &) = 0;
        virtual void updateImpl(IndexableId, utils::Aabb const &) = 0;
        virtual void eraseImpl(IndexableId) = 0;

        IndexPayload * get(IndexableId indexableId) const { return _idManager.payload(indexableId); }
        IndexLayer layer(IndexableId indexableId) const { return _idManager.layer(indexableId); }

        template<typename Callback>
        void iterate(IndexLayer indexLayer, Callback && callback) const
        {
            _idManager.iterate(indexLayer, std::forward<Callback>(callback));
        }

    private:
        impl::IndexableIdManager<IndexPayload> _idManager;

        friend class SpatialHandler;
    };
}