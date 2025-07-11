#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <Windows.h>

#include "TextRenderer.h"

namespace Nexus {

class GraphicsDevice;
class AudioDevice;
class AudioSystem;
class InputManager;
class ScriptingEngine;
class ResourceManager;
class PhysicsEngine;
class AIManager;
class LightingEngine;
class AnimationSystem;
class ParticleSystem;
class MotionControlSystem;
class EngineUI;
class UISystem;
class EngineErrorRecovery;

struct InitParams {
    std::string configFile;
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    std::string title = "Nexus Engine";
};

/**
 * Main engine class that manages all subsystems
 */
class Engine {
public:
    Engine();
    ~Engine();

    // Core lifecycle
    bool Initialize(const std::string& configFile = "");
    void Run();
    void Shutdown();

    // Subsystem access
    GraphicsDevice* GetGraphics() const { return graphics_.get(); }
    AudioDevice* GetAudio() const { return audio_.get(); }
    AudioSystem* GetAudioSystem() const { return audioSystem_.get(); }
    InputManager* GetInput() const { return input_.get(); }
#ifdef NEXUS_PYTHON_ENABLED
    ScriptingEngine* GetScripting() const { return scripting_.get(); }
#else
    ScriptingEngine* GetScripting() const { return nullptr; }
#endif
    ResourceManager* GetResources() const { return resources_.get(); }
    PhysicsEngine* GetPhysics() const { return physics_.get(); }
    AIManager* GetAI() const { return ai_.get(); }
    LightingEngine* GetLighting() const { return lighting_.get(); }
    AnimationSystem* GetAnimation() const { return animation_.get(); }
    ParticleSystem* GetParticles() const { return particles_.get(); }
    MotionControlSystem* GetMotionControl() const { return motionControl_.get(); }
    EngineUI* GetUI() const { return ui_.get(); }
    EngineErrorRecovery* GetErrorRecovery() const { return errorRecovery_.get(); }

    // Frame control
    void SetTargetFPS(float fps) { targetFPS_ = fps; }
    int GetFPS(); // Remove const since method modifies member variables
    float GetDeltaTime() const { return deltaTime_; }

    // State
    bool IsRunning() const { return isRunning_; }
    void RequestExit() { isRunning_ = false; }

private:
    void Update(float deltaTime);
    void Render();
    void SafeShutdown();
    
    // Core subsystems
    std::unique_ptr<GraphicsDevice> graphics_;
    std::unique_ptr<AudioDevice> audio_;
    std::unique_ptr<AudioSystem> audioSystem_;
    std::unique_ptr<InputManager> input_;
#ifdef NEXUS_PYTHON_ENABLED
    std::unique_ptr<ScriptingEngine> scripting_;
#endif
    std::unique_ptr<ResourceManager> resources_;
    
    // Advanced subsystems
    std::unique_ptr<PhysicsEngine> physics_;
    std::unique_ptr<AIManager> ai_;
    std::unique_ptr<LightingEngine> lighting_;
    std::unique_ptr<AnimationSystem> animation_;
    std::unique_ptr<ParticleSystem> particles_;
    std::unique_ptr<MotionControlSystem> motionControl_;
    std::unique_ptr<TextRenderer> textRenderer_;
    std::unique_ptr<EngineUI> ui_;
    std::unique_ptr<EngineErrorRecovery> errorRecovery_;

    // Window and initialization
    HWND hwnd_;
    int width_;
    int height_;
    bool fullscreen_;
    std::string windowClass_;

    // Engine state
    bool initialized_;
    bool isRunning_;
    bool shouldExit_;
    bool recoveringFromError_;
    float targetFPS_;
    float deltaTime_;
    
    // Performance stats
    struct {
        float frameTime;
        float updateTime;
        float renderTime;
        int memoryUsage;
    } perfStats_;
    
    // FPS calculation
    std::chrono::high_resolution_clock::time_point lastFPSUpdate_;
    int frameCount_;
    float timeAccumulator_;
};

} // namespace Nexus
