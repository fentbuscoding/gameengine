# Python bindings for Nexus Engine
cmake_minimum_required(VERSION 3.16)

# Find Python and pybind11
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Try to find pybind11 via CMake first
find_package(pybind11 2.10.0 QUIET)

if(NOT pybind11_FOUND)
    message("-- pybind11 not found via CMake, trying Python discovery...")
    
    # Try to find pybind11 through Python
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir())"
        OUTPUT_VARIABLE pybind11_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    if(pybind11_DIR)
        message("-- Found pybind11 at: ${pybind11_DIR}")
        find_package(pybind11 REQUIRED PATHS ${pybind11_DIR})
    else()
        message(FATAL_ERROR "pybind11 not found! Please install it with: pip install pybind11")
    endif()
endif()

# Find existing Python binding sources
set(PYTHON_BINDING_SOURCES)

# Check for existing binding files
file(GLOB EXISTING_BINDINGS "bindings/*.cpp")
foreach(binding_file ${EXISTING_BINDINGS})
    list(APPEND PYTHON_BINDING_SOURCES ${binding_file})
endforeach()

# If no bindings exist, create a minimal one
if(NOT PYTHON_BINDING_SOURCES)
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/bindings/minimal_bindings.cpp"
        "#include <pybind11/pybind11.h>\n\n"
        "PYBIND11_MODULE(nexus_engine, m) {\n"
        "    m.doc() = \"Nexus Game Engine Python Bindings\";\n"
        "    m.def(\"hello_world\", []() { return \"Hello from Nexus Engine!\"; });\n"
        "}\n"
    )
    list(APPEND PYTHON_BINDING_SOURCES "bindings/minimal_bindings.cpp")
endif()

# Create the Python module - use unique name to avoid conflicts
if(pybind11_FOUND AND PYTHON_BINDING_SOURCES)
    pybind11_add_module(nexus_python_bindings
        ${PYTHON_BINDING_SOURCES}
    )
    
    # Set the module name to nexus_engine for Python imports
    set_target_properties(nexus_python_bindings PROPERTIES 
        OUTPUT_NAME "nexus_engine"
        CXX_VISIBILITY_PRESET "hidden"
        VISIBILITY_INLINES_HIDDEN YES
    )
    
    # Link against the core engine library
    target_link_libraries(nexus_python_bindings PRIVATE NexusCore)
    
    # Include directories
    target_include_directories(nexus_python_bindings PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/include
    )
    
    # Compiler-specific options
    target_compile_definitions(nexus_python_bindings PRIVATE VERSION_INFO=${PROJECT_VERSION})
    
    # Install the module
    install(TARGETS nexus_python_bindings
        COMPONENT python
        LIBRARY DESTINATION python
        ARCHIVE DESTINATION python
        RUNTIME DESTINATION python
    )
else()
    message(WARNING "Could not create Python bindings - missing pybind11 or source files")
endif()

# Copy Python API files if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nexus_api.py")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/nexus_api.py
        ${CMAKE_BINARY_DIR}/bin/nexus_api.py
        COPYONLY
    )
endif()
