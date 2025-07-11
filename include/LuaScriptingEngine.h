#pragma once

#ifdef NEXUS_LUA_ENABLED
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

#include <memory>
#include <string>
#include <functional>
#include <map>
#include <vector>

namespace Nexus {

class Engine;

/**
 * Lua scripting engine for game logic
 */
class LuaScriptingEngine {
public:
    LuaScriptingEngine();
    ~LuaScriptingEngine();

    // Initialization
    bool Initialize(Engine* engine);
    void Shutdown();

    // Script execution
    bool ExecuteFile(const std::string& filename);
    bool ExecuteString(const std::string& code);
    
    // Variable access
    void SetGlobal(const std::string& name, double value);
    void SetGlobal(const std::string& name, const std::string& value);
    void SetGlobal(const std::string& name, bool value);
    
    double GetGlobalNumber(const std::string& name);
    std::string GetGlobalString(const std::string& name);
    bool GetGlobalBool(const std::string& name);

    // Function calls
    bool CallFunction(const std::string& functionName);
    bool CallFunction(const std::string& functionName, double arg);
    bool CallFunction(const std::string& functionName, const std::string& arg);
    
    // C function registration
    void RegisterFunction(const std::string& name, lua_CFunction func);
    
    // Hot reloading
    void EnableHotReload(bool enable) { hotReloadEnabled_ = enable; }
    void CheckForChanges();
    void ReloadModifiedScripts();

    // Engine integration
    void SetEngine(Engine* engine) { engine_ = engine; }
    Engine* GetEngine() const { return engine_; }

    // Event system
    void RegisterEventCallback(const std::string& eventName, std::function<void()> callback);
    void TriggerEvent(const std::string& eventName);

    // Update loop
    void Update(float deltaTime);

    // Get raw Lua state for advanced operations
    lua_State* GetLuaState() const { return L_; }

private:
    void InitializeLuaBindings();
    void AddToPath(const std::string& path);
    void RegisterEngineFunctions();
    
    Engine* engine_;
    bool initialized_;
    bool hotReloadEnabled_;
    
#ifdef NEXUS_LUA_ENABLED
    lua_State* L_;
#else
    void* L_; // Placeholder when Lua is disabled
#endif
    
    std::map<std::string, std::function<void()>> eventCallbacks_;
    std::map<std::string, long long> scriptModTimes_;
};

} // namespace Nexus