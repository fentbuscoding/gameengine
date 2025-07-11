#include "LuaScriptingEngine.h"
#include "Engine.h"
#include "Logger.h"
#include "GameModuleAPI.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef NEXUS_LUA_ENABLED
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

namespace Nexus {

LuaScriptingEngine::LuaScriptingEngine()
    : engine_(nullptr)
    , initialized_(false)
    , hotReloadEnabled_(false)
    , L_(nullptr)
{
}

LuaScriptingEngine::~LuaScriptingEngine() {
    Shutdown();
}

bool LuaScriptingEngine::Initialize(Engine* engine) {
    if (initialized_) return true;
    
    engine_ = engine;
    
    try {
#ifdef NEXUS_LUA_ENABLED
        // Create new Lua state
        L_ = luaL_newstate();
        if (!L_) {
            Logger::Error("Failed to create Lua state");
            return false;
        }
        
        // Open standard libraries
        luaL_openlibs(L_);
        
        // Initialize Lua bindings
        InitializeLuaBindings();
        RegisterEngineFunctions();
        
        // Add script paths
        AddToPath(".");
        AddToPath("scripts/lua");
        AddToPath("games/lua");
        
        initialized_ = true;
        Logger::Info("Lua scripting engine initialized");
        return true;
#else
        Logger::Warning("Lua support not enabled in this build");
        return false;
#endif
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to initialize Lua scripting engine: " + std::string(e.what()));
        return false;
    }
}

void LuaScriptingEngine::Shutdown() {
    if (!initialized_) return;
    
#ifdef NEXUS_LUA_ENABLED
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
#endif
    
    initialized_ = false;
    Logger::Info("Lua scripting engine shutdown");
}

bool LuaScriptingEngine::ExecuteFile(const std::string& filename) {
    if (!initialized_) {
        Logger::Error("Lua scripting engine not initialized");
        return false;
    }
    
#ifdef NEXUS_LUA_ENABLED
    try {
        int result = luaL_dofile(L_, filename.c_str());
        if (result != LUA_OK) {
            std::string error = lua_tostring(L_, -1);
            lua_pop(L_, 1);
            Logger::Error("Error executing Lua script " + filename + ": " + error);
            return false;
        }
        
        Logger::Info("Successfully executed Lua script: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception executing Lua script " + filename + ": " + std::string(e.what()));
        return false;
    }
#else
    Logger::Error("Lua support not enabled");
    return false;
#endif
}

bool LuaScriptingEngine::ExecuteString(const std::string& code) {
    if (!initialized_) {
        Logger::Error("Lua scripting engine not initialized");
        return false;
    }
    
#ifdef NEXUS_LUA_ENABLED
    try {
        int result = luaL_dostring(L_, code.c_str());
        if (result != LUA_OK) {
            std::string error = lua_tostring(L_, -1);
            lua_pop(L_, 1);
            Logger::Error("Error executing Lua code: " + error);
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception executing Lua code: " + std::string(e.what()));
        return false;
    }
#else
    Logger::Error("Lua support not enabled");
    return false;
#endif
}

void LuaScriptingEngine::SetGlobal(const std::string& name, double value) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return;
    lua_pushnumber(L_, value);
    lua_setglobal(L_, name.c_str());
#endif
}

void LuaScriptingEngine::SetGlobal(const std::string& name, const std::string& value) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return;
    lua_pushstring(L_, value.c_str());
    lua_setglobal(L_, name.c_str());
#endif
}

void LuaScriptingEngine::SetGlobal(const std::string& name, bool value) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return;
    lua_pushboolean(L_, value);
    lua_setglobal(L_, name.c_str());
#endif
}

double LuaScriptingEngine::GetGlobalNumber(const std::string& name) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return 0.0;
    lua_getglobal(L_, name.c_str());
    double result = lua_tonumber(L_, -1);
    lua_pop(L_, 1);
    return result;
#else
    return 0.0;
#endif
}

std::string LuaScriptingEngine::GetGlobalString(const std::string& name) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return "";
    lua_getglobal(L_, name.c_str());
    std::string result = lua_tostring(L_, -1);
    lua_pop(L_, 1);
    return result;
#else
    return "";
#endif
}

bool LuaScriptingEngine::GetGlobalBool(const std::string& name) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return false;
    lua_getglobal(L_, name.c_str());
    bool result = lua_toboolean(L_, -1);
    lua_pop(L_, 1);
    return result;
#else
    return false;
#endif
}

