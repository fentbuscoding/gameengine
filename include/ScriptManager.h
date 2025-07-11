#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace Nexus {

class Engine;
class ScriptingEngine;
class LuaScriptingEngine;

/**
 * Unified script manager for multiple scripting languages
 */
class ScriptManager {
public:
    enum class ScriptLanguage {
        Python,
        Lua,
        JavaScript, // Future support
        CSharp      // Future support
    };

    ScriptManager();
    ~ScriptManager();

    // Initialization
    bool Initialize(Engine* engine);
    void Shutdown();

    // Script execution
    bool ExecuteFile(const std::string& filename, ScriptLanguage language = ScriptLanguage::Python);
    bool ExecuteString(const std::string& code, ScriptLanguage language = ScriptLanguage::Python);
    
    // Auto-detect language from file extension
    bool ExecuteFile(const std::string& filename);
    
    // Language-specific engines
    ScriptingEngine* GetPythonEngine() const { return pythonEngine_.get(); }
    LuaScriptingEngine* GetLuaEngine() const { return luaEngine_.get(); }
    
    // Hot reloading
    void EnableHotReload(bool enable);
    void CheckForChanges();
    void ReloadModifiedScripts();

    // Event system (broadcasts to all engines)
    void RegisterEventCallback(const std::string& eventName, std::function<void()> callback);
    void TriggerEvent(const std::string& eventName);

    // Update loop
    void Update(float deltaTime);

    // Game module management
    bool LoadGameModule(const std::string& modulePath);
    void UnloadGameModule(const std::string& moduleName);
    std::vector<std::string> GetLoadedModules() const;

    // Template system
    bool CreateGameTemplate(const std::string& templateName, ScriptLanguage language);
    std::vector<std::string> GetAvailableTemplates() const;

private:
    ScriptLanguage DetectLanguage(const std::string& filename);
    void InitializeTemplates();
    
    Engine* engine_;
    bool initialized_;
    
    std::unique_ptr<ScriptingEngine> pythonEngine_;
    std::unique_ptr<LuaScriptingEngine> luaEngine_;
    
    std::map<std::string, std::function<void()>> eventCallbacks_;
    std::vector<std::string> loadedModules_;
    std::map<std::string, std::string> templates_;
};

} // namespace Nexus