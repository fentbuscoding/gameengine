cmake_minimum_required(VERSION 3.16)

# Set project name and version
project(NexusEngine VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set policies for modern CMake
cmake_policy(SET CMP0135 NEW)  # Handle download timestamps

# Build configuration
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# Feature options
option(ENABLE_PYTHON "Enable Python scripting support" ON)
option(ENABLE_LUA "Enable Lua scripting support" ON)
option(ENABLE_BULLET_PHYSICS "Enable Bullet Physics" ON)
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
option(ENABLE_EXAMPLES "Enable example projects" OFF)  # Disabled for now

# Find Python for scripting
if(ENABLE_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development)
    if(Python3_FOUND)
        message(STATUS "Found Python ${Python3_VERSION}: ${Python3_EXECUTABLE}")
        message(STATUS "Python include dirs: ${Python3_INCLUDE_DIRS}")
        message(STATUS "Python libraries: ${Python3_LIBRARIES}")
    else()
        message(WARNING "Python not found - Python scripting will be disabled")
        set(ENABLE_PYTHON OFF)
    endif()
endif()

# Find required packages
find_package(DirectX QUIET)

# Try to find Bullet Physics
if(ENABLE_BULLET_PHYSICS)
    # Check multiple possible locations for Bullet
    set(BULLET_SEARCH_PATHS
        "${CMAKE_SOURCE_DIR}/third_party/bullet3"
        "${CMAKE_SOURCE_DIR}/third_party/bullet"
        "$ENV{BULLET_ROOT}"
        "C:/Program Files/bullet3"
        "C:/bullet3"
    )
    
    foreach(search_path ${BULLET_SEARCH_PATHS})
        if(EXISTS "${search_path}/src")
            set(BULLET_ROOT "${search_path}")
            break()
        endif()
    endforeach()
    
    if(BULLET_ROOT AND EXISTS "${BULLET_ROOT}/src")
        message(STATUS "Found Bullet Physics at: ${BULLET_ROOT}")
        
        # Add Bullet as subdirectory if CMakeLists.txt exists
        if(EXISTS "${BULLET_ROOT}/CMakeLists.txt")
            # Configure Bullet options
            set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "")
            set(BUILD_BULLET3 ON CACHE BOOL "")
            set(BUILD_EXTRAS OFF CACHE BOOL "")
            set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "")
            set(BUILD_UNIT_TESTS OFF CACHE BOOL "")
            set(BUILD_CPU_DEMOS OFF CACHE BOOL "")
            set(INSTALL_LIBS OFF CACHE BOOL "")
            
            add_subdirectory(${BULLET_ROOT} ${CMAKE_BINARY_DIR}/build/bullet3)
            
            set(BULLET_FOUND TRUE)
            set(BULLET_INCLUDE_DIRS "${BULLET_ROOT}/src")
            set(BULLET_LIBRARIES 
                BulletDynamics 
                BulletCollision 
                LinearMath
            )
        endif()
    else()
        message(STATUS "Bullet Physics not found - using basic physics")
        set(BULLET_FOUND FALSE)
    endif()
endif()