bool LuaScriptingEngine::CallFunction(const std::string& functionName) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return false;
    
    lua_getglobal(L_, functionName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    
    int result = lua_pcall(L_, 0, 0, 0);
    return result == LUA_OK;
#else
    return false;
#endif
}

bool LuaScriptingEngine::CallFunction(const std::string& functionName, double arg) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return false;
    
    lua_getglobal(L_, functionName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    
    lua_pushnumber(L_, arg);
    int result = lua_pcall(L_, 1, 0, 0);
    return result == LUA_OK;
#else
    return false;
#endif
}

bool LuaScriptingEngine::CallFunction(const std::string& functionName, const std::string& arg) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return false;
    
    lua_getglobal(L_, functionName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    
    lua_pushstring(L_, arg.c_str());
    int result = lua_pcall(L_, 1, 0, 0);
    return result == LUA_OK;
#else
    return false;
#endif
}

void LuaScriptingEngine::RegisterFunction(const std::string& name, lua_CFunction func) {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return;
    lua_register(L_, name.c_str(), func);
#endif
}

void LuaScriptingEngine::CheckForChanges() {
    if (!hotReloadEnabled_ || !initialized_) return;
    
    // Check modification times of loaded scripts
    // Reload if changed
}

void LuaScriptingEngine::ReloadModifiedScripts() {
    if (!hotReloadEnabled_ || !initialized_) return;
    
    // Reload modified scripts
}

void LuaScriptingEngine::RegisterEventCallback(const std::string& eventName, std::function<void()> callback) {
    eventCallbacks_[eventName] = callback;
}

void LuaScriptingEngine::TriggerEvent(const std::string& eventName) {
    auto it = eventCallbacks_.find(eventName);
    if (it != eventCallbacks_.end()) {
        it->second();
    }
}

void LuaScriptingEngine::Update(float deltaTime) {
    if (!initialized_) return;
    
    // Update delta time
    SetGlobal("deltaTime", deltaTime);
    
    // Call update function if it exists
    CallFunction("update", deltaTime);
    
    // Check for hot reload
    if (hotReloadEnabled_) {
        CheckForChanges();
    }
}

void LuaScriptingEngine::InitializeLuaBindings() {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return;
    
    // Create nexus table
    lua_newtable(L_);
    lua_setglobal(L_, "nexus");
    
    // Set version info
    SetGlobal("NEXUS_VERSION", "1.0.0");
    SetGlobal("LUA_SUPPORT", true);
#endif
}

void LuaScriptingEngine::RegisterEngineFunctions() {
#ifdef NEXUS_LUA_ENABLED
    if (!initialized_) return;
    
    // Register C API functions as Lua functions
    lua_register(L_, "log_info", [](lua_State* L) {
        const char* msg = lua_tostring(L, 1);
        if (msg) nexus_log_info(msg);
        return 0;
    });
    
    lua_register(L_, "log_warning", [](lua_State* L) {
        const char* msg = lua_tostring(L, 1);
        if (msg) nexus_log_warning(msg);
        return 0;
    });
    
    lua_register(L_, "log_error", [](lua_State* L) {
        const char* msg = lua_tostring(L, 1);
        if (msg) nexus_log_error(msg);
        return 0;
    });
    
    lua_register(L_, "get_delta_time", [](lua_State* L) {
        lua_pushnumber(L, nexus_get_delta_time());
        return 1;
    });
    
    lua_register(L_, "get_fps", [](lua_State* L) {
        lua_pushinteger(L, nexus_get_fps());
        return 1;
    });
    
    lua_register(L_, "is_key_pressed", [](lua_State* L) {
        int key = (int)lua_tointeger(L, 1);
        lua_pushboolean(L, nexus_is_key_pressed(key));
        return 1;
    });
    
    lua_register(L_, "clear_screen", [](lua_State* L) {
        float r = (float)lua_tonumber(L, 1);
        float g = (float)lua_tonumber(L, 2);
        float b = (float)lua_tonumber(L, 3);
        float a = (float)lua_tonumber(L, 4);
        nexus_clear_screen(r, g, b, a);
        return 0;
    });
    
    lua_register(L_, "present_frame", [](lua_State* L) {
        nexus_present_frame();
        return 0;
    });
#endif
}

void LuaScriptingEngine::AddToPath(const std::string& path) {
    std::string luaCode = "package.path = package.path .. ';./" + path + "/?.lua'";
    ExecuteString(luaCode);
}

} // namespace Nexus