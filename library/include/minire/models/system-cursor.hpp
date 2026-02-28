#pragma once

namespace minire::models
{
    enum class SystemCursor
    {
        kArrow,     // SDL_SYSTEM_CURSOR_ARROW,     /**< Arrow */
        kIbeam,     // SDL_SYSTEM_CURSOR_IBEAM,     /**< I-beam */
        kWait,      // SDL_SYSTEM_CURSOR_WAIT,      /**< Wait */
        kCrosshair, // SDL_SYSTEM_CURSOR_CROSSHAIR, /**< Crosshair */
        kWaitArrow, // SDL_SYSTEM_CURSOR_WAITARROW, /**< Small wait cursor (or Wait if not available) */
        kSizeNWSE,  // SDL_SYSTEM_CURSOR_SIZENWSE,  /**< Double arrow pointing northwest and southeast */
        kSizeNESW,  // SDL_SYSTEM_CURSOR_SIZENESW,  /**< Double arrow pointing northeast and southwest */
        kSizeWE,    // SDL_SYSTEM_CURSOR_SIZEWE,    /**< Double arrow pointing west and east */
        kSizeNS,    // SDL_SYSTEM_CURSOR_SIZENS,    /**< Double arrow pointing north and south */
        kSizeAll,   // SDL_SYSTEM_CURSOR_SIZEALL,   /**< Four pointed arrow pointing north, south, east, and west */
        kNo,        // SDL_SYSTEM_CURSOR_NO,        /**< Slashed circle or crossbones */
        kHand,      // SDL_SYSTEM_CURSOR_HAND,      /**< Hand */
    };
}