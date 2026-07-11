#pragma once

#include <minire/logging.hpp>

#include <opengl.hpp>

#include <cassert>
#include <vector>

namespace minire::rasterizer
{
    template<typename Data>
    class FencedData
    {
        FencedData(FencedData const &) = delete;
        FencedData & operator=(FencedData const &) = delete;
        FencedData(FencedData &&) = delete;
        FencedData & operator=(FencedData &&) = delete;

    public:
        using Uptr = std::unique_ptr<FencedData>;

        FencedData()
        {}

        ~FencedData()
        {
            try
            {
                releaseFence();
            }
            catch(...)
            {}
        }

    public:
        void setFence(GLsync fence)
        {
            assert(nullptr == _fence);
            assert(nullptr != fence);
            _fence = fence;
        }

        bool isBusy()
        {
            if (nullptr == _fence)
                return false;

            GLint status = GL_UNSIGNALED;
            MINIRE_GL(glGetSynciv, _fence, GL_SYNC_STATUS, sizeof(GLint), NULL, &status);
            if (GL_SIGNALED == status)
            {
                releaseFence();
                return false;
            }

            return true;
        }

        void wait()
        {
            if (_fence != nullptr)
            {
                MINIRE_GL(glClientWaitSync, _fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
                releaseFence();
            }
        }

    public:
        Data & operator*()              { assert(nullptr == _fence); return _data; }
        Data const & operator*() const  { assert(nullptr == _fence); return _data; }
        Data * operator->()             { assert(nullptr == _fence); return &_data; }
        Data const * operator->() const { assert(nullptr == _fence); return &_data; }

    private:
        void releaseFence()
        {
            if (_fence != nullptr)
            {
                MINIRE_GL(glDeleteSync, _fence);
                _fence = nullptr;
            }
        }

    private:
        Data   _data;
        GLsync _fence = nullptr;
    };

    template<typename Data>
    class FencedDataPool
    {
    public:
        FencedData<Data> & fetch()
        {
            for (typename FencedData<Data>::Uptr & i : _store)
            {
                assert(i);
                if (!i->isBusy())
                    return *i;
            }

            // avoid inifinite grow
            if (_store.size() > 3)
            {
                _store[0]->wait();
                return *_store[0];
            }

            _store.emplace_back(std::make_unique<FencedData<Data>>());
            return *_store.back();
        }

    private:
        std::vector<typename FencedData<Data>::Uptr> _store;
    };
}