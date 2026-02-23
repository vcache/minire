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

    void ImageViewImpl::setClippingWindow(gui::MaybeArea const & area)
    {
        utils::MaybeRect rect;
        if (area)
        {
            rect.emplace(area->_left,
                         area->_top,
                         area->_left + area->_width,
                         area->_top + area->_height);
        }

        if (rect != _clippingWindow)
        {
            _newClippingWindow = rect;
            enqueueUncommited();
        }
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

    void ImageViewImpl::setContent(content::Id const & textureId,
                                   utils::Patch const & patch)
    {
        setTexture(textureId);
        setPatch(patch);
    }

    void ImageViewImpl::setPatch(utils::Patch const & patch)
    {
        if (patch != _patch || _newPatch)
        {
            _newPatch = patch;
            auto [size, resizable] = _controller.measure(patch, _textureId);
            _imageSize = size;
            _resizable = resizable;
            invalidate();
            enqueueUncommited();
        }
    }

    void ImageViewImpl::setTexture(content::Id const & textureId)
    {
        if (textureId != _textureId || _newTextureId)
        {
            _newTextureId = textureId;
            invalidate();
            enqueueUncommited();
        }
    }

    void ImageViewImpl::commit()
    {
        if (_spriteId.empty())
        {
            _spriteId = utils::newUuid();

            if (_newPatch)
            {
                _patch = *_newPatch;
                _newPatch.reset();
            }

            if (_newTextureId)
            {
                _textureId = *_newTextureId;
                _newTextureId.reset();
            }

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

            if (_newClippingWindow)
            {
                _clippingWindow = *_newClippingWindow;
                _newClippingWindow.reset();
            }

            _controller.enqueue<CreateSprite>(
                _spriteId, _textureId, _patch, _spritePosition,
                _spriteSize, _clippingWindow, _visible, _zOrder);
        }

        if (_newPatch)
        {
            if (_patch != *_newPatch)
            {
                _patch = *_newPatch;
                assert(!_spriteId.empty());
                _controller.enqueue<SetSpritePatch>(_spriteId, _patch);
            }
            _newPatch.reset();
        }

        if (_newTextureId)
        {
            if (_textureId != *_newTextureId)
            {
                _textureId = *_newTextureId;
                assert(!_spriteId.empty());
                _controller.enqueue<SetSpriteTexture>(_spriteId, _textureId);
            }
        }

        bool changePosition = false;
        bool changeDimension = false;
        bool changeClippingWindow = false;
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

        if (_newClippingWindow)
        {
            if (_clippingWindow != *_newClippingWindow)
            {
                _clippingWindow = *_newClippingWindow;
                changeClippingWindow = true;
            }
            _newClippingWindow.reset();
        }

        if (changePosition && changeDimension)
        {
            assert(!_spriteId.empty());
            _controller.enqueue<SetSpriteArea>(_spriteId, _spritePosition,
                                               _spriteSize, _clippingWindow);
        }
        else if (changePosition)
        {
            assert(!_spriteId.empty());
            _controller.enqueue<MoveSprite>(_spriteId, _spritePosition, _clippingWindow);
        }
        else if (changeDimension)
        {
            assert(!_spriteId.empty());
            _controller.enqueue<ResizeSprite>(_spriteId, _spriteSize, _clippingWindow);
        }
        else if (changeClippingWindow)
        {
            assert(!_spriteId.empty());
            _controller.enqueue<SetSpriteClippingWindow>(_spriteId, _clippingWindow);
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
