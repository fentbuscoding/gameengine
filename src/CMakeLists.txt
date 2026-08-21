cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE ALL_CPP_FILES CONFIGURE_DEPENDS "*.cpp")

set(NEXUS_LIB_SOURCES)
set(NEXUS_EXCLUDED_SOURCES)

# ---------------------------------------------------------------------------
# Legacy Direct3D 11 / Win32 runtime layer
#
# These translation units call Direct3D 11, DirectInput, XAudio2, the Kinect
# SDK or the Win32 windowing API directly - not merely through a typedef, but
# as real API calls - so they can only be compiled against the Windows SDK.
# The engine's headers are portable (see compat/win32), and the newer RHI
# backends under src/rhi/ are the cross-platform path forward; this list is
# what is left of the pre-RHI renderer.
#
# Everything not listed here builds on every platform, which is what lets the
# core library and its tests run in CI on Linux.
# ---------------------------------------------------------------------------
set(NEXUS_WINDOWS_ONLY_SOURCES
    core/Engine.cpp                 # Win32 window creation and message loop
    core/EngineErrorRecovery.cpp    # depends on Engine
    api/NexusC.cpp                  # C API over Engine
    graphics/GraphicsDevice.cpp     # Direct3D 11 device and swap chain
    graphics/Texture.cpp            # Direct3D 11 textures
    graphics/Shader.cpp             # Direct3D 11 shaders / D3DCompile
    graphics/Mesh.cpp               # Direct3D 11 vertex and index buffers
    graphics/TextRenderer.cpp       # Direct3D 11 glyph atlas
    graphics/LightingEngine.cpp     # Direct3D 11 render targets
    graphics/AnimationSystem.cpp    # Direct3D 11 skinning buffers
    utils/ResourceManager.cpp       # creates Direct3D 11 resources
    input/InputManager.cpp          # DirectInput 8
    audio/AudioSystem.cpp           # XAudio2 / X3DAudio
    motion/MotionControlSystem.cpp  # Kinect SDK and XInput
    ui/EngineUI.cpp                 # ImGui Win32 + DX11 backends
    ui/EngineUI_Modern.cpp          # ImGui Win32 + DX11 backends
    ui/GameImporterUI.cpp           # depends on the ImGui UI layer
)

