#include <gui-controller/image-view.hpp>

#include <minire/events/controller.hpp>
#include <minire/gui-controller.hpp>
#include <minire/logging.hpp>

#include <utils/uuid.hpp>

#include <cassert>

namespace minire::gui_controller
{
    using namespace events::controller;

    ImageViewImpl::ImageViewImpl(content::Id const & textureId,
                                 utils::Patch const & patch,
                                 GuiController & controller)
        : _controller(controller)
        , _textureId(textureId)
        , _patch(patch)
    {
        auto [size, resizable] = _controller.measure(patch, textureId);
        _imageSize = size;
        _resizable = resizable;
        _newSpriteSize = size;
    }

    ImageViewImpl::~ImageViewImpl()
    {
        if (!_spriteId.empty())
        {
            _controller.enqueue<RemoveSprite>(_spriteId);
        }
    }

    void ImageViewImpl::initialize()
    {
        enqueueUncommited();
    }

    std::pair<glm::vec2, bool> ImageViewImpl::measure() const
    {
        return std::make_pair(_imageSize, _resizable);
    }

    void ImageViewImpl::setContentPosition(float left, float top)
    {
        if (glm::vec2 const newPosition(left, top);
            newPosition != _spritePosition || _newSpritePosition)
        {
            _newSpritePosition = newPosition;
            enqueueUncommited();
        }
    }

    void ImageViewImpl::setContentSize(float width, float height)
    {
        if (!_resizable)
        {
            return;
        }

        if (glm::vec2 const newSize(width, height);
            newSize != _spriteSize || _newSpriteSize)
        {
            _newSpriteSize = newSize;
            enqueueUncommited();
        }
    }

    void ImageViewImpl::setContentArea(gui::Area const & area)
    {
        setContentPosition(area._left, area._top);
        setContentSize(area._width, area._height);
    }

    size_t ImageViewImpl::onZOrderChanged(size_t zOffset)
    {
        if (_zOrder != zOffset || _newZOrder)
        {
            _newZOrder = zOffset;
            enqueueUncommited();
        }
        return zOffset + 1;
    }

    void ImageViewImpl::setVisible(bool visible)
    {
        if (_visible != visible || _newVisible)
        {
            _newVisible = visible;
            enqueueUncommited();
        }
    }

    void ImageViewImpl::commit()
    {
        if (_spriteId.empty())
        {
            _spriteId = utils::newUuid();

            if (_newSpritePosition)
            {
                _spritePosition = *_newSpritePosition;
                _newSpritePosition.reset();
            }

            if (_newSpriteSize)
            {
                _spriteSize = *_newSpriteSize;
                _newSpriteSize.reset();
            }

            if (_newZOrder)
            {
                _zOrder = *_newZOrder;
                _newZOrder.reset();
            }

            if (_newVisible)
            {
                _visible = *_newVisible;
                _newVisible.reset();
            }

            _controller.enqueue<CreateSprite>(
                _spriteId, _textureId, _patch, _spritePosition,
                _spriteSize, _visible, _zOrder);
        }

        bool changePosition = false;
        bool changeDimension = false;
        if (_newSpritePosition)
        {
            if (_spritePosition != *_newSpritePosition)
            {
                _spritePosition = *_newSpritePosition;
                changePosition = true;
            }
            _newSpritePosition.reset();
        }

        if (_newSpriteSize)
        {
            if (_spriteSize != *_newSpriteSize)
            {
                _spriteSize = *_newSpriteSize;
                changeDimension = true;
            }
            _newSpriteSize.reset();
        }

        if (changePosition && changeDimension)
        {
            assert(!_spriteId.empty());
            _controller.enqueue<SetSpriteArea>(_spriteId, _spritePosition,
                                               _spriteSize);
        }
        else if (changePosition)
        {
            assert(!_spriteId.empty());
            _controller.enqueue<MoveSprite>(_spriteId, _spritePosition);
        }
        else if (changeDimension)
        {
            assert(!_spriteId.empty());
            _controller.enqueue<ResizeSprite>(_spriteId, _spriteSize);
        }

        if (_newZOrder)
        {
            if (_zOrder != *_newZOrder)
            {
                _zOrder = *_newZOrder;
                assert(!_spriteId.empty());
                _controller.enqueue<SetSpriteZOrder>(_spriteId, _zOrder);
            }
            _newZOrder.reset();
        }

        if (_newVisible)
        {
            if (_visible != *_newVisible)
            {
                _visible = *_newVisible;
                assert(!_spriteId.empty());
                _controller.enqueue<SetSpriteVisible>(_spriteId, _visible);
            }
            _newVisible.reset();
        }
    }

    void ImageViewImpl::enqueueUncommited()
    {
        _controller.enqueueView(shared_from_this());
    }
}
