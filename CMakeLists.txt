cmake_minimum_required(VERSION 3.16)

project(NexusEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

set(NEXUS_THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty" CACHE PATH "Directory containing vendored third-party dependencies")
set(THIRD_PARTY_DIR "${NEXUS_THIRD_PARTY_DIR}")

option(ENABLE_PYTHON "Enable Python scripting support" ON)
option(ENABLE_LUA "Enable Lua scripting support" ON)
option(ENABLE_JOLT_PHYSICS "Enable Jolt Physics" ON)
option(ENABLE_PHYSX "Enable NVIDIA PhysX" OFF)
option(ENABLE_FMOD "Enable FMOD audio" OFF)
option(ENABLE_IMGUI "Enable ImGui for debugging UI" ON)
option(ENABLE_ASSIMP "Enable Assimp for advanced model loading" OFF)
option(ENABLE_ADVANCED_RENDERING "Enable advanced rendering features" ON)
option(ENABLE_RAY_TRACING "Enable ray tracing support" ON)
option(ENABLE_VR_SUPPORT "Enable VR support" OFF)
option(ENABLE_NETWORKING "Enable networking support" ON)
option(ENABLE_C_API "Enable C API bindings" ON)
option(ENABLE_GAME_IMPORTERS "Enable game project importers" ON)
option(ENABLE_CONSOLE_PLATFORMS "Enable console platform support" ON)
option(ENABLE_EXAMPLES "Enable example projects" OFF)
option(NEXUS_BUILD_TOOLS "Build Nexus command-line tools" ON)

option(ENABLE_VULKAN "Enable Vulkan rendering backend" ON)
option(ENABLE_OPENGL "Enable OpenGL rendering backend" ON)
option(ENABLE_DIRECTX9 "Enable DirectX 9 rendering backend" ON)
option(ENABLE_DIRECTX10 "Enable DirectX 10 rendering backend" ON)
option(ENABLE_DIRECTX11 "Enable DirectX 11 rendering backend" ON)
set(NEXUS_DEFAULT_GRAPHICS_API "Vulkan" CACHE STRING "Default graphics API (Vulkan, OpenGL, DirectX9, DirectX10, DirectX11)")
set_property(CACHE NEXUS_DEFAULT_GRAPHICS_API PROPERTY STRINGS "Vulkan" "OpenGL" "DirectX9" "DirectX10" "DirectX11")

set(NEXUS_THIRD_PARTY_INCLUDE_DIRS)

# ---------------------------------------------------------------------------
# Win32/DirectX compatibility shims
#
# The engine's headers are written against DirectXMath and a handful of Win32
# typedefs. On non-Windows platforms those headers do not exist, which made the
# whole codebase unbuildable rather than merely missing a renderer. compat/win32
# supplies a portable DirectXMath implementation plus declaration-only stand-ins
# for the DirectX headers, so shared engine code compiles everywhere while real
# Direct3D work stays confined to the Windows-only backends.
#
# Windows builds never see this directory and keep using the Windows SDK.
# ---------------------------------------------------------------------------
if(NOT WIN32)
    list(APPEND NEXUS_THIRD_PARTY_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/compat/win32")
    message(STATUS "Using portable Win32/DirectXMath compatibility shims from compat/win32")
endif()

if(EXISTS "${THIRD_PARTY_DIR}/stb")
    list(APPEND NEXUS_THIRD_PARTY_INCLUDE_DIRS "${THIRD_PARTY_DIR}/stb")
endif()

if(EXISTS "${THIRD_PARTY_DIR}/rapidjson")
    list(APPEND NEXUS_THIRD_PARTY_INCLUDE_DIRS "${THIRD_PARTY_DIR}/rapidjson")
endif()

if(EXISTS "${THIRD_PARTY_DIR}/rapidjson/include")
    list(APPEND NEXUS_THIRD_PARTY_INCLUDE_DIRS "${THIRD_PARTY_DIR}/rapidjson/include")
endif()

if(ENABLE_IMGUI AND EXISTS "${THIRD_PARTY_DIR}/imgui/imgui.h")
    set(IMGUI_FOUND TRUE)
    list(APPEND NEXUS_THIRD_PARTY_INCLUDE_DIRS
        "${THIRD_PARTY_DIR}"
        "${THIRD_PARTY_DIR}/imgui"
        "${THIRD_PARTY_DIR}/imgui/backends"
        "${THIRD_PARTY_DIR}/imgui/misc/cpp"
    )

    set(NEXUS_IMGUI_SOURCES
        "${THIRD_PARTY_DIR}/imgui/imgui.cpp"
        "${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp"
        "${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp"
        "${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp"
        "${THIRD_PARTY_DIR}/imgui/misc/cpp/imgui_stdlib.cpp"
    )

    if(WIN32)
        list(APPEND NEXUS_IMGUI_SOURCES
            "${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_win32.cpp"
            "${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_dx11.cpp"
        )
    endif()

    add_library(NexusImGui STATIC ${NEXUS_IMGUI_SOURCES})
    target_include_directories(NexusImGui PUBLIC
        "${THIRD_PARTY_DIR}/imgui"
        "${THIRD_PARTY_DIR}/imgui/backends"
        "${THIRD_PARTY_DIR}/imgui/misc/cpp"
    )

    if(WIN32)
        target_link_libraries(NexusImGui PUBLIC d3d11.lib user32.lib gdi32.lib dwmapi.lib)
    endif()
else()
    set(IMGUI_FOUND FALSE)
endif()

if(ENABLE_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development QUIET)
    if(Python3_FOUND)
        message(STATUS "Found Python ${Python3_VERSION}: ${Python3_EXECUTABLE}")
    else()
        message(WARNING "Python not found - Python scripting will be disabled")
        set(ENABLE_PYTHON OFF)
    endif()
endif()

find_package(DirectX QUIET)

set(JOLT_FOUND FALSE)
if(ENABLE_JOLT_PHYSICS)
    set(JOLT_SEARCH_PATHS
        "${THIRD_PARTY_DIR}/joltphysics"
        "${THIRD_PARTY_DIR}/JoltPhysics"
        "${CMAKE_SOURCE_DIR}/third_party/joltphysics"
        "${CMAKE_SOURCE_DIR}/third_party/JoltPhysics"
        "$ENV{JOLT_ROOT}"
    )

    foreach(search_path IN LISTS JOLT_SEARCH_PATHS)
        if(search_path AND EXISTS "${search_path}/Build/CMakeLists.txt")
            set(JOLT_ROOT "${search_path}")
            break()
        endif()
    endforeach()

    if(JOLT_ROOT)
        message(STATUS "Found Jolt Physics at: ${JOLT_ROOT}")

        set(TARGET_UNIT_TESTS OFF CACHE BOOL "Build Jolt unit tests" FORCE)
        set(TARGET_HELLO_WORLD OFF CACHE BOOL "Build Jolt hello world" FORCE)
        set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "Build Jolt performance test" FORCE)
        set(TARGET_SAMPLES OFF CACHE BOOL "Build Jolt samples" FORCE)
        set(TARGET_VIEWER OFF CACHE BOOL "Build Jolt viewer" FORCE)
        set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "Enable IPO for Jolt" FORCE)
        set(CROSS_PLATFORM_DETERMINISTIC ON CACHE BOOL "Cross-platform deterministic Jolt" FORCE)

        if(NOT TARGET Jolt)
            add_subdirectory("${JOLT_ROOT}/Build" "${CMAKE_BINARY_DIR}/thirdparty/jolt" EXCLUDE_FROM_ALL)
        endif()

        set(JOLT_FOUND TRUE)
        set(JOLT_INCLUDE_DIRS "${JOLT_ROOT}")
        set(JOLT_LIBRARIES Jolt)
    else()
        message(STATUS "Jolt Physics not found - using basic physics")
    endif()
endif()

# ---------------------------------------------------------------------------
# SDL2 - windowing, input and Vulkan surface creation
#
# Three ways to find it, in order of specificity: a vendored copy under
# thirdparty/, SDL2's own CMake package config, then pkg-config. The result is
# normalised into one interface target, NexusSDL2, because the three mechanisms
# disagree about what they set - the package config exports SDL2_INCLUDE_DIRS
# (plural, containing both <prefix>/include and <prefix>/include/SDL2) while
# Homebrew's exports only the SDL2 subdirectory. Consuming the singular variable,
# as this used to, silently dropped the include path on macOS.
# ---------------------------------------------------------------------------
set(SDL2_SEARCH_PATHS
    "${THIRD_PARTY_DIR}/SDL2"
    "${CMAKE_SOURCE_DIR}/third_party/SDL2"
    "$ENV{SDL2_DIR}"
    "C:/SDL2"
)

set(SDL2_FOUND FALSE)
set(NEXUS_SDL2_INCLUDE_DIRS)
set(NEXUS_SDL2_LINK_LIBRARIES)

foreach(search_path IN LISTS SDL2_SEARCH_PATHS)
    if(search_path AND EXISTS "${search_path}/include/SDL.h")
        if(WIN32)
            find_library(SDL2_LIBRARY
                NAMES SDL2
                PATHS "${search_path}/lib" "${search_path}/lib/x64"
                NO_DEFAULT_PATH
            )
            find_library(SDL2MAIN_LIBRARY
                NAMES SDL2main
                PATHS "${search_path}/lib" "${search_path}/lib/x64"
                NO_DEFAULT_PATH
            )
        else()
            find_library(SDL2_LIBRARY
                NAMES SDL2
                PATHS "${search_path}/lib"
                NO_DEFAULT_PATH
            )
        endif()
        if(SDL2_LIBRARY)
            set(SDL2_FOUND TRUE)
            # Both the prefix and its SDL2/ subdirectory, so <SDL.h> and
            # <SDL2/SDL.h> each resolve regardless of how the source spells it.
            set(NEXUS_SDL2_INCLUDE_DIRS "${search_path}/include")
            if(EXISTS "${search_path}/include/SDL2/SDL.h")
                list(APPEND NEXUS_SDL2_INCLUDE_DIRS "${search_path}/include/SDL2")
            endif()
            set(NEXUS_SDL2_LINK_LIBRARIES ${SDL2_LIBRARY})
            if(WIN32 AND SDL2MAIN_LIBRARY)
                list(APPEND NEXUS_SDL2_LINK_LIBRARIES ${SDL2MAIN_LIBRARY})
            endif()
            message(STATUS "Found vendored SDL2 at: ${search_path}")
            break()
        endif()
    endif()
endforeach()

if(NOT SDL2_FOUND)
    # Homebrew installs outside CMake's default prefixes on Apple Silicon, where
    # /opt/homebrew is not searched unless it is on CMAKE_PREFIX_PATH.
    if(APPLE)
        find_program(NEXUS_BREW_EXECUTABLE brew)
        if(NEXUS_BREW_EXECUTABLE)
            execute_process(
                COMMAND "${NEXUS_BREW_EXECUTABLE}" --prefix
                OUTPUT_VARIABLE NEXUS_BREW_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(NEXUS_BREW_PREFIX)
                list(APPEND CMAKE_PREFIX_PATH "${NEXUS_BREW_PREFIX}")
                message(STATUS "Added Homebrew prefix to search path: ${NEXUS_BREW_PREFIX}")
            endif()
        endif()
    endif()

    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        # SDL2_INCLUDE_DIRS when the config provides it, SDL2_INCLUDE_DIR
        # otherwise; the FindSDL2 module shipped by some distributions sets only
        # the latter.
        if(SDL2_INCLUDE_DIRS)
            set(NEXUS_SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
        elseif(SDL2_INCLUDE_DIR)
            set(NEXUS_SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
        endif()

        if(TARGET SDL2::SDL2)
            set(NEXUS_SDL2_LINK_LIBRARIES SDL2::SDL2)
        elseif(SDL2_LIBRARIES)
            set(NEXUS_SDL2_LINK_LIBRARIES ${SDL2_LIBRARIES})
        endif()
        message(STATUS "Found SDL2 via its CMake package config")
    endif()
endif()

if(NOT SDL2_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_SDL2 QUIET sdl2)
        if(PC_SDL2_FOUND)
            set(SDL2_FOUND TRUE)
            set(NEXUS_SDL2_INCLUDE_DIRS ${PC_SDL2_INCLUDE_DIRS})
            set(NEXUS_SDL2_LINK_LIBRARIES ${PC_SDL2_LINK_LIBRARIES})
            message(STATUS "Found SDL2 via pkg-config (${PC_SDL2_VERSION})")
        endif()
    endif()
endif()

if(SDL2_FOUND AND NOT NEXUS_SDL2_LINK_LIBRARIES)
    # Headers with no library is not a usable SDL2: it links on a static
    # library (nothing is resolved until an executable is produced) and then
    # fails at the first executable. Better to reject it here.
    message(WARNING "SDL2 headers were found but no SDL2 library was - treating SDL2 as unavailable")
    set(SDL2_FOUND FALSE)
endif()

if(SDL2_FOUND)
    add_library(NexusSDL2 INTERFACE)
    if(NEXUS_SDL2_INCLUDE_DIRS)
        target_include_directories(NexusSDL2 INTERFACE ${NEXUS_SDL2_INCLUDE_DIRS})
    endif()
    target_link_libraries(NexusSDL2 INTERFACE ${NEXUS_SDL2_LINK_LIBRARIES})
    message(STATUS "  SDL2 include: ${NEXUS_SDL2_INCLUDE_DIRS}")
    message(STATUS "  SDL2 libraries: ${NEXUS_SDL2_LINK_LIBRARIES}")
else()
    message(WARNING "SDL2 not found - no windowing backend, and the Vulkan backend cannot be built")
    if(WIN32)
        message(WARNING "  Run setup_dependencies.bat, or install SDL2 development libraries")
    elseif(APPLE)
        message(WARNING "  brew install sdl2")
    else()
        message(WARNING "  apt install libsdl2-dev (or your distribution's equivalent)")
    endif()
endif()

# ---------------------------------------------------------------------------
# Vulkan
#
# On macOS there is no native Vulkan driver: MoltenVK translates to Metal, and
# the Vulkan loader finds it through an ICD manifest. Headers alone are enough
# to compile but not to run, so the configuration reports which of the two is
# missing rather than leaving it to fail at vkCreateInstance.
# ---------------------------------------------------------------------------
if(ENABLE_VULKAN)
    set(VULKAN_SEARCH_PATHS
        "${THIRD_PARTY_DIR}/vulkan"
        "${CMAKE_SOURCE_DIR}/third_party/vulkan"
        "$ENV{VULKAN_SDK}"
    )

    set(VULKAN_FOUND FALSE)
    foreach(search_path IN LISTS VULKAN_SEARCH_PATHS)
        if(search_path AND EXISTS "${search_path}/include/vulkan/vulkan.h")
            set(Vulkan_INCLUDE_DIRS "${search_path}/include")
            set(VULKAN_FOUND TRUE)
            message(STATUS "Found Vulkan headers at: ${search_path}")
            message(STATUS "  Note: Using headers only. Install the full Vulkan SDK for validation layers and tools.")
            break()
        endif()
    endforeach()

    if(NOT VULKAN_FOUND)
        find_package(Vulkan QUIET)
        if(Vulkan_FOUND)
            message(STATUS "Found Vulkan: ${Vulkan_INCLUDE_DIRS}")
            set(VULKAN_FOUND TRUE)
        endif()
    endif()

    if(NOT VULKAN_FOUND)
        message(STATUS "Vulkan not found - Vulkan backend disabled")
        if(APPLE)
            message(STATUS "  brew install molten-vk vulkan-headers vulkan-loader")
            message(STATUS "  or install the LunarG SDK from https://vulkan.lunarg.com/")
        else()
            message(STATUS "  Install the Vulkan SDK from https://vulkan.lunarg.com/, or your distribution's libvulkan-dev")
        endif()
        set(ENABLE_VULKAN OFF)
    elseif(APPLE)
        # A Mac with headers but no MoltenVK ICD compiles the backend and then
        # finds zero physical devices at runtime, which is a confusing way to
        # discover a missing dependency.
        find_file(NEXUS_MOLTENVK_ICD
            NAMES MoltenVK_icd.json
            PATH_SUFFIXES share/vulkan/icd.d vulkan/icd.d
        )
        if(NEXUS_MOLTENVK_ICD)
            message(STATUS "  MoltenVK ICD: ${NEXUS_MOLTENVK_ICD}")
        else()
            message(WARNING "Vulkan headers found but no MoltenVK ICD manifest. "
                            "The Vulkan backend will build but report no GPUs at runtime. "
                            "Install it with: brew install molten-vk")
        endif()
    endif()
endif()

if(ENABLE_OPENGL)
    find_package(OpenGL QUIET)
    if(OPENGL_FOUND)
        message(STATUS "Found OpenGL")
        set(OPENGL_FOUND TRUE)

        # Check for GLAD (required for OpenGL function loading)
        set(GLAD_DIR "${THIRD_PARTY_DIR}/glad")
        if(EXISTS "${GLAD_DIR}/src/glad.c" AND EXISTS "${GLAD_DIR}/include/glad/glad.h")
            message(STATUS "Found GLAD at: ${GLAD_DIR}")
            add_library(glad STATIC "${GLAD_DIR}/src/glad.c")
            target_include_directories(glad PUBLIC "${GLAD_DIR}/include")
            set(GLAD_FOUND TRUE)
        else()
            message(WARNING "GLAD not found - OpenGL backend disabled")
            message(WARNING "  Generate GLAD files from: https://glad.dav1d.de/")
            message(WARNING "  Required files:")
            message(WARNING "    - ${GLAD_DIR}/src/glad.c")
            message(WARNING "    - ${GLAD_DIR}/include/glad/glad.h")
            message(WARNING "    - ${GLAD_DIR}/include/KHR/khrplatform.h")
            message(WARNING "  See docs/OPENGL_SETUP.md for detailed instructions")
            set(ENABLE_OPENGL OFF)
            set(GLAD_FOUND FALSE)
        endif()
    else()
        message(STATUS "OpenGL not found - OpenGL backend disabled")
        set(ENABLE_OPENGL OFF)
    endif()
endif()

if(ENABLE_LUA)
    find_package(Lua 5.4 QUIET)

    if(NOT LUA_FOUND)
        set(LUA_SOURCE_CANDIDATES
            "${THIRD_PARTY_DIR}/lua/src"
            "${THIRD_PARTY_DIR}/lua"
            "${THIRD_PARTY_DIR}/lua-5.4.7/src"
            "${CMAKE_SOURCE_DIR}/third_party/lua/src"
            "${CMAKE_SOURCE_DIR}/third_party/lua-5.4.7/src"
        )

        foreach(lua_source_dir IN LISTS LUA_SOURCE_CANDIDATES)
            if(EXISTS "${lua_source_dir}/lua.c" AND EXISTS "${lua_source_dir}/lauxlib.h")
                set(LUA_SOURCE_DIR "${lua_source_dir}")
                break()
            endif()
        endforeach()

        if(LUA_SOURCE_DIR)
            message(STATUS "Building Lua from source at: ${LUA_SOURCE_DIR}")
            file(GLOB LUA_SOURCES CONFIGURE_DEPENDS "${LUA_SOURCE_DIR}/*.c")
            list(REMOVE_ITEM LUA_SOURCES
                "${LUA_SOURCE_DIR}/lua.c"
                "${LUA_SOURCE_DIR}/luac.c"
            )

            add_library(lua STATIC ${LUA_SOURCES})
            target_include_directories(lua PUBLIC "${LUA_SOURCE_DIR}")

            set(LUA_FOUND TRUE)
            set(LUA_INCLUDE_DIR "${LUA_SOURCE_DIR}")
            set(LUA_LIBRARIES lua)
        else()
            message(STATUS "Lua library/source not found - Lua scripting will be disabled")
            set(ENABLE_LUA OFF)
        endif()
    else()
        message(STATUS "Found system Lua: ${LUA_VERSION_STRING}")
    endif()
endif()

set(PHYSX_FOUND FALSE)
if(ENABLE_PHYSX)
    find_path(PHYSX_INCLUDE_DIR
        NAMES PxPhysicsAPI.h
        PATHS
            "${THIRD_PARTY_DIR}/physx"
            "${THIRD_PARTY_DIR}/physx/include"
            "${CMAKE_SOURCE_DIR}/third_party/physx/include"
            "$ENV{PHYSX_ROOT}/include"
            "C:/Program Files/NVIDIA Corporation/PhysX/v5.1/include"
        PATH_SUFFIXES PhysX
    )

    if(PHYSX_INCLUDE_DIR)
        find_library(PHYSX_LIB
            NAMES PhysX_64 PhysX
            PATHS
                "${THIRD_PARTY_DIR}/physx/lib"
                "${CMAKE_SOURCE_DIR}/third_party/physx/lib"
                "$ENV{PHYSX_ROOT}/lib"
                "C:/Program Files/NVIDIA Corporation/PhysX/v5.1/lib"
            PATH_SUFFIXES x64 x86_64
        )

        if(PHYSX_LIB)
            set(PHYSX_FOUND TRUE)
            set(PHYSX_LIBRARIES ${PHYSX_LIB})
            set(PHYSX_INCLUDE_DIRS ${PHYSX_INCLUDE_DIR})
            message(STATUS "Found PhysX at: ${PHYSX_INCLUDE_DIR}")
        else()
            message(STATUS "PhysX headers found but libraries were not found - PhysX disabled")
            set(ENABLE_PHYSX OFF)
        endif()
    else()
        message(STATUS "PhysX not found - PhysX disabled")
        set(ENABLE_PHYSX OFF)
    endif()
endif()

set(FMOD_FOUND FALSE)
if(ENABLE_FMOD)
    find_path(FMOD_INCLUDE_DIR
        NAMES fmod.h
        PATHS
            "${THIRD_PARTY_DIR}/fmod/include"
            "${CMAKE_SOURCE_DIR}/third_party/fmod/include"
            "$ENV{FMOD_ROOT}/include"
            "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc"
            "C:/FMOD/api/core/inc"
    )

    if(FMOD_INCLUDE_DIR)
        find_library(FMOD_LIB
            NAMES fmod_vc fmod64_vc fmod
            PATHS
                "${THIRD_PARTY_DIR}/fmod/lib"
                "${CMAKE_SOURCE_DIR}/third_party/fmod/lib"
                "$ENV{FMOD_ROOT}/lib"
                "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64"
                "C:/FMOD/api/core/lib/x64"
        )

        if(FMOD_LIB)
            set(FMOD_FOUND TRUE)
            set(FMOD_LIBRARIES ${FMOD_LIB})
            set(FMOD_INCLUDE_DIRS ${FMOD_INCLUDE_DIR})
            message(STATUS "Found FMOD at: ${FMOD_INCLUDE_DIR}")
        else()
            message(STATUS "FMOD headers found but library was not found - FMOD disabled")
            set(ENABLE_FMOD OFF)
        endif()
    else()
        message(STATUS "FMOD not found - using basic audio")
        set(ENABLE_FMOD OFF)
    endif()
endif()

set(ASSIMP_FOUND FALSE)
if(ENABLE_ASSIMP)
    find_package(assimp QUIET)

    if(assimp_FOUND)
        set(ASSIMP_FOUND TRUE)
        message(STATUS "Found Assimp via CMake")
    else()
        find_path(ASSIMP_INCLUDE_DIR
            NAMES assimp/Importer.hpp
            PATHS
                "${THIRD_PARTY_DIR}/assimp/include"
                "${CMAKE_SOURCE_DIR}/third_party/assimp/include"
                "$ENV{ASSIMP_ROOT}/include"
                "C:/Program Files/assimp/include"
        )

        if(ASSIMP_INCLUDE_DIR)
            find_library(ASSIMP_LIB
                NAMES assimp assimp-vc143-mt
                PATHS
                    "${THIRD_PARTY_DIR}/assimp/lib"
                    "${CMAKE_SOURCE_DIR}/third_party/assimp/lib"
                    "$ENV{ASSIMP_ROOT}/lib"
                    "C:/Program Files/assimp/lib"
                PATH_SUFFIXES x64 x86_64
            )

            if(ASSIMP_LIB)
                set(ASSIMP_FOUND TRUE)
                set(ASSIMP_LIBRARIES ${ASSIMP_LIB})
                set(ASSIMP_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIR})
                message(STATUS "Found Assimp at: ${ASSIMP_INCLUDE_DIR}")
            endif()
        endif()
    endif()

    if(NOT ASSIMP_FOUND)
        message(STATUS "Assimp not found - advanced model loading disabled")
        set(ENABLE_ASSIMP OFF)
    endif()
endif()

set(NEXUS_PYTHON_ENABLED FALSE)
if(ENABLE_PYTHON AND Python3_FOUND)
    set(NEXUS_PYTHON_ENABLED TRUE)
endif()

set(NEXUS_LUA_ENABLED FALSE)
if(ENABLE_LUA AND LUA_FOUND)
    set(NEXUS_LUA_ENABLED TRUE)
endif()

set(NEXUS_JOLT_PHYSICS_ENABLED FALSE)
if(ENABLE_JOLT_PHYSICS AND JOLT_FOUND)
    set(NEXUS_JOLT_PHYSICS_ENABLED TRUE)
endif()

set(NEXUS_PHYSX_ENABLED ${PHYSX_FOUND})
set(NEXUS_FMOD_ENABLED ${FMOD_FOUND})
set(NEXUS_ASSIMP_ENABLED ${ASSIMP_FOUND})
set(NEXUS_IMGUI_ENABLED ${IMGUI_FOUND})
set(NEXUS_ADVANCED_RENDERING_ENABLED ${ENABLE_ADVANCED_RENDERING})
set(NEXUS_RAY_TRACING_ENABLED ${ENABLE_RAY_TRACING})
set(NEXUS_VR_ENABLED ${ENABLE_VR_SUPPORT})
set(NEXUS_NETWORKING_ENABLED ${ENABLE_NETWORKING})
set(NEXUS_C_API_ENABLED ${ENABLE_C_API})
set(NEXUS_GAME_IMPORTERS_ENABLED ${ENABLE_GAME_IMPORTERS})
set(NEXUS_CONSOLE_PLATFORMS_ENABLED ${ENABLE_CONSOLE_PLATFORMS})

set(NEXUS_SDL2_ENABLED ${SDL2_FOUND})
# The Vulkan backend creates its surface and queries its instance extensions
# through SDL2 (see src/rhi/vulkan/VulkanDevice.cpp), so Vulkan headers alone are
# not enough to build it. Checking that here turns a wall of compile errors into
# one actionable message.
set(NEXUS_VULKAN_ENABLED FALSE)
if(ENABLE_VULKAN AND VULKAN_FOUND)
    if(SDL2_FOUND)
        set(NEXUS_VULKAN_ENABLED TRUE)
    else()
        message(WARNING "Vulkan found but SDL2 is missing - the Vulkan backend needs SDL2 for surface creation. Install SDL2 development headers to enable it.")
    endif()
endif()

set(NEXUS_OPENGL_ENABLED FALSE)
if(ENABLE_OPENGL AND OPENGL_FOUND)
    set(NEXUS_OPENGL_ENABLED TRUE)
endif()

set(NEXUS_DIRECTX9_ENABLED FALSE)
if(ENABLE_DIRECTX9 AND WIN32)
    set(NEXUS_DIRECTX9_ENABLED TRUE)
endif()

set(NEXUS_DIRECTX10_ENABLED FALSE)
if(ENABLE_DIRECTX10 AND WIN32)
    set(NEXUS_DIRECTX10_ENABLED TRUE)
endif()

set(NEXUS_DIRECTX11_ENABLED FALSE)
if(ENABLE_DIRECTX11 AND WIN32)
    set(NEXUS_DIRECTX11_ENABLED TRUE)
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")

if(EXISTS "${CMAKE_SOURCE_DIR}/include/EngineConfig.h.in")
    configure_file(
        "${CMAKE_SOURCE_DIR}/include/EngineConfig.h.in"
        "${CMAKE_BINARY_DIR}/include/EngineConfig.h"
    )
else()
    message(STATUS "EngineConfig.h.in not found - skipping configuration")
endif()

add_subdirectory(src)

# ---------------------------------------------------------------------------
# Tests
#
# On by default: the suite links only the portable core, so it costs little and
# is the only thing standing between the cross-platform build and silent
# regression. Run with `ctest --test-dir <build> --output-on-failure`.
# ---------------------------------------------------------------------------
option(NEXUS_BUILD_TESTS "Build the Nexus test suite" ON)

if(NEXUS_BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Tests enabled - run: ctest --test-dir ${CMAKE_BINARY_DIR} --output-on-failure")
endif()

if(ENABLE_EXAMPLES AND EXISTS "${CMAKE_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(NEXUS_PYTHON_ENABLED AND EXISTS "${CMAKE_SOURCE_DIR}/python/CMakeLists.txt")
    add_subdirectory(python)
endif()

install(TARGETS NexusCore
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY shaders/ DESTINATION share/nexus/shaders)
install(DIRECTORY assets/ DESTINATION share/nexus/assets)

message(STATUS "=== Nexus Engine Configuration Summary ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Third-party root: ${THIRD_PARTY_DIR}")
message(STATUS "Python support: ${NEXUS_PYTHON_ENABLED}")
if(NEXUS_PYTHON_ENABLED)
    message(STATUS "  Python version: ${Python3_VERSION}")
endif()
message(STATUS "Lua support: ${NEXUS_LUA_ENABLED}")
if(NEXUS_LUA_ENABLED AND LUA_VERSION_STRING)
    message(STATUS "  Lua version: ${LUA_VERSION_STRING}")
endif()
message(STATUS "Jolt Physics: ${NEXUS_JOLT_PHYSICS_ENABLED}")
message(STATUS "NVIDIA PhysX: ${NEXUS_PHYSX_ENABLED}")
message(STATUS "FMOD Audio: ${NEXUS_FMOD_ENABLED}")
message(STATUS "Assimp: ${NEXUS_ASSIMP_ENABLED}")
message(STATUS "ImGui: ${NEXUS_IMGUI_ENABLED}")
message(STATUS "SDL2: ${NEXUS_SDL2_ENABLED}")
message(STATUS "Graphics APIs:")
message(STATUS "  Vulkan: ${NEXUS_VULKAN_ENABLED}")
message(STATUS "  OpenGL: ${NEXUS_OPENGL_ENABLED}")
message(STATUS "  DirectX 11: ${NEXUS_DIRECTX11_ENABLED}")
message(STATUS "  Default API: ${NEXUS_DEFAULT_GRAPHICS_API}")
message(STATUS "Advanced Rendering: ${ENABLE_ADVANCED_RENDERING}")
message(STATUS "Ray Tracing: ${ENABLE_RAY_TRACING}")
message(STATUS "VR Support: ${ENABLE_VR_SUPPORT}")
message(STATUS "Game Importers: ${ENABLE_GAME_IMPORTERS}")
message(STATUS "Console Platforms: ${ENABLE_CONSOLE_PLATFORMS}")
message(STATUS "Examples: ${ENABLE_EXAMPLES}")
message(STATUS "Tools: ${NEXUS_BUILD_TOOLS}")
message(STATUS "==========================================")

