#include "Engine.h"
#include "GraphicsDevice.h"
#include "AudioDevice.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "Timer.h"
#include "Logger.h"
#include "ScriptingEngine.h"
#include "ResourceManager.h"
#include "PhysicsEngine.h"
#include "AISystem.h"
#include "LightingEngine.h"
#include "AnimationSystem.h"
#include "ParticleSystem.h"
#include "MotionControlSystem.h"
#include "EngineUI.h"
#include "EngineErrorRecovery.h"
#include <windowsx.h>
#include <chrono>
#include <stdexcept>
#include <sstream>

namespace Nexus {

// Static instance for window procedure
static Engine* g_engineInstance = nullptr;

Engine::Engine()
    : initialized_(false)
    , isRunning_(false)
    , shouldExit_(false)
    , recoveringFromError_(false)
    , deltaTime_(0.0f)
    , hwnd_(nullptr)
    , windowClass_("NexusEngineWindow")
    , frameCount_(0)
    , timeAccumulator_(0.0f)
{
    g_engineInstance = this;
    
    // Initialize performance stats
    perfStats_ = {};
}

Engine::~Engine() {
    SafeShutdown();
    g_engineInstance = nullptr;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_engineInstance && g_engineInstance->GetUI()) {
        // Handle UI events first
        extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return true;
    }
    
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE:
            if (g_engineInstance && g_engineInstance->GetGraphics()) {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                // Handle resize
            }
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                if (g_engineInstance) {
                    g_engineInstance->RequestExit();
                }
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool Engine::Initialize(const std::string& configFile) {
    if (initialized_) return true;
    
    Logger::Info("Initializing Nexus Engine...");
    
    try {
        // Initialize platform
        if (!Platform::Initialize()) {
            Logger::Error("Failed to initialize platform");
            return false;
        }

        // Create window
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "NexusEngine";

        if (!RegisterClassExA(&wc)) {
            Logger::Error("Failed to register window class");
            return false;
        }

        hwnd_ = CreateWindowExA(
            0,
            "NexusEngine",
            "Nexus Game Engine",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width_, height_,
            nullptr, nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (!hwnd_) {
            Logger::Error("Failed to create window");
            return false;
        }

        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);

        // Initialize graphics first
        if (!graphics_->Initialize(hwnd_, width_, height_, fullscreen_)) {
            Logger::Error("Failed to initialize graphics device");
            return false;
        }

        // Initialize input
        if (!input_->Initialize(hwnd_)) {
            Logger::Error("Failed to initialize input manager");
            return false;
        }

        // Initialize resource manager with graphics device
        if (!resources_->Initialize(graphics_->GetDevice())) {
            Logger::Error("Failed to initialize resource manager");
            return false;
        }

        // Initialize audio
        if (!audio_->Initialize()) {
            Logger::Error("Failed to initialize audio device");
            return false;
        }

        // Fix: Use correct AudioSystem::Initialize signature (takes int parameters, not AudioDevice*)
        if (!audioSystem_->Initialize(44100, 2, 16, AudioSystem::AudioChannelLayout::Stereo)) {
            Logger::Error("Failed to initialize audio system");
            return false;
        }

#ifdef NEXUS_PYTHON_ENABLED
        // Initialize scripting
        if (!scripting_->Initialize()) {
            Logger::Warning("Python scripting not available");
        }
#endif

        // Initialize physics
        if (!physics_->Initialize()) {
            Logger::Error("Failed to initialize physics engine");
            return false;
        }

        // Initialize AI system
        if (!ai_->Initialize()) {
            Logger::Error("Failed to initialize AI system");
            return false;
        }

        // Initialize lighting engine
        if (!lighting_->Initialize(graphics_->GetDevice(), graphics_->GetContext(), width_, height_)) {
            Logger::Error("Failed to initialize lighting engine");
            return false;
        }

        // Fix: AnimationSystem::Initialize takes device and context parameters
        if (!animation_->Initialize(graphics_->GetDevice(), graphics_->GetContext())) {
            Logger::Error("Failed to initialize animation system");
            return false;
        }

        // Initialize particle system
        if (!particles_->Initialize(graphics_->GetDevice(), graphics_->GetContext())) {
            Logger::Error("Failed to initialize particle system");
            return false;
        }

        // Fix: MotionControlSystem::Initialize takes no parameters
        if (!motionControl_->Initialize()) {
            Logger::Warning("Motion control initialization failed - continuing without motion control");
        }

        // Create TextRenderer
        textRenderer_ = std::make_unique<TextRenderer>();
        if (!textRenderer_->Initialize(graphics_->GetDevice(), graphics_->GetContext())) {
            Logger::Warning("Failed to initialize text renderer");
        }

        // Create physics demo
        physics_->CreatePhysicsDemo();

        Logger::Info("Engine initialized successfully");
        initialized_ = true;
        return true;

    } catch (const std::exception& e) {
        Logger::Error("Exception during engine initialization: " + std::string(e.what()));
        return false;
    }
}

void Engine::Run() {
    Logger::Info("Starting main engine loop...");
    
    isRunning_ = true;
    Timer timer;
    
    try {
        while (isRunning_) {
            timer.Reset();
            
            MSG msg = {};
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                
                if (msg.message == WM_QUIT) {
                    isRunning_ = false;
                    break;
                }
            }

            // Check for ESC key to exit
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                isRunning_ = false;
                break;
            }

            // Update
            deltaTime_ = timer.GetElapsedTime();
            try {
                Update(deltaTime_);
            } catch (const std::exception& e) {
                Logger::Error("Exception during update: " + std::string(e.what()));
                isRunning_ = false;
                break;
            }

            // Render frame
            try {
                Render();
            } catch (const std::exception& e) {
                Logger::Error("Exception during render: " + std::string(e.what()));
                isRunning_ = false;
                break;
            }

            // Cap frame rate
            float frameTime = timer.GetElapsedTime();
            if (targetFPS_ > 0) {
                float targetFrameTime = 1.0f / targetFPS_;
                if (frameTime < targetFrameTime) {
                    Sleep(static_cast<DWORD>((targetFrameTime - frameTime) * 1000.0f));
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::Error("Exception in main loop: " + std::string(e.what()));
    }
    
    Logger::Info("Main loop ended");
}

void Engine::Update(float deltaTime) {
    // Update input first
    if (input_) {
        input_->Update();
    }
    
    // Update physics
    if (physics_) {
        physics_->Update(deltaTime);
    }
    
    // Update AI
    if (ai_) {
        ai_->Update(deltaTime);
    }
    
    // Update audio
    if (audioSystem_) {
        audioSystem_->Update(deltaTime);
    }
    
    // Update animation
    if (animation_) {
        animation_->Update(deltaTime);
    }
    
    // Update particles
    if (particles_) {
        particles_->Update(deltaTime);
    }
    
    // Update motion control
    if (motionControl_) {
        motionControl_->Update(deltaTime);
    }
    
#ifdef NEXUS_PYTHON_ENABLED
    // Update scripting
    if (scripting_) {
        scripting_->Update(deltaTime);
    }
#endif
    
    // Update UI
    if (ui_) {
        ui_->Update(deltaTime);
    }
    
    // Update error recovery
    if (errorRecovery_) {
        errorRecovery_->Update(deltaTime);
    }
}

void Engine::Render() {
    if (!graphics_) return;
    
    graphics_->BeginFrame();
    
    // Render physics objects
    if (physics_) {
        const auto& renderObjects = physics_->GetRenderObjects();
        if (!renderObjects.empty()) {
            static bool firstRender = true;
            if (firstRender) {
                Logger::Info("Rendering " + std::to_string(renderObjects.size()) + " physics objects");
                firstRender = false;
            }
            for (const auto& obj : renderObjects) {
                switch (obj.shapeType) {
                    case CollisionShape::Type::Box:
                        graphics_->RenderBox(obj.position, obj.scale, obj.color);
                        break;
                    case CollisionShape::Type::Sphere:
                        graphics_->RenderSphere(obj.position, obj.scale.x, obj.color);
                        break;
                    case CollisionShape::Type::Capsule:
                        graphics_->RenderCapsule(obj.position, obj.scale.x, obj.scale.y, obj.color);
                        break;
                }
            }
        }
        
        // Render UI text (basic status information)
        if (textRenderer_) {
            using namespace DirectX;
            textRenderer_->RenderText("Nexus Engine v1.0", 10.0f, 10.0f, 1.0f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
            textRenderer_->RenderText("FPS: " + std::to_string(GetFPS()), 10.0f, 30.0f, 1.0f, XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
            textRenderer_->RenderText("Objects: " + std::to_string(physics_ ? physics_->GetRenderObjects().size() : 0), 10.0f, 50.0f, 1.0f, XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
        }
    }
    
    // Render UI
    if (ui_) {
        ui_->Render();
    }
    
    graphics_->EndFrame();
    graphics_->Present();
}

int Engine::GetFPS() {
    static auto lastUpdate = std::chrono::high_resolution_clock::now();
    static int frameCount = 0;
    static int currentFPS = 0;
    
    frameCount++;
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<float>(now - lastUpdate).count();
    
    if (elapsed >= 1.0f) {
        currentFPS = frameCount;
        frameCount = 0;
        lastUpdate = now;
    }
    
    return currentFPS;
}

void Engine::Shutdown() {
    Logger::Info("Shutting down engine...");
    
    isRunning_ = false;
    
    // Shutdown subsystems in reverse order
    textRenderer_.reset();
    motionControl_.reset();
    particles_.reset();
    animation_.reset();
    lighting_.reset();
    ai_.reset();
    physics_.reset();
    
#ifdef NEXUS_PYTHON_ENABLED
    scripting_.reset();
#endif
    
    resources_.reset();
    input_.reset();
    audioSystem_.reset();
    audio_.reset();
    graphics_.reset();
    
    // Cleanup window
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    
    UnregisterClassA("NexusEngine", GetModuleHandle(nullptr));
    Platform::Shutdown();
    
    Logger::Info("Engine shutdown complete");
}

} // namespace Nexus
