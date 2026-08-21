#pragma once

/**
 * @file SDLVulkanCompat.h
 * @brief SDL2's Vulkan helpers, following the same layout probe as SDLCompat.h.
 *
 * SDL_vulkan.h declares the handful of Vulkan handle types it needs itself
 * rather than including vulkan.h, so this header is safe to include in a build
 * that has SDL but no Vulkan SDK.
 */

#include "SDLCompat.h"

#ifdef NEXUS_SDL2_ENABLED
    #if NEXUS_SDL_HEADER_PREFIXED
        #include <SDL2/SDL_vulkan.h>
    #else
        #include <SDL_vulkan.h>
    #endif
#endif
