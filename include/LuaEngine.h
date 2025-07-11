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
 * Lua scripting engine for lightweight game scripting
 */
class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();

    // Initialization
    bool Initialize(Engine* engine);
    void Shutdown();

    // Script execution
    bool ExecuteFile(const std::string& filename);
    bool ExecuteString(const std::string& code);
    
    // Hot reloading
    void EnableHotReload(bool enable) { hotReloadEnabled_ = enable; }
    void CheckForChanges();
    void ReloadModifiedScripts();

    // Engine integration
    void SetEngine(Engine* engine) { engine_ = engine; }
    Engine* GetEngine() const { return engine_; }

    // Lua state access
    lua_State* GetLuaState() const { return luaState_; }

    // Variable get/set
    template<typename T>
    void SetGlobal(const std::string& name, T value);
    
    template<typename T>
    T GetGlobal(const std::string& name);

    // Function calling
    template<typename... Args>
    bool CallFunction(const std::string& functionName, Args... args);

    // Event system
    void RegisterCallback(const std::string& eventName, const std::string& luaFunction);
    void TriggerEvent(const std::string& eventName);

    // Update loop
    void Update(float deltaTime);

private:
    void RegisterLuaBindings();
    void AddToPath(const std::string& path);
    
#ifdef NEXUS_LUA_ENABLED
    static int LuaPrint(lua_State* L);
    static int LuaLogError(lua_State* L);
    static int LuaLogInfo(lua_State* L);
    
    // Engine bindings
    static int LuaEngineGetDeltaTime(lua_State* L);
    static int LuaEngineGetFPS(lua_State* L);
    static int LuaEngineRequestExit(lua_State* L);
    
    // Graphics bindings
    static int LuaGraphicsBeginFrame(lua_State* L);
    static int LuaGraphicsEndFrame(lua_State* L);
    static int LuaGraphicsClear(lua_State* L);
    static int LuaGraphicsDrawCube(lua_State* L);
    
    // Input bindings
    static int LuaInputIsKeyPressed(lua_State* L);
    static int LuaInputGetMousePosition(lua_State* L);
    
    // Physics bindings
    static int LuaPhysicsCreateBox(lua_State* L);
    static int LuaPhysicsApplyForce(lua_State* L);

    lua_State* luaState_;
#endif
    Engine* engine_;
    bool initialized_;
    bool hotReloadEnabled_;
    std::map<std::string, std::string> eventCallbacks_;
    std::map<std::string, long long> scriptModTimes_;
};

} // namespace Nexus