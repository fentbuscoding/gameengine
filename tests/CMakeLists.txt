# ---------------------------------------------------------------------------
# Nexus Engine test suite
#
# Tests link only against the portable core, so they build and run on every
# platform - which is the point: they are what stops the cross-platform build
# from silently rotting again.
# ---------------------------------------------------------------------------

set(NEXUS_TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${NEXUS_THIRD_PARTY_INCLUDE_DIRS}
)

# Registers one test executable and its CTest entry.
function(nexus_add_test test_name)
    add_executable(${test_name} ${ARGN})

    target_include_directories(${test_name} PRIVATE ${NEXUS_TEST_INCLUDE_DIRS})

    set_target_properties(${test_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    )

    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra)
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})

    # A test that crashes or hangs should fail the run, not stall CI.
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 120)
endfunction()

# The math tests exercise compat/win32/DirectXMath.h only, so they deliberately
# do not link the engine - on Windows this compiles against the real SDK header
# and becomes a conformance check of the portable version against the original.
nexus_add_test(NexusMathTests MathTests.cpp)

# Parser tests cover the header-only Valve block extractor.
nexus_add_test(NexusParserTests ParserTests.cpp)

# Core tests need the platform layer and timer implementations.
nexus_add_test(NexusCoreTests
    CoreTests.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/Platform.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Timer.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Logger.cpp
)

# ---------------------------------------------------------------------------
# Header self-containment
#
# Every public header gets a generated translation unit that includes it and
# nothing else. This is what catches the class of bug that made the engine
# Windows-only in the first place: a header that happens to compile because
# some *other* header included <algorithm> or <d3d11.h> before it, and breaks
# the moment the include order changes or a platform differs.
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Case-insensitive filename collisions
#
# macOS and Windows use case-insensitive filesystems by default, so two tracked
# files differing only in case cannot both exist in a checkout - one clobbers
# the other, and the resulting breakage never reproduces on Linux. The compat
# layer is the obvious place for this to happen (Windows.h vs windows.h), so it
# is checked at configure time rather than left for someone to hit.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE NEXUS_ALL_TRACKED_FILES
    RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/compat/*"
    "${CMAKE_SOURCE_DIR}/include/*"
    "${CMAKE_SOURCE_DIR}/src/*"
    "${CMAKE_SOURCE_DIR}/tests/*"
)

set(NEXUS_LOWERCASED_PATHS)
foreach(tracked IN LISTS NEXUS_ALL_TRACKED_FILES)
    string(TOLOWER "${tracked}" tracked_lower)
    if(tracked_lower IN_LIST NEXUS_LOWERCASED_PATHS)
        message(FATAL_ERROR
            "Case-insensitive filename collision on '${tracked}'.\n"
            "Two files differ only by case, which cannot be checked out on "
            "macOS or Windows. Rename one of them.")
    endif()
    list(APPEND NEXUS_LOWERCASED_PATHS "${tracked_lower}")
endforeach()

option(NEXUS_TEST_HEADER_SELF_CONTAINMENT "Compile each public header on its own" ON)

if(NEXUS_TEST_HEADER_SELF_CONTAINMENT)
    file(GLOB NEXUS_PUBLIC_HEADERS "${CMAKE_SOURCE_DIR}/include/*.h")

    # Headers that require a third-party SDK this repository does not ship.
    # They now fail with an explicit #error naming the missing dependency
    # instead of a cascade of unknown-type errors, so there is nothing useful
    # left for this check to verify. Everything else - including headers whose
    # *implementation* is Windows-only - must still parse on every platform.
    set(NEXUS_HEADER_SELFTEST_SKIP
        PhysXEngine.h             # needs the NVIDIA PhysX SDK (ENABLE_PHYSX)
        AdvancedPhysicsEngine.h   # needs Bullet, which is not vendored here
    )

    set(NEXUS_HEADER_TU_SOURCES)
    set(NEXUS_HEADER_TU_COUNT 0)
    set(NEXUS_HEADER_SKIPPED 0)

    foreach(header IN LISTS NEXUS_PUBLIC_HEADERS)
        get_filename_component(header_name "${header}" NAME)
        get_filename_component(header_we "${header}" NAME_WE)

        if(header_name IN_LIST NEXUS_HEADER_SELFTEST_SKIP)
            math(EXPR NEXUS_HEADER_SKIPPED "${NEXUS_HEADER_SKIPPED} + 1")
            continue()
        endif()

        set(generated "${CMAKE_CURRENT_BINARY_DIR}/header_selftest/${header_we}_selftest.cpp")
        file(WRITE "${generated}"
            "// Generated by tests/CMakeLists.txt - do not edit.\n"
            "// Compiles <${header_name}> as the first include in a translation\n"
            "// unit, so any missing include of its own is a compile error here.\n"
            "#include \"${header_name}\"\n"
        )
        list(APPEND NEXUS_HEADER_TU_SOURCES "${generated}")
        math(EXPR NEXUS_HEADER_TU_COUNT "${NEXUS_HEADER_TU_COUNT} + 1")
    endforeach()

    if(NEXUS_HEADER_TU_SOURCES)
        # A library, not an executable: this checks compilation, and there is no
        # entry point to link.
        add_library(NexusHeaderSelfTest STATIC ${NEXUS_HEADER_TU_SOURCES})

        target_include_directories(NexusHeaderSelfTest PRIVATE ${NEXUS_TEST_INCLUDE_DIRS})

        set_target_properties(NexusHeaderSelfTest PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )

        message(STATUS "Header self-containment check: ${NEXUS_HEADER_TU_COUNT} public headers "
                       "(${NEXUS_HEADER_SKIPPED} skipped: missing third-party SDK)")
    endif()
endif()