# Sources that call directly into a graphics API only build where that API's
# headers and libraries exist. Selecting them here - rather than wrapping each
# file in a giant #ifdef - keeps the backends readable and makes it obvious
# which parts of the engine are platform-specific.
foreach(source_file IN LISTS ALL_CPP_FILES)
    get_filename_component(source_name "${source_file}" NAME)
    get_filename_component(source_dir "${source_file}" DIRECTORY)
    file(RELATIVE_PATH source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${source_file}")
    set(exclude_source FALSE)

    if(source_name MATCHES "^(main|main_debug|main_simple|texture_test)\\.cpp$")
        # Entry points are linked into executables, not the core library.
        set(exclude_source TRUE)
    elseif(source_name MATCHES "(_backup|_clean|_fixed|_stub)\\.cpp$")
        # Superseded variants kept only for reference.
        set(exclude_source TRUE)
    elseif(source_dir STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/tools")
        # Tool sources belong to their own executables.
        set(exclude_source TRUE)
    elseif(NOT WIN32 AND source_rel MATCHES "^rhi/(d3d9|d3d10|d3d11)/")
        # Direct3D backends need the Windows SDK.
        set(exclude_source TRUE)
    elseif(NOT WIN32 AND source_name MATCHES "_dx(9|10|11)\\.cpp$")
        set(exclude_source TRUE)
    elseif(NOT WIN32 AND source_rel IN_LIST NEXUS_WINDOWS_ONLY_SOURCES)
        set(exclude_source TRUE)
    elseif(NOT NEXUS_VULKAN_ENABLED AND source_rel MATCHES "^rhi/vulkan/")
        set(exclude_source TRUE)
    elseif(NOT NEXUS_OPENGL_ENABLED AND source_rel MATCHES "^rhi/opengl/")
        set(exclude_source TRUE)
    endif()

    if(exclude_source)
        list(APPEND NEXUS_EXCLUDED_SOURCES "${source_file}")
    else()
        list(APPEND NEXUS_LIB_SOURCES "${source_file}")
    endif()
endforeach()

list(LENGTH NEXUS_LIB_SOURCES NUM_SOURCES)
list(LENGTH NEXUS_EXCLUDED_SOURCES NUM_EXCLUDED_SOURCES)
message(STATUS "Building NexusCore with ${NUM_SOURCES} source files")
message(STATUS "Excluded ${NUM_EXCLUDED_SOURCES} alternate/tool source files from NexusCore")

add_library(NexusCore STATIC ${NEXUS_LIB_SOURCES})

target_include_directories(NexusCore
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
        ${NEXUS_THIRD_PARTY_INCLUDE_DIRS}
)

if(JOLT_FOUND)
    target_include_directories(NexusCore PUBLIC ${JOLT_INCLUDE_DIRS})
    target_link_libraries(NexusCore PUBLIC ${JOLT_LIBRARIES})
endif()

if(NEXUS_LUA_ENABLED)
    target_include_directories(NexusCore PUBLIC ${LUA_INCLUDE_DIR})
    target_link_libraries(NexusCore PUBLIC ${LUA_LIBRARIES})
endif()

if(NEXUS_PYTHON_ENABLED)
    target_include_directories(NexusCore PUBLIC ${Python3_INCLUDE_DIRS})
    if(TARGET Python3::Python)
        target_link_libraries(NexusCore PUBLIC Python3::Python)
    else()
        target_link_libraries(NexusCore PUBLIC ${Python3_LIBRARIES})
    endif()
endif()

if(PHYSX_FOUND)
    target_include_directories(NexusCore PUBLIC ${PHYSX_INCLUDE_DIRS})
    target_link_libraries(NexusCore PUBLIC ${PHYSX_LIBRARIES})
endif()

if(FMOD_FOUND)
    target_include_directories(NexusCore PUBLIC ${FMOD_INCLUDE_DIRS})
    target_link_libraries(NexusCore PUBLIC ${FMOD_LIBRARIES})
endif()

if(ASSIMP_FOUND)
    if(TARGET assimp::assimp)
        target_link_libraries(NexusCore PUBLIC assimp::assimp)
    else()
        target_include_directories(NexusCore PUBLIC ${ASSIMP_INCLUDE_DIRS})
        target_link_libraries(NexusCore PUBLIC ${ASSIMP_LIBRARIES})
    endif()
endif()

if(TARGET NexusImGui)
    target_link_libraries(NexusCore PUBLIC NexusImGui)
endif()

# NexusSDL2 is the normalised interface target the root CMakeLists builds from
# whichever discovery mechanism succeeded; see the SDL2 block there.
if(TARGET NexusSDL2)
    target_link_libraries(NexusCore PUBLIC NexusSDL2)
endif()

if(NEXUS_VULKAN_ENABLED)
    target_include_directories(NexusCore PUBLIC ${Vulkan_INCLUDE_DIRS})
    if(TARGET Vulkan::Vulkan)
        target_link_libraries(NexusCore PUBLIC Vulkan::Vulkan)
    else()
        # Headers only - need to link vulkan-1 manually on Windows
        if(WIN32)
            # Try to find vulkan-1.lib in system paths or SDK
            find_library(VULKAN_LIBRARY vulkan-1
                PATHS "$ENV{VULKAN_SDK}/Lib"
                      "C:/VulkanSDK/*/Lib"
                PATH_SUFFIXES x64 x86_64
            )
            if(VULKAN_LIBRARY)
                target_link_libraries(NexusCore PUBLIC ${VULKAN_LIBRARY})
                message(STATUS "Linked Vulkan library: ${VULKAN_LIBRARY}")
            else()
                message(WARNING "Vulkan headers found but vulkan-1.lib not found. Install full Vulkan SDK for linking.")
            endif()
        elseif(UNIX)
            find_library(VULKAN_LIBRARY vulkan)
            if(VULKAN_LIBRARY)
                target_link_libraries(NexusCore PUBLIC ${VULKAN_LIBRARY})
            endif()
        endif()
    endif()
endif()

if(NEXUS_OPENGL_ENABLED)
    target_link_libraries(NexusCore PUBLIC OpenGL::GL)
    if(TARGET glad)
        target_link_libraries(NexusCore PUBLIC glad)
        message(STATUS "Linked GLAD library to NexusCore")
    endif()
endif()

target_compile_definitions(NexusCore
    PUBLIC
        NEXUS_STATIC_BUILD
        $<$<BOOL:${NEXUS_PYTHON_ENABLED}>:NEXUS_PYTHON_ENABLED>
        $<$<BOOL:${NEXUS_LUA_ENABLED}>:NEXUS_LUA_ENABLED>
        $<$<BOOL:${NEXUS_JOLT_PHYSICS_ENABLED}>:NEXUS_JOLT_PHYSICS_ENABLED>
        $<$<BOOL:${NEXUS_PHYSX_ENABLED}>:NEXUS_PHYSX_ENABLED>
        $<$<BOOL:${NEXUS_FMOD_ENABLED}>:NEXUS_FMOD_ENABLED>
        $<$<BOOL:${NEXUS_ASSIMP_ENABLED}>:NEXUS_ASSIMP_ENABLED>
        $<$<BOOL:${NEXUS_IMGUI_ENABLED}>:NEXUS_IMGUI_ENABLED>
        $<$<BOOL:${NEXUS_SDL2_ENABLED}>:NEXUS_SDL2_ENABLED>
        $<$<BOOL:${NEXUS_VULKAN_ENABLED}>:NEXUS_VULKAN_ENABLED>
        $<$<BOOL:${NEXUS_OPENGL_ENABLED}>:NEXUS_OPENGL_ENABLED>
        $<$<BOOL:${NEXUS_DIRECTX11_ENABLED}>:NEXUS_DIRECTX11_ENABLED>
        $<$<BOOL:${NEXUS_ADVANCED_RENDERING_ENABLED}>:NEXUS_ADVANCED_RENDERING_ENABLED>
        $<$<BOOL:${NEXUS_RAY_TRACING_ENABLED}>:NEXUS_RAY_TRACING_ENABLED>
        $<$<BOOL:${NEXUS_VR_ENABLED}>:NEXUS_VR_ENABLED>
        $<$<BOOL:${NEXUS_NETWORKING_ENABLED}>:NEXUS_NETWORKING_ENABLED>
        $<$<BOOL:${NEXUS_C_API_ENABLED}>:NEXUS_C_API_ENABLED>
        $<$<BOOL:${NEXUS_GAME_IMPORTERS_ENABLED}>:NEXUS_GAME_IMPORTERS_ENABLED>
        $<$<BOOL:${NEXUS_CONSOLE_PLATFORMS_ENABLED}>:NEXUS_CONSOLE_PLATFORMS_ENABLED>
)

if(WIN32)
    target_compile_definitions(NexusCore
        PUBLIC
            NEXUS_PLATFORM_WINDOWS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            UNICODE
            _UNICODE
    )

    target_link_libraries(NexusCore
        PUBLIC
            d3d11.lib dxgi.lib d3dcompiler.lib
            dinput8.lib dxguid.lib dsound.lib winmm.lib
            user32.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib
            uuid.lib comdlg32.lib advapi32.lib psapi.lib pdh.lib
            comctl32.lib kernel32.lib ws2_32.lib setupapi.lib version.lib
    )
else()
    target_compile_definitions(NexusCore
        PUBLIC
            $<$<PLATFORM_ID:Linux>:NEXUS_PLATFORM_LINUX>
            $<$<PLATFORM_ID:Darwin>:NEXUS_PLATFORM_MACOS>
    )

    find_package(Threads REQUIRED)
    target_link_libraries(NexusCore PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

set_target_properties(NexusCore PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# The engine executable drives the legacy Direct3D 11 runtime listed above, so
# it can only be linked where that layer was compiled. Declaring the target
# anyway would produce a confusing wall of undefined symbols; skipping it with
# an explicit message keeps the failure legible.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    if(WIN32)
        add_executable(NexusEngine main.cpp)
        target_link_libraries(NexusEngine PRIVATE NexusCore)
        set_target_properties(NexusEngine PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
        message(STATUS "Created NexusEngine executable")
    else()
        message(STATUS "NexusEngine executable skipped: requires the Direct3D 11 runtime layer")
        message(STATUS "  NexusCore, the tools and the tests still build on this platform")
    endif()
endif()

# ---------------------------------------------------------------------------
# Cross-platform demo
#
# The NexusEngine executable above needs the Direct3D 11 runtime layer and so
# exists only on Windows. This is the runnable target everywhere else: it opens
# a window through Nexus::Window and drives an RHI device on it, which is the
# whole cross-platform path in one program. It needs a window, so it is built
# only where SDL2 was found and at least one portable backend is enabled.
# ---------------------------------------------------------------------------
if(NEXUS_SDL2_ENABLED AND (NEXUS_VULKAN_ENABLED OR NEXUS_OPENGL_ENABLED OR NEXUS_DIRECTX11_ENABLED)
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/rhi_demo.cpp")
    add_executable(NexusRHIDemo tools/rhi_demo.cpp)
    target_link_libraries(NexusRHIDemo PRIVATE NexusCore)
    set_target_properties(NexusRHIDemo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    message(STATUS "Created NexusRHIDemo (cross-platform window + RHI sample)")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/rhi_demo.cpp")
    message(STATUS "NexusRHIDemo skipped: needs SDL2 and one of Vulkan/OpenGL/DirectX 11")
endif()

if(NEXUS_BUILD_TOOLS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/asset_converter.cpp")
    add_executable(NexusAssetConverter tools/asset_converter.cpp tools/AssetConverter.cpp)
    target_link_libraries(NexusAssetConverter PRIVATE NexusCore)
    target_include_directories(NexusAssetConverter PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
    )
    set_target_properties(NexusAssetConverter PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    message(STATUS "Created NexusAssetConverter tool")
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

message(STATUS "NexusCore library configured successfully")

