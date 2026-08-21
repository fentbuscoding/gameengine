#pragma once

/**
 * @file SDLCompat.h
 * @brief Single include point for SDL2, tolerant of how SDL is laid out.
 *
 * SDL2 installs its headers under an `SDL2/` subdirectory, but which directory
 * ends up on the include path depends entirely on how it was found:
 *
 *   - Debian/Ubuntu `libsdl2-dev` puts them in /usr/include/SDL2 and its CMake
 *     config exports /usr/include, so `<SDL2/SDL.h>` resolves.
 *   - Homebrew's sdl2 exports `$(brew --prefix)/include/SDL2` *itself* as the
 *     include directory, so only `<SDL.h>` resolves - `<SDL2/SDL.h>` does not.
 *   - sdl2-config --cflags likewise emits -I.../include/SDL2.
 *
 * The Vulkan backend hard-coded `<SDL2/SDL.h>`, which is why it built on Linux
 * and would not build on macOS. Probing with __has_include covers every layout
 * without the build system having to guess, and keeps the spelling in one place
 * rather than repeated in each translation unit that touches SDL.
 *
 * Including this header when SDL support is not compiled in is deliberately a
 * no-op, so headers that only optionally use SDL stay self-contained.
 */

#ifdef NEXUS_SDL2_ENABLED
    #if defined(__has_include)
        #if __has_include(<SDL2/SDL.h>)
            #include <SDL2/SDL.h>
            #define NEXUS_SDL_HEADER_PREFIXED 1
        #elif __has_include(<SDL.h>)
            #include <SDL.h>
            #define NEXUS_SDL_HEADER_PREFIXED 0
        #else
            #error "NEXUS_SDL2_ENABLED is defined but neither <SDL2/SDL.h> nor <SDL.h> is on the include path."
        #endif
    #else
        #include <SDL2/SDL.h>
        #define NEXUS_SDL_HEADER_PREFIXED 1
    #endif
#endif