# Try to find Lua
if(ENABLE_LUA)
    # Check for system Lua first
    find_package(Lua 5.4 QUIET)
    
    if(NOT LUA_FOUND)
        # Try to build Lua from source
        set(LUA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/lua-5.4.7")
        
        if(EXISTS "${LUA_SOURCE_DIR}/src")
            message(STATUS "Building Lua from source at: ${LUA_SOURCE_DIR}")
            
            # Create Lua library
            file(GLOB LUA_SOURCES 
                "${LUA_SOURCE_DIR}/src/*.c"
            )
            # Remove lua.c and luac.c (main functions)
            list(REMOVE_ITEM LUA_SOURCES 
                "${LUA_SOURCE_DIR}/src/lua.c"
                "${LUA_SOURCE_DIR}/src/luac.c"
            )
            
            add_library(lua STATIC ${LUA_SOURCES})
            target_include_directories(lua PUBLIC "${LUA_SOURCE_DIR}/src")
            
            set(LUA_FOUND TRUE)
            set(LUA_INCLUDE_DIR "${LUA_SOURCE_DIR}/src")
            set(LUA_LIBRARIES lua)
            
            message(STATUS "Lua built successfully")
        else()
            message(STATUS "Lua source not found at ${LUA_SOURCE_DIR}")
            message(STATUS "Please download Lua 5.4.7 source and extract to third_party/lua-5.4.7/")
            set(LUA_FOUND FALSE)
        endif()
    else()
        message(STATUS "Found system Lua: ${LUA_VERSION_STRING}")
    endif()
endif()

# Try to find PhysX
if(ENABLE_PHYSX)
    find_path(PHYSX_INCLUDE_DIR
        NAMES PxPhysicsAPI.h
        PATHS
            "${CMAKE_SOURCE_DIR}/third_party/physx/include"
            "$ENV{PHYSX_ROOT}/include"
            "C:/Program Files/NVIDIA Corporation/PhysX/v5.1/include"
        PATH_SUFFIXES PhysX
    )
    
    if(PHYSX_INCLUDE_DIR)
        set(PHYSX_FOUND TRUE)
        message(STATUS "Found PhysX headers at: ${PHYSX_INCLUDE_DIR}")
        
        # Find PhysX libraries
        find_library(PHYSX_LIB
            NAMES PhysX_64 PhysX
            PATHS 
                "${CMAKE_SOURCE_DIR}/third_party/physx/lib"
                "$ENV{PHYSX_ROOT}/lib"
                "C:/Program Files/NVIDIA Corporation/PhysX/v5.1/lib"
            PATH_SUFFIXES x64 x86_64
        )
        
        if(PHYSX_LIB)
            set(PHYSX_LIBRARIES ${PHYSX_LIB})
            set(PHYSX_INCLUDE_DIRS ${PHYSX_INCLUDE_DIR})
        else()
            set(PHYSX_FOUND FALSE)
            message(STATUS "PhysX headers found but libraries not found")
        endif()
    else()
        set(PHYSX_FOUND FALSE)
        message(STATUS "PhysX not found - using Bullet Physics")
    endif()
endif()

# Try to find FMOD
if(ENABLE_FMOD)
    find_path(FMOD_INCLUDE_DIR
        NAMES fmod.h
        PATHS
            "${CMAKE_SOURCE_DIR}/third_party/fmod/include"
            "$ENV{FMOD_ROOT}/include"
            "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc"
            "C:/FMOD/api/core/inc"
    )
    
    if(FMOD_INCLUDE_DIR)
        find_library(FMOD_LIB
            NAMES fmod_vc fmod64_vc fmod
            PATHS
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
            set(FMOD_FOUND FALSE)
            message(STATUS "FMOD headers found but library not found")
        endif()
    else()
        set(FMOD_FOUND FALSE)
        message(STATUS "FMOD not found - using basic audio")
    endif()
endif()

# Try to find Assimp
if(ENABLE_ASSIMP)
    find_package(assimp QUIET)
    
    if(NOT assimp_FOUND)
        find_path(ASSIMP_INCLUDE_DIR
            NAMES assimp/Importer.hpp
            PATHS
                "${CMAKE_SOURCE_DIR}/third_party/assimp/include"
                "$ENV{ASSIMP_ROOT}/include"
                "C:/Program Files/assimp/include"
        )
        
        if(ASSIMP_INCLUDE_DIR)
            find_library(ASSIMP_LIB
                NAMES assimp assimp-vc143-mt
                PATHS
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
    else()
        set(ASSIMP_FOUND TRUE)
        message(STATUS "Found Assimp via CMake")
    endif()
    
    if(NOT ASSIMP_FOUND)
        message(STATUS "Assimp not found - using basic model loading")
    endif()
endif()

# Create build directory for generated headers
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")

# Configure compile definitions based on found packages
if(EXISTS "${CMAKE_SOURCE_DIR}/include/EngineConfig.h.in")
    configure_file(
        "${CMAKE_SOURCE_DIR}/include/EngineConfig.h.in"
        "${CMAKE_BINARY_DIR}/include/EngineConfig.h"
    )
else()
    message(STATUS "EngineConfig.h.in not found - skipping configuration")
endif()

# Add subdirectories
add_subdirectory(src)

# Add examples if enabled and they exist
if(ENABLE_EXAMPLES AND EXISTS "${CMAKE_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# Add Python bindings if enabled and Python is found
if(ENABLE_PYTHON AND Python3_FOUND)
    if(EXISTS "${CMAKE_SOURCE_DIR}/python/CMakeLists.txt")
        add_subdirectory(python)
    endif()
endif()

# Installation
install(TARGETS NexusCore
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY shaders/ DESTINATION share/nexus/shaders)
install(DIRECTORY assets/ DESTINATION share/nexus/assets)

# Print configuration summary
message(STATUS "=== Nexus Engine Configuration Summary ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Python support: ${ENABLE_PYTHON}")
if(ENABLE_PYTHON AND Python3_FOUND)
    message(STATUS "  Python version: ${Python3_VERSION}")
endif()
message(STATUS "Lua support: ${ENABLE_LUA}")
if(LUA_FOUND)
    message(STATUS "  Lua version: 5.4.7")
endif()
message(STATUS "Bullet Physics: ${BULLET_FOUND}")
message(STATUS "NVIDIA PhysX: ${PHYSX_FOUND}")
message(STATUS "FMOD Audio: ${FMOD_FOUND}")
message(STATUS "Assimp: ${ASSIMP_FOUND}")
message(STATUS "Advanced Rendering: ${ENABLE_ADVANCED_RENDERING}")
message(STATUS "Ray Tracing: ${ENABLE_RAY_TRACING}")
message(STATUS "VR Support: ${ENABLE_VR_SUPPORT}")
message(STATUS "Game Importers: ${ENABLE_GAME_IMPORTERS}")
message(STATUS "Console Platforms: ${ENABLE_CONSOLE_PLATFORMS}")
message(STATUS "Examples: ${ENABLE_EXAMPLES}")
message(STATUS "==========================================")
