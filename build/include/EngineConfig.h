#ifndef NEXUS_ENGINE_CONFIG_H
#define NEXUS_ENGINE_CONFIG_H

// Engine version
#define NEXUS_VERSION_MAJOR 1
#define NEXUS_VERSION_MINOR 0
#define NEXUS_VERSION_PATCH 0
#define NEXUS_VERSION_STRING "1.0.0"

// Platform detection
#ifdef _WIN32
    #define NEXUS_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define NEXUS_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define NEXUS_PLATFORM_MACOS 1
#endif

// Feature flags - configured by CMake
/* #undef NEXUS_PYTHON_ENABLED */
/* #undef NEXUS_LUA_ENABLED */
/* #undef NEXUS_BULLET_PHYSICS_ENABLED */
/* #undef NEXUS_PHYSX_ENABLED */
/* #undef NEXUS_FMOD_ENABLED */
/* #undef NEXUS_ASSIMP_ENABLED */
/* #undef NEXUS_IMGUI_ENABLED */
/* #undef NEXUS_ADVANCED_RENDERING_ENABLED */
/* #undef NEXUS_RAY_TRACING_ENABLED */
/* #undef NEXUS_VR_ENABLED */
/* #undef NEXUS_NETWORKING_ENABLED */
/* #undef NEXUS_C_API_ENABLED */
/* #undef NEXUS_GAME_IMPORTERS_ENABLED */
/* #undef NEXUS_CONSOLE_PLATFORMS_ENABLED */

// Debug features
#ifdef _DEBUG
    #define NEXUS_DEBUG 1
    #define NEXUS_ENABLE_PROFILING 1
    #define NEXUS_MEMORY_DEBUGGING 1
#endif

// API export/import macros
#ifdef NEXUS_SHARED_LIBRARY_BUILD
    #ifdef NEXUS_PLATFORM_WINDOWS
        #define NEXUS_API __declspec(dllexport)
    #else
        #define NEXUS_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef NEXUS_PLATFORM_WINDOWS
        #define NEXUS_API __declspec(dllimport)
    #else
        #define NEXUS_API
    #endif
#endif

// Static library build
#ifdef NEXUS_STATIC_BUILD
    #undef NEXUS_API
    #define NEXUS_API
#endif

// Compiler-specific features
#ifdef _MSC_VER
    #define NEXUS_FORCE_INLINE __forceinline
    #define NEXUS_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define NEXUS_FORCE_INLINE __attribute__((always_inline)) inline
    #define NEXUS_NO_INLINE __attribute__((noinline))
#else
    #define NEXUS_FORCE_INLINE inline
    #define NEXUS_NO_INLINE
#endif

// DirectX version support
#define NEXUS_DIRECTX_11_ENABLED 1
#ifdef NEXUS_RAY_TRACING_ENABLED
    #define NEXUS_DIRECTX_12_ENABLED 1
    #define NEXUS_DXR_ENABLED 1
#endif

// Memory alignment
#define NEXUS_MEMORY_ALIGNMENT 16

// Maximum supported features
#define NEXUS_MAX_LIGHTS 32
#define NEXUS_MAX_TEXTURES 16
#define NEXUS_MAX_VERTEX_ATTRIBUTES 16
#define NEXUS_MAX_UNIFORM_BUFFERS 16

#endif // NEXUS_ENGINE_CONFIG_H
