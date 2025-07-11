#include "ScriptManager.h"
#include "ScriptingEngine.h"
#include "LuaScriptingEngine.h"
#include "Engine.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>

namespace Nexus {

ScriptManager::ScriptManager()
    : engine_(nullptr)
    , initialized_(false)
{
}

ScriptManager::~ScriptManager() {
    Shutdown();
}

bool ScriptManager::Initialize(Engine* engine) {
    if (initialized_) return true;
    
    engine_ = engine;
    
    // Initialize Python engine
#ifdef NEXUS_PYTHON_ENABLED
    pythonEngine_ = std::make_unique<ScriptingEngine>();
    if (!pythonEngine_->Initialize(engine)) {
        Logger::Warning("Failed to initialize Python scripting engine");
        pythonEngine_.reset();
    }
#endif
    
    // Initialize Lua engine
#ifdef NEXUS_LUA_ENABLED
    luaEngine_ = std::make_unique<LuaScriptingEngine>();
    if (!luaEngine_->Initialize(engine)) {
        Logger::Warning("Failed to initialize Lua scripting engine");
        luaEngine_.reset();
    }
#endif
    
    if (!pythonEngine_ && !luaEngine_) {
        Logger::Error("No scripting engines available");
        return false;
    }
    
    // Initialize templates
    InitializeTemplates();
    
    initialized_ = true;
    Logger::Info("Script manager initialized");
    return true;
}

void ScriptManager::Shutdown() {
    if (!initialized_) return;
    
    if (pythonEngine_) {
        pythonEngine_->Shutdown();
        pythonEngine_.reset();
    }
    
    if (luaEngine_) {
        luaEngine_->Shutdown();
        luaEngine_.reset();
    }
    
    initialized_ = false;
    Logger::Info("Script manager shutdown");
}

bool ScriptManager::ExecuteFile(const std::string& filename, ScriptLanguage language) {
    if (!initialized_) {
        Logger::Error("Script manager not initialized");
        return false;
    }
    
    switch (language) {
        case ScriptLanguage::Python:
            if (pythonEngine_) {
                return pythonEngine_->ExecuteFile(filename);
            }
            Logger::Error("Python engine not available");
            return false;
            
        case ScriptLanguage::Lua:
            if (luaEngine_) {
                return luaEngine_->ExecuteFile(filename);
            }
            Logger::Error("Lua engine not available");
            return false;
            
        default:
            Logger::Error("Unsupported script language");
            return false;
    }
}

bool ScriptManager::ExecuteString(const std::string& code, ScriptLanguage language) {
    if (!initialized_) {
        Logger::Error("Script manager not initialized");
        return false;
    }
    
    switch (language) {
        case ScriptLanguage::Python:
            if (pythonEngine_) {
                return pythonEngine_->ExecuteString(code);
            }
            Logger::Error("Python engine not available");
            return false;
            
        case ScriptLanguage::Lua:
            if (luaEngine_) {
                return luaEngine_->ExecuteString(code);
            }
            Logger::Error("Lua engine not available");
            return false;
            
        default:
            Logger::Error("Unsupported script language");
            return false;
    }
}

bool ScriptManager::ExecuteFile(const std::string& filename) {
    ScriptLanguage language = DetectLanguage(filename);
    return ExecuteFile(filename, language);
}

void ScriptManager::EnableHotReload(bool enable) {
    if (pythonEngine_) {
        pythonEngine_->EnableHotReload(enable);
    }
    if (luaEngine_) {
        luaEngine_->EnableHotReload(enable);
    }
}

void ScriptManager::CheckForChanges() {
    if (pythonEngine_) {
        pythonEngine_->CheckForChanges();
    }
    if (luaEngine_) {
        luaEngine_->CheckForChanges();
    }
}

void ScriptManager::ReloadModifiedScripts() {
    if (pythonEngine_) {
        pythonEngine_->ReloadModifiedScripts();
    }
    if (luaEngine_) {
        luaEngine_->ReloadModifiedScripts();
    }
}

void ScriptManager::RegisterEventCallback(const std::string& eventName, std::function<void()> callback) {
    eventCallbacks_[eventName] = callback;
    
    // Register with all engines
    if (pythonEngine_) {
        pythonEngine_->RegisterEventCallback(eventName, callback);
    }
    if (luaEngine_) {
        luaEngine_->RegisterEventCallback(eventName, callback);
    }
}

void ScriptManager::TriggerEvent(const std::string& eventName) {
    auto it = eventCallbacks_.find(eventName);
    if (it != eventCallbacks_.end()) {
        it->second();
    }
    
    // Trigger in all engines
    if (pythonEngine_) {
        pythonEngine_->TriggerEvent(eventName);
    }
    if (luaEngine_) {
        luaEngine_->TriggerEvent(eventName);
    }
}

void ScriptManager::Update(float deltaTime) {
    if (pythonEngine_) {
        pythonEngine_->Update(deltaTime);
    }
    if (luaEngine_) {
        luaEngine_->Update(deltaTime);
    }
}

bool ScriptManager::LoadGameModule(const std::string& modulePath) {
    ScriptLanguage language = DetectLanguage(modulePath);
    
    if (ExecuteFile(modulePath, language)) {
        loadedModules_.push_back(modulePath);
        Logger::Info("Loaded game module: " + modulePath);
        return true;
    }
    
    return false;
}

void ScriptManager::UnloadGameModule(const std::string& moduleName) {
    auto it = std::find(loadedModules_.begin(), loadedModules_.end(), moduleName);
    if (it != loadedModules_.end()) {
        loadedModules_.erase(it);
        Logger::Info("Unloaded game module: " + moduleName);
    }
}

std::vector<std::string> ScriptManager::GetLoadedModules() const {
    return loadedModules_;
}

bool ScriptManager::CreateGameTemplate(const std::string& templateName, ScriptLanguage language) {
    std::string filename;
    std::string content;
    
    switch (language) {
        case ScriptLanguage::Python:
            filename = "games/python/" + templateName + ".py";
            content = templates_["python_game"];
            break;
            
        case ScriptLanguage::Lua:
            filename = "games/lua/" + templateName + ".lua";
            content = templates_["lua_game"];
            break;
            
        default:
            Logger::Error("Unsupported language for template creation");
            return false;
    }
    
    std::ofstream file(filename);
    if (!file) {
        Logger::Error("Failed to create template file: " + filename);
        return false;
    }
    
    file << content;
    file.close();
    
    Logger::Info("Created game template: " + filename);
    return true;
}

std::vector<std::string> ScriptManager::GetAvailableTemplates() const {
    std::vector<std::string> templateNames;
    for (const auto& [name, content] : templates_) {
        templateNames.push_back(name);
    }
    return templateNames;
}

ScriptManager::ScriptLanguage ScriptManager::DetectLanguage(const std::string& filename) {
    std::filesystem::path path(filename);
    std::string extension = path.extension().string();
    
    if (extension == ".py") {
        return ScriptLanguage::Python;
    } else if (extension == ".lua") {
        return ScriptLanguage::Lua;
    } else if (extension == ".js") {
        return ScriptLanguage::JavaScript;
    } else if (extension == ".cs") {
        return ScriptLanguage::CSharp;
    }
    
    // Default to Python
    return ScriptLanguage::Python;
}

void ScriptManager::InitializeTemplates() {
    // Python game template
    templates_["python_game"] = R"(
import nexus_engine as engine
import time

class Game:
    def __init__(self):
        self.player_x = 0.0
        self.player_y = 0.0
        self.player_speed = 100.0
        
    def initialize(self):
        print("Game initialized!")
        return True
        
    def update(self, delta_time):
        # Handle input
        if engine.is_key_pressed(ord('W')):
            self.player_y += self.player_speed * delta_time
        if engine.is_key_pressed(ord('S')):
            self.player_y -= self.player_speed * delta_time
        if engine.is_key_pressed(ord('A')):
            self.player_x -= self.player_speed * delta_time
        if engine.is_key_pressed(ord('D')):
            self.player_x += self.player_speed * delta_time
            
    def render(self):
        # Clear screen
        engine.clear_screen(0.2, 0.3, 0.8, 1.0)
        
        # Render game objects here
        
        # Present frame
        engine.present_frame()
        
    def shutdown(self):
        print("Game shutdown!")

# Create and run game
if __name__ == "__main__":
    game = Game()
    if game.initialize():
        print("Game running! Press ESC to quit.")
        
        while engine.is_running():
            delta_time = engine.get_delta_time()
            game.update(delta_time)
            game.render()
            
            if engine.is_key_pressed(27):  # ESC key
                engine.request_exit()
                
        game.shutdown()
)";

