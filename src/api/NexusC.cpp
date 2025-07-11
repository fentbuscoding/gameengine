#include "NexusC.h"
#include "Engine.h"
#include "GraphicsDevice.h"
#include "InputManager.h"
#include "AudioDevice.h"
#include "PhysicsEngine.h"
#include "Logger.h"
#include <DirectXMath.h>
#include <memory>
#include <map>

using namespace Nexus;
using namespace DirectX;

// Internal storage for C callbacks
struct NexusCallbacks {
    NexusUpdateCallback updateCallback = nullptr;
    void* updateUserData = nullptr;
    NexusRenderCallback renderCallback = nullptr;
    void* renderUserData = nullptr;
    NexusInputCallback inputCallback = nullptr;
    void* inputUserData = nullptr;
};

static std::map<NexusEngine*, std::unique_ptr<NexusCallbacks>> s_callbacks;

// Helper functions
static XMFLOAT3 ToXMFloat3(NexusVector3 v) {
    return XMFLOAT3(v.x, v.y, v.z);
}

static NexusVector3 FromXMFloat3(const XMFLOAT3& v) {
    return {v.x, v.y, v.z};
}

static XMFLOAT4 ToXMFloat4(NexusColor c) {
    return XMFLOAT4(c.r, c.g, c.b, c.a);
}

// Engine management
extern "C" {

NexusEngine* nexus_engine_create(void) {
    try {
        auto* engine = new Engine();
        s_callbacks[reinterpret_cast<NexusEngine*>(engine)] = std::make_unique<NexusCallbacks>();
        return reinterpret_cast<NexusEngine*>(engine);
    } catch (...) {
        return nullptr;
    }
}

bool nexus_engine_initialize(NexusEngine* engine) {
    if (!engine) return false;
    try {
        return reinterpret_cast<Engine*>(engine)->Initialize();
    } catch (...) {
        return false;
    }
}

void nexus_engine_run(NexusEngine* engine) {
    if (!engine) return;
    try {
        auto* e = reinterpret_cast<Engine*>(engine);
        auto* callbacks = s_callbacks[engine].get();
        
        // Custom run loop that calls C callbacks
        while (e->IsRunning()) {
            float deltaTime = e->GetDeltaTime();
            
            // Update callback
            if (callbacks && callbacks->updateCallback) {
                callbacks->updateCallback(deltaTime, callbacks->updateUserData);
            }
            
            // Input callback  
            if (callbacks && callbacks->inputCallback) {
                callbacks->inputCallback(reinterpret_cast<NexusInput*>(e->GetInput()), 
                                       callbacks->inputUserData);
            }
            
            // Render callback
            if (callbacks && callbacks->renderCallback) {
                auto* graphics = e->GetGraphics();
                graphics->BeginFrame();
                callbacks->renderCallback(reinterpret_cast<NexusGraphics*>(graphics), 
                                        callbacks->renderUserData);
                graphics->EndFrame();
                graphics->Present();
            }
            
            // Check for ESC key to exit
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                e->RequestExit();
            }
            
            Sleep(16); // ~60 FPS
        }
    } catch (...) {
        // Handle errors
    }
}

void nexus_engine_shutdown(NexusEngine* engine) {
    if (!engine) return;
    try {
        reinterpret_cast<Engine*>(engine)->Shutdown();
    } catch (...) {
        // Handle errors
    }
}

void nexus_engine_destroy(NexusEngine* engine) {
    if (!engine) return;
    try {
        s_callbacks.erase(engine);
        delete reinterpret_cast<Engine*>(engine);
    } catch (...) {
        // Handle errors
    }
}

bool nexus_engine_is_running(NexusEngine* engine) {
    if (!engine) return false;
    try {
        return reinterpret_cast<Engine*>(engine)->IsRunning();
    } catch (...) {
        return false;
    }
}

void nexus_engine_request_exit(NexusEngine* engine) {
    if (!engine) return;
    try {
        reinterpret_cast<Engine*>(engine)->RequestExit();
    } catch (...) {
        // Handle errors
    }
}

