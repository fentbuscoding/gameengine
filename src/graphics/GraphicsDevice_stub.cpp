// Stub implementation for GraphicsDevice when DirectX is not available
#include "GraphicsDevice.h"
#include "Logger.h"

#ifdef NEXUS_USE_DIRECTX11

namespace Nexus {

void GraphicsDevice::Shutdown() {
    if (backBufferRTV_) {
        backBufferRTV_->Release();
        backBufferRTV_ = nullptr;
    }
    
    if (swapChain_) {
        swapChain_->Release();
        swapChain_ = nullptr;
    }
    
    if (context_) {
        context_->Release();
        context_ = nullptr;
    }
    
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
    
    Logger::Info("DirectX 11 graphics device shutdown");
}

void GraphicsDevice::BeginFrame() {
    if (!context_) return;
    
    // Clear the back buffer
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context_->ClearRenderTargetView(renderTargetView_, clearColor);
}

void GraphicsDevice::EndFrame() {
    // Nothing to do here for DX11
}

void GraphicsDevice::Present() {
    if (swapChain_) {
        swapChain_->Present(0, 0);
    }
}

void GraphicsDevice::Clear(const XMFLOAT4& color) {
    if (!context_) return;
    
    float clearColor[4] = { color.x, color.y, color.z, color.w };
    context_->ClearRenderTargetView(backBufferRTV_, clearColor);
}

void GraphicsDevice::SetViewport(int x, int y, int width, int height) {
    if (!context_) return;
    
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = (float)x;
    viewport.TopLeftY = (float)y;
    viewport.Width = (float)width;
    viewport.Height = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
}

// Stub implementations for other methods
void GraphicsDevice::RenderMesh(const Mesh& mesh, const Shader& shader) {
    Logger::Info("RenderMesh called (stub implementation)");
}

void GraphicsDevice::SetBloomEnabled(bool enabled) {
    bloomEnabled_ = enabled;
    Logger::Info("Bloom enabled: " + std::to_string(enabled));
}

void GraphicsDevice::SetHeatHazeEnabled(bool enabled) {
    heatHazeEnabled_ = enabled;
    Logger::Info("Heat haze enabled: " + std::to_string(enabled));
}

void GraphicsDevice::SetShadowsEnabled(bool enabled) {
    shadowsEnabled_ = enabled;
    Logger::Info("Shadows enabled: " + std::to_string(enabled));
}

bool GraphicsDevice::ResetDevice() {
    Logger::Info("ResetDevice called (stub implementation)");
    return true;
}

void GraphicsDevice::InitializePostProcessing() {
    Logger::Info("InitializePostProcessing called (stub implementation)");
}

void GraphicsDevice::InitializeShadowMapping() {
    Logger::Info("InitializeShadowMapping called (stub implementation)");
}

void GraphicsDevice::RenderBloomPass() {
    Logger::Info("RenderBloomPass called (stub implementation)");
}

void GraphicsDevice::RenderHeatHazePass() {
    Logger::Info("RenderHeatHazePass called (stub implementation)");
}

void GraphicsDevice::CreateDefaultShaders() {
    Logger::Info("CreateDefaultShaders called (stub implementation)");
}

} // namespace Nexus

#endif // NEXUS_USE_DIRECTX11
