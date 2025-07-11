cmake_minimum_required(VERSION 3.16)

if(ENABLE_LUA AND LUA_FOUND)
    message(STATUS "Building Lua examples and utilities")
    
    # Lua game runner executable
    add_executable(NexusLua
        lua_runner.cpp
    )
    
    target_link_libraries(NexusLua 
        NexusCore 
        ${LUA_LIBRARIES}
    )
    
    # Copy Lua examples
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../examples/lua/simple_game.lua
        ${CMAKE_BINARY_DIR}/bin/examples/lua/simple_game.lua
        COPYONLY
    )
    
    # Create Lua module helpers
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/nexus.lua
        ${CMAKE_BINARY_DIR}/bin/nexus.lua
        COPYONLY
    )
    
    message(STATUS "Lua integration configured successfully")
endif()