float nexus_engine_get_delta_time(NexusEngine* engine) {
    if (!engine) return 0.0f;
    try {
        return reinterpret_cast<Engine*>(engine)->GetDeltaTime();
    } catch (...) {
        return 0.0f;
    }
}

int nexus_engine_get_fps(NexusEngine* engine) {
    if (!engine) return 0;
    try {
        return reinterpret_cast<Engine*>(engine)->GetFPS();
    } catch (...) {
        return 0;
    }
}

void nexus_engine_set_target_fps(NexusEngine* engine, float fps) {
    if (!engine) return;
    try {
        reinterpret_cast<Engine*>(engine)->SetTargetFPS(fps);
    } catch (...) {
        // Handle errors
    }
}

// Graphics API
NexusGraphics* nexus_engine_get_graphics(NexusEngine* engine) {
    if (!engine) return nullptr;
    try {
        return reinterpret_cast<NexusGraphics*>(reinterpret_cast<Engine*>(engine)->GetGraphics());
    } catch (...) {
        return nullptr;
    }
}

void nexus_graphics_begin_frame(NexusGraphics* graphics) {
    if (!graphics) return;
    try {
        reinterpret_cast<GraphicsDevice*>(graphics)->BeginFrame();
    } catch (...) {
        // Handle errors
    }
}

void nexus_graphics_end_frame(NexusGraphics* graphics) {
    if (!graphics) return;
    try {
        reinterpret_cast<GraphicsDevice*>(graphics)->EndFrame();
    } catch (...) {
        // Handle errors
    }
}

void nexus_graphics_present(NexusGraphics* graphics) {
    if (!graphics) return;
    try {
        reinterpret_cast<GraphicsDevice*>(graphics)->Present();
    } catch (...) {
        // Handle errors
    }
}

void nexus_graphics_clear(NexusGraphics* graphics, NexusColor color) {
    if (!graphics) return;
    try {
        reinterpret_cast<GraphicsDevice*>(graphics)->Clear(ToXMFloat4(color));
    } catch (...) {
        // Handle errors
    }
}

// Callbacks
void nexus_engine_set_update_callback(NexusEngine* engine, NexusUpdateCallback callback, void* userData) {
    if (!engine) return;
    auto* callbacks = s_callbacks[engine].get();
    if (callbacks) {
        callbacks->updateCallback = callback;
        callbacks->updateUserData = userData;
    }
}

void nexus_engine_set_render_callback(NexusEngine* engine, NexusRenderCallback callback, void* userData) {
    if (!engine) return;
    auto* callbacks = s_callbacks[engine].get();
    if (callbacks) {
        callbacks->renderCallback = callback;
        callbacks->renderUserData = userData;
    }
}

void nexus_engine_set_input_callback(NexusEngine* engine, NexusInputCallback callback, void* userData) {
    if (!engine) return;
    auto* callbacks = s_callbacks[engine].get();
    if (callbacks) {
        callbacks->inputCallback = callback;
        callbacks->inputUserData = userData;
    }
}

// Math utilities
NexusVector3 nexus_vector3_add(NexusVector3 a, NexusVector3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

NexusVector3 nexus_vector3_subtract(NexusVector3 a, NexusVector3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

NexusVector3 nexus_vector3_multiply(NexusVector3 v, float scalar) {
    return {v.x * scalar, v.y * scalar, v.z * scalar};
}

float nexus_vector3_dot(NexusVector3 a, NexusVector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

NexusVector3 nexus_vector3_cross(NexusVector3 a, NexusVector3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float nexus_vector3_length(NexusVector3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

NexusVector3 nexus_vector3_normalize(NexusVector3 v) {
    float length = nexus_vector3_length(v);
    if (length > 0.0f) {
        return nexus_vector3_multiply(v, 1.0f / length);
    }
    return {0, 0, 0};
}

} // extern "C"