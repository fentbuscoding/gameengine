#include "ScriptingEngine.h"
#include "Engine.h"
#include "Logger.h"
#include <iostream>

namespace Nexus {

ScriptingEngine::ScriptingEngine()
    : engine_(nullptr)
    , initialized_(false)
    , hotReloadEnabled_(false)
{
}

ScriptingEngine::~ScriptingEngine() {
    Shutdown();
}

bool ScriptingEngine::Initialize(Engine* engine) {
    if (initialized_) return true;
    
    engine_ = engine;
    
    try {
#ifdef NEXUS_PYTHON_ENABLED
        // Initialize Python interpreter
        Py_Initialize();
        if (!Py_IsInitialized()) {
            Logger::Error("Failed to initialize Python interpreter");
            return false;
        }
#else
        Logger::Warning("Python support not enabled in this build");
        return false;
#endif
        
        // Add current directory to Python path
        AddToPath(".");
        AddToPath("python");
        AddToPath("examples");
        
        // Initialize Python bindings
        InitializePythonBindings();
        
        initialized_ = true;
        Logger::Info("Scripting engine initialized");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to initialize scripting engine: " + std::string(e.what()));
        return false;
    }
}

void ScriptingEngine::Shutdown() {
    if (!initialized_) return;
    
#ifdef NEXUS_PYTHON_ENABLED
    // Cleanup Python
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
#endif
    
    initialized_ = false;
    Logger::Info("Scripting engine shutdown");
}

bool ScriptingEngine::ExecuteFile(const std::string& filename) {
    if (!initialized_) {
        Logger::Error("Scripting engine not initialized");
        return false;
    }
    
    try {
        FILE* file = fopen(filename.c_str(), "r");
        if (!file) {
            Logger::Error("Could not open script file: " + filename);
            return false;
        }
        
#ifdef NEXUS_PYTHON_ENABLED
        int result = PyRun_SimpleFile(file, filename.c_str());
#else
        int result = -1; // Error when Python not enabled
#endif
        fclose(file);
        
        if (result != 0) {
            Logger::Error("Error executing script: " + filename);
            return false;
        }
        
        Logger::Info("Successfully executed script: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception executing script " + filename + ": " + std::string(e.what()));
        return false;
    }
}

bool ScriptingEngine::ExecuteString(const std::string& code) {
    if (!initialized_) {
        Logger::Error("Scripting engine not initialized");
        return false;
    }
    
    try {
#ifdef NEXUS_PYTHON_ENABLED
        int result = PyRun_SimpleString(code.c_str());
#else
        int result = -1; // Error when Python not enabled
#endif
        if (result != 0) {
            Logger::Error("Error executing Python code");
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Exception executing Python code: " + std::string(e.what()));
        return false;
    }
}

void ScriptingEngine::CheckForChanges() {
    if (!hotReloadEnabled_ || !initialized_) return;
    
    // Check modification times of loaded scripts
    // Reload if changed
}

void ScriptingEngine::ReloadModifiedScripts() {
    if (!hotReloadEnabled_ || !initialized_) return;
    
    // Reload modified scripts
}

void ScriptingEngine::RegisterEventCallback(const std::string& eventName, std::function<void()> callback) {
    eventCallbacks_[eventName] = callback;
}

void ScriptingEngine::TriggerEvent(const std::string& eventName) {
    auto it = eventCallbacks_.find(eventName);
    if (it != eventCallbacks_.end()) {
        it->second();
    }
}

void ScriptingEngine::Update(float deltaTime) {
    if (!initialized_) return;
    
    // Check for hot reload
    if (hotReloadEnabled_) {
        CheckForChanges();
    }
}

void ScriptingEngine::InitializePythonBindings() {
    // Python bindings would be initialized here
    // This would typically involve importing the nexus_engine module
    ExecuteString("import sys");
    ExecuteString("print('Python version:', sys.version)");
}

void ScriptingEngine::AddToPath(const std::string& path) {
    std::string pythonCode = "import sys; sys.path.append('" + path + "')";
#ifdef NEXUS_PYTHON_ENABLED
    PyRun_SimpleString(pythonCode.c_str());
#else
    Logger::Warning("Python support not enabled - cannot add to path: " + path);
#endif
}

} // namespace Nexus
