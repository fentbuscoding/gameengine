#pragma once

#include "Platform.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace Nexus {

class Engine;

/**
 * C API for game modules (can be used from C, C++, or other languages via FFI)
 */
extern "C" {
    // Engine access
    void* nexus_get_engine();
    void* nexus_get_graphics();
    void* nexus_get_input();
    void* nexus_get_audio();
    void* nexus_get_physics();
    
    // Basic operations
    void nexus_log_info(const char* message);
    void nexus_log_warning(const char* message);
    void nexus_log_error(const char* message);
    
    // Input
    bool nexus_is_key_pressed(int keyCode);
    bool nexus_is_mouse_button_pressed(int button);
    void nexus_get_mouse_position(float* x, float* y);
    
    // Graphics
    void nexus_clear_screen(float r, float g, float b, float a);
    void nexus_present_frame();
    void nexus_set_viewport(int x, int y, int width, int height);
    
    // Time
    float nexus_get_delta_time();
    int nexus_get_fps();
    double nexus_get_time();
    
    // Game object management
    int nexus_create_game_object();
    void nexus_destroy_game_object(int id);
    void nexus_set_position(int id, float x, float y, float z);
    void nexus_get_position(int id, float* x, float* y, float* z);
    
    // Physics
    int nexus_create_physics_body(int gameObjectId, float mass);
    void nexus_apply_force(int bodyId, float x, float y, float z);
    void nexus_set_velocity(int bodyId, float x, float y, float z);
    
    // Audio
    int nexus_load_sound(const char* filename);
    void nexus_play_sound(int soundId);
    void nexus_stop_sound(int soundId);
    void nexus_set_volume(int soundId, float volume);
}

/**
 * C++ Game Module base class
 */
class GameModule {
public:
    virtual ~GameModule() = default;
    
    // Lifecycle
    virtual bool Initialize(Engine* engine) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;
    
    // Events
    virtual void OnKeyPressed(int keyCode) {}
    virtual void OnKeyReleased(int keyCode) {}
    virtual void OnMousePressed(int button, float x, float y) {}
    virtual void OnMouseReleased(int button, float x, float y) {}
    virtual void OnMouseMoved(float x, float y) {}
    
protected:
    Engine* engine_ = nullptr;
};

/**
 * Game Module Factory for dynamic loading
 */
class GameModuleFactory {
public:
    using CreateModuleFunc = std::function<std::unique_ptr<GameModule>()>;
    
    static void RegisterModule(const std::string& name, CreateModuleFunc factory);
    static std::unique_ptr<GameModule> CreateModule(const std::string& name);
    static std::vector<std::string> GetRegisteredModules();
    
private:
    static std::map<std::string, CreateModuleFunc> factories_;
};

// Macro for easy module registration
#define REGISTER_GAME_MODULE(className, moduleName) \
    namespace { \
        bool _registered_##className = []() { \
            GameModuleFactory::RegisterModule(moduleName, []() { \
                return std::make_unique<className>(); \
            }); \
            return true; \
        }(); \
    }

} // namespace Nexus