#pragma once

// Only include Python if it's available
#ifdef NEXUS_PYTHON_ENABLED
#include <Python.h>
#endif
#include <memory>
#include <string>
#include <functional>
#include <map>
#include <vector>

namespace Nexus {

class Engine;

/**
 * Python scripting engine for game logic
 */
class ScriptingEngine {
public:
    ScriptingEngine();
    ~ScriptingEngine();

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

    // Event system
    void RegisterEventCallback(const std::string& eventName, std::function<void()> callback);
    void TriggerEvent(const std::string& eventName);

    // Update loop
    void Update(float deltaTime);

private:
    void InitializePythonBindings();
    void AddToPath(const std::string& path);
    
    Engine* engine_;
    bool initialized_;
    bool hotReloadEnabled_;
    
    std::map<std::string, std::function<void()>> eventCallbacks_;
    std::map<std::string, long long> scriptModTimes_;
};

} // namespace Nexus