    // Lua game template
    templates_["lua_game"] = R"(
-- Game state
local game = {
    player_x = 0.0,
    player_y = 0.0,
    player_speed = 100.0
}

function initialize()
    log_info("Lua game initialized!")
    return true
end

function update(delta_time)
    -- Handle input
    if is_key_pressed(string.byte('W')) then
        game.player_y = game.player_y + game.player_speed * delta_time
    end
    if is_key_pressed(string.byte('S')) then
        game.player_y = game.player_y - game.player_speed * delta_time
    end
    if is_key_pressed(string.byte('A')) then
        game.player_x = game.player_x - game.player_speed * delta_time
    end
    if is_key_pressed(string.byte('D')) then
        game.player_x = game.player_x + game.player_speed * delta_time
    end
end

function render()
    -- Clear screen
    clear_screen(0.2, 0.3, 0.8, 1.0)
    
    -- Render game objects here
    
    -- Present frame
    present_frame()
end

function shutdown()
    log_info("Lua game shutdown!")
end

-- Game loop (called automatically by engine)
log_info("Lua game script loaded!")
)";

    // C++ game template
    templates_["cpp_game"] = R"(
#include "GameModuleAPI.h"
#include "Engine.h"
#include "Logger.h"

class MyGame : public Nexus::GameModule {
private:
    float playerX_ = 0.0f;
    float playerY_ = 0.0f;
    float playerSpeed_ = 100.0f;
    
public:
    bool Initialize(Nexus::Engine* engine) override {
        engine_ = engine;
        Nexus::Logger::Info("C++ game initialized!");
        return true;
    }
    
    void Update(float deltaTime) override {
        // Handle input
        if (nexus_is_key_pressed('W')) {
            playerY_ += playerSpeed_ * deltaTime;
        }
        if (nexus_is_key_pressed('S')) {
            playerY_ -= playerSpeed_ * deltaTime;
        }
        if (nexus_is_key_pressed('A')) {
            playerX_ -= playerSpeed_ * deltaTime;
        }
        if (nexus_is_key_pressed('D')) {
            playerX_ += playerSpeed_ * deltaTime;
        }
    }
    
    void Render() override {
        // Clear screen
        nexus_clear_screen(0.2f, 0.3f, 0.8f, 1.0f);
        
        // Render game objects here
        
        // Present frame
        nexus_present_frame();
    }
    
    void Shutdown() override {
        Nexus::Logger::Info("C++ game shutdown!");
    }
    
    void OnKeyPressed(int keyCode) override {
        if (keyCode == 27) { // ESC
            engine_->RequestExit();
        }
    }
};

// Register the module
REGISTER_GAME_MODULE(MyGame, "MyGame");
)";
}

} // namespace Nexus