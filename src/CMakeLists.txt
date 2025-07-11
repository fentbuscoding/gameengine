cmake_minimum_required(VERSION 3.16)

# Find all existing source files
file(GLOB_RECURSE ALL_CPP_FILES "*.cpp")

# Remove main files from library sources (they contain main functions)  
set(MAIN_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main_simple.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main_debug.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/texture_test.cpp"
)

set(NEXUS_LIB_SOURCES ${ALL_CPP_FILES})
foreach(main_file ${MAIN_FILES})
    list(REMOVE_ITEM NEXUS_LIB_SOURCES ${main_file})
endforeach()

# Remove tool sources from library (they have their own main functions)
file(GLOB TOOL_SOURCES "tools/*.cpp")
foreach(tool_file ${TOOL_SOURCES})
    list(REMOVE_ITEM NEXUS_LIB_SOURCES ${tool_file})
endforeach()

list(LENGTH NEXUS_LIB_SOURCES NUM_SOURCES)
message(STATUS "Building NexusCore with ${NUM_SOURCES} source files")

# Create the main engine library
add_library(NexusCore STATIC ${NEXUS_LIB_SOURCES})

target_include_directories(NexusCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

# Link required Windows libraries
target_link_libraries(NexusCore
    d3d11.lib dxgi.lib d3dcompiler.lib
    dinput8.lib dxguid.lib dsound.lib winmm.lib
    user32.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib
    uuid.lib comdlg32.lib advapi32.lib psapi.lib pdh.lib
    comctl32.lib kernel32.lib ws2_32.lib setupapi.lib version.lib
)

# Set target properties
set_target_properties(NexusCore PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Platform-specific compilation flags
if(WIN32)
    target_compile_definitions(NexusCore PRIVATE 
        NEXUS_PLATFORM_WINDOWS
        NOMINMAX WIN32_LEAN_AND_MEAN
        UNICODE _UNICODE
    )
endif()

# Create main executable if main.cpp exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    add_executable(NexusEngine main.cpp)
    target_link_libraries(NexusEngine NexusCore)
    set_target_properties(NexusEngine PROPERTIES
        OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    message(STATUS "Created NexusEngine executable")
endif()

# Create directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

message(STATUS "NexusCore library configured successfully")
