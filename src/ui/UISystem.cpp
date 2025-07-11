#include "UISystem.h"
#include "Engine.h"
#include "Logger.h"

// ImGui includes - these need to be properly installed
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include <d3d11.h>

namespace Nexus {

UISystem::UISystem()
    : initialized_(false)
    , isVisible_(true)
    , engine_(nullptr)
    , device_(nullptr)
    , context_(nullptr)
    , hwnd_(nullptr)
    , imguiContext_(nullptr)
{
}

UISystem::~UISystem() {
    Shutdown();
}

bool UISystem::Initialize(Engine* engine, ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd) {
    if (initialized_) return true;
    
    engine_ = engine;
    device_ = device;
    context_ = context;
    hwnd_ = hwnd;
    
    Logger::Info("Initializing UI System...");
    
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imguiContext_ = ImGui::GetCurrentContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    if (!ImGui_ImplWin32_Init(hwnd_)) {
        Logger::Error("Failed to initialize ImGui Win32 backend");
        return false;
    }
    
    if (!ImGui_ImplDX11_Init(device_, context_)) {
        Logger::Error("Failed to initialize ImGui DirectX 11 backend");
        return false;
    }
    
    initialized_ = true;
    Logger::Info("UI System initialized successfully");
    return true;
}

void UISystem::Shutdown() {
    if (!initialized_) return;
    
    Logger::Info("Shutting down UI System...");
    
    if (imguiContext_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiContext_ = nullptr;
    }
    
    initialized_ = false;
    Logger::Info("UI System shutdown complete");
}

void UISystem::NewFrame() {
    if (!initialized_ || !isVisible_) return;
    
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UISystem::Render() {
    if (!initialized_ || !isVisible_) return;
    
    // Simple demo window
    if (ImGui::Begin("Nexus Engine UI")) {
        ImGui::Text("Engine Status: Running");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        if (ImGui::Button("Exit")) {
            if (engine_) {
                engine_->RequestExit();
            }
        }
    }
    ImGui::End();
}

void UISystem::EndFrame() {
    if (!initialized_ || !isVisible_) return;
    
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void UISystem::HandleKeyPress(int key) {
    // Handle key presses
    if (key == VK_F1) {
        ToggleVisibility();
    }
}

void UISystem::HandleMouseMove(int x, int y) {
    // Handle mouse movement
}

void UISystem::HandleMouseClick(int button, int x, int y) {
    // Handle mouse clicks
}

} // namespace Nexus