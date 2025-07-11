#include "LightingEngine.h"
#include "Logger.h"
#include <cmath>

namespace Nexus {

LightingEngine::LightingEngine()
    : device_(nullptr), context_(nullptr), screenWidth_(0), screenHeight_(0),
      sceneTexture_(nullptr), sceneSurface_(nullptr), normalTexture_(nullptr), normalSurface_(nullptr),
      depthTexture_(nullptr), depthSurface_(nullptr), bloomTexture_(nullptr), bloomSurface_(nullptr),
      heatHazeTexture_(nullptr), heatHazeSurface_(nullptr), shadowTexture_(nullptr), shadowSurface_(nullptr),
      shadowDepthTexture_(nullptr), shadowDepthSurface_(nullptr) {
}

LightingEngine::~LightingEngine() {
    Shutdown();
}

bool LightingEngine::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int screenWidth, int screenHeight) {
    device_ = device;
    context_ = context;
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    
    if (!CreateRenderTargets()) {
        Logger::Error("Failed to create render targets");
        return false;
    }
    
    if (!CreateShadowMaps()) {
        Logger::Error("Failed to create shadow maps");
        return false;
    }
    
    if (!CreateGBuffer()) {
        Logger::Error("Failed to create G-Buffer");
        return false;
    }
    
    return true;
}

void LightingEngine::Shutdown() {
    // Release render targets
    if (sceneSurface_) {
        sceneSurface_->Release();
        sceneSurface_ = nullptr;
    }
    if (sceneTexture_) {
        sceneTexture_->Release();
        sceneTexture_ = nullptr;
    }
    
    if (normalSurface_) {
        normalSurface_->Release();
        normalSurface_ = nullptr;
    }
    if (normalTexture_) {
        normalTexture_->Release();
        normalTexture_ = nullptr;
    }
    
    if (depthSurface_) {
        depthSurface_->Release();
        depthSurface_ = nullptr;
    }
    if (depthTexture_) {
        depthTexture_->Release();
        depthTexture_ = nullptr;
    }
    
    if (bloomSurface_) {
        bloomSurface_->Release();
        bloomSurface_ = nullptr;
    }
    if (bloomTexture_) {
        bloomTexture_->Release();
        bloomTexture_ = nullptr;
    }
    
    if (heatHazeSurface_) {
        heatHazeSurface_->Release();
        heatHazeSurface_ = nullptr;
    }
    if (heatHazeTexture_) {
        heatHazeTexture_->Release();
        heatHazeTexture_ = nullptr;
    }
    
    if (shadowSurface_) {
        shadowSurface_->Release();
        shadowSurface_ = nullptr;
    }
    if (shadowTexture_) {
        shadowTexture_->Release();
        shadowTexture_ = nullptr;
    }
    
    if (shadowDepthSurface_) {
        shadowDepthSurface_->Release();
        shadowDepthSurface_ = nullptr;
    }
    if (shadowDepthTexture_) {
        shadowDepthTexture_->Release();
        shadowDepthTexture_ = nullptr;
    }
    
    // Release shadow maps
    for (auto& shadowMap : shadowMaps_) {
        DestroyShadowMap(shadowMap);
    }
    shadowMaps_.clear();
    
    // Release G-Buffer
    DestroyGBuffer();
}

bool LightingEngine::CreateRenderTargets() {
    HRESULT hr;
    
    // Create scene render target texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = screenWidth_;
    textureDesc.Height = screenHeight_;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &sceneTexture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create scene texture");
        return false;
    }
    
    // Create render target view for scene texture
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = textureDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    
    hr = device_->CreateRenderTargetView(sceneTexture_, &rtvDesc, &sceneSurface_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create scene render target view");
        return false;
    }
    
    // Create normal render target texture
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &normalTexture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create normal texture");
        return false;
    }
    
    hr = device_->CreateRenderTargetView(normalTexture_, &rtvDesc, &normalSurface_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create normal render target view");
        return false;
    }
    
    // Create depth render target texture
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &depthTexture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create depth texture");
        return false;
    }
    
    hr = device_->CreateRenderTargetView(depthTexture_, &rtvDesc, &depthSurface_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create depth render target view");
        return false;
    }
    
    // Create bloom render target (half resolution)
    textureDesc.Width = screenWidth_ / 2;
    textureDesc.Height = screenHeight_ / 2;
    
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &bloomTexture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create bloom texture");
        return false;
    }
    
    hr = device_->CreateRenderTargetView(bloomTexture_, &rtvDesc, &bloomSurface_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create bloom render target view");
        return false;
    }
    
    // Create heat haze render target (full resolution)
    textureDesc.Width = screenWidth_;
    textureDesc.Height = screenHeight_;
    
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &heatHazeTexture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create heat haze texture");
        return false;
    }
    
    hr = device_->CreateRenderTargetView(heatHazeTexture_, &rtvDesc, &heatHazeSurface_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create heat haze render target view");
        return false;
    }
    
    return true;
}

bool LightingEngine::CreateShadowMaps() {
    // Create shadow mapping resources
    const int shadowMapSize = 2048;
    
    // Create shadow map render target
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = shadowMapSize;
    textureDesc.Height = shadowMapSize;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    HRESULT hr = device_->CreateTexture2D(&textureDesc, nullptr, &shadowTexture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow texture");
        return false;
    }
    
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = textureDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    
    hr = device_->CreateRenderTargetView(shadowTexture_, &rtvDesc, &shadowSurface_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow render target view");
        return false;
    }
    
    // Create shadow map depth texture
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = shadowMapSize;
    depthDesc.Height = shadowMapSize;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;
    
    hr = device_->CreateTexture2D(&depthDesc, nullptr, &shadowDepthTexture_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow depth texture");
        return false;
    }
    
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    
    hr = device_->CreateDepthStencilView(shadowDepthTexture_, &dsvDesc, &shadowDepthSurface_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow depth stencil view");
        return false;
    }
    
    return true;
}

bool LightingEngine::CreateGBuffer() {
    // Create G-Buffer textures for deferred rendering
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = screenWidth_;
    textureDesc.Height = screenHeight_;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    HRESULT hr;
    
    // Create albedo texture (RGB = albedo, A = metallic)
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &gBuffer_.albedoTexture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer albedo texture");
        return false;
    }
    
    // Create normal texture (RGB = world space normal, A = roughness)
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &gBuffer_.normalTexture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer normal texture");
        return false;
    }
    
    // Create position texture (RGB = world position, A = depth)
    textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &gBuffer_.positionTexture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer position texture");
        return false;
    }
    
    // Create render target views
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = device_->CreateRenderTargetView(gBuffer_.albedoTexture, &rtvDesc, &gBuffer_.albedoRTV);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer albedo RTV");
        return false;
    }
    
    hr = device_->CreateRenderTargetView(gBuffer_.normalTexture, &rtvDesc, &gBuffer_.normalRTV);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer normal RTV");
        return false;
    }
    
    rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = device_->CreateRenderTargetView(gBuffer_.positionTexture, &rtvDesc, &gBuffer_.positionRTV);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer position RTV");
        return false;
    }
    
    // Create shader resource views
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = device_->CreateShaderResourceView(gBuffer_.albedoTexture, &srvDesc, &gBuffer_.albedoSRV);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer albedo SRV");
        return false;
    }
    
    hr = device_->CreateShaderResourceView(gBuffer_.normalTexture, &srvDesc, &gBuffer_.normalSRV);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer normal SRV");
        return false;
    }
    
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hr = device_->CreateShaderResourceView(gBuffer_.positionTexture, &srvDesc, &gBuffer_.positionSRV);
    if (FAILED(hr)) {
        Logger::Error("Failed to create G-Buffer position SRV");
        return false;
    }
    
    return true;
}

void LightingEngine::DestroyGBuffer() {
    if (gBuffer_.albedoSRV) {
        gBuffer_.albedoSRV->Release();
        gBuffer_.albedoSRV = nullptr;
    }
    if (gBuffer_.normalSRV) {
        gBuffer_.normalSRV->Release();
        gBuffer_.normalSRV = nullptr;
    }
    if (gBuffer_.positionSRV) {
        gBuffer_.positionSRV->Release();
        gBuffer_.positionSRV = nullptr;
    }
    if (gBuffer_.albedoRTV) {
        gBuffer_.albedoRTV->Release();
        gBuffer_.albedoRTV = nullptr;
    }
    if (gBuffer_.normalRTV) {
        gBuffer_.normalRTV->Release();
        gBuffer_.normalRTV = nullptr;
    }
    if (gBuffer_.positionRTV) {
        gBuffer_.positionRTV->Release();
        gBuffer_.positionRTV = nullptr;
    }
    if (gBuffer_.albedoTexture) {
        gBuffer_.albedoTexture->Release();
        gBuffer_.albedoTexture = nullptr;
    }
    if (gBuffer_.normalTexture) {
        gBuffer_.normalTexture->Release();
        gBuffer_.normalTexture = nullptr;
    }
    if (gBuffer_.positionTexture) {
        gBuffer_.positionTexture->Release();
        gBuffer_.positionTexture = nullptr;
    }
}

void LightingEngine::BeginFrame() {
    // Clear G-Buffer
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->ClearRenderTargetView(gBuffer_.albedoRTV, clearColor);
    context_->ClearRenderTargetView(gBuffer_.normalRTV, clearColor);
    context_->ClearRenderTargetView(gBuffer_.positionRTV, clearColor);
    
    // Set G-Buffer as render targets
    ID3D11RenderTargetView* renderTargets[] = {
        gBuffer_.albedoRTV,
        gBuffer_.normalRTV,
        gBuffer_.positionRTV
    };
    context_->OMSetRenderTargets(3, renderTargets, nullptr);
}

void LightingEngine::EndFrame() {
    // Perform deferred lighting pass
    PerformDeferredLightingPass();
    
    // Apply post-processing effects
    ApplyBloomEffect();
    ApplyHeatHazeEffect();
    
    // Update self-shadowing if enabled
    if (settings_.enableSelfShadowing) {
        UpdateSelfShadowMaps();
    }
}

void LightingEngine::PerformDeferredLightingPass() {
    // Set scene render target as output
    context_->OMSetRenderTargets(1, &sceneSurface_, nullptr);
    
    // Clear scene render target
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context_->ClearRenderTargetView(sceneSurface_, clearColor);
    
    // Bind G-Buffer textures as shader resources
    ID3D11ShaderResourceView* srvs[] = {
        gBuffer_.albedoSRV,
        gBuffer_.normalSRV,
        gBuffer_.positionSRV
    };
    context_->PSSetShaderResources(0, 3, srvs);
    
    // Perform lighting calculations for each light
    for (const auto& light : lights_) {
        RenderLight(light);
    }
}

void LightingEngine::RenderLight(const Light& light) {
    // Change from passing shared_ptr to dereferencing it
    for (const auto& lightPtr : lights_) {
        if (lightPtr) {
            RenderLight(*lightPtr);  // Dereference the shared_ptr
        }
    }
}

void LightingEngine::ApplyBloomEffect() {
    // Set bloom render target
    context_->OMSetRenderTargets(1, &bloomSurface_, nullptr);
    
    // Clear bloom render target
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->ClearRenderTargetView(bloomSurface_, clearColor);
    
    // Bind scene texture as input
    context_->PSSetShaderResources(0, 1, &sceneSurface_);
    
    // Apply bloom shader (placeholder)
    // This would render a full-screen quad with bloom shader
}

void LightingEngine::ApplyHeatHazeEffect() {
    // Set heat haze render target
    context_->OMSetRenderTargets(1, &heatHazeSurface_, nullptr);
    
    // Clear heat haze render target
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->ClearRenderTargetView(heatHazeSurface_, clearColor);
    
    // Bind scene texture as input
    context_->PSSetShaderResources(0, 1, &sceneSurface_);
    
    // Apply heat haze shader (placeholder)
    // This would render a full-screen quad with heat haze distortion shader
}

void LightingEngine::AddLight(std::shared_ptr<Light> light) {
    if (light) {
        lights_.push_back(light);  // Push the shared_ptr, not the dereferenced object
    }
}

void LightingEngine::RemoveLight(std::shared_ptr<Light> light) {
    if (light) {
        auto it = std::find_if(lights_.begin(), lights_.end(), 
            [&](const std::shared_ptr<Light>& l) { 
                return l.get() == light.get(); 
            });
        if (it != lights_.end()) {
            lights_.erase(it);
        }
    }
}

void LightingEngine::UpdateLight(int lightId, const Light& light) {
    for (auto& existingLight : lights_) {
        if (existingLight.id == lightId) {
            existingLight = light;
            break;
        }
    }
}

void LightingEngine::CreateShadowMap(int lightId, int size) {
    ShadowMap shadowMap;
    shadowMap.lightId = lightId;
    shadowMap.size = size;
    shadowMap.renderTargetView = nullptr;
    shadowMap.shaderResourceView = nullptr;
    shadowMap.depthStencilView = nullptr;
    
    // Create shadow map texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = size;
    textureDesc.Height = size;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    HRESULT hr = device_->CreateTexture2D(&textureDesc, nullptr, &shadowMap.texture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow map texture");
        return;
    }
    
    // Create render target view
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = textureDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    
    hr = device_->CreateRenderTargetView(shadowMap.texture, &rtvDesc, &shadowMap.renderTargetView);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow map render target view");
        return;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    
    hr = device_->CreateShaderResourceView(shadowMap.texture, &srvDesc, &shadowMap.shaderResourceView);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow map shader resource view");
        return;
    }
    
    // Create depth texture
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = size;
    depthDesc.Height = size;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;
    
    hr = device_->CreateTexture2D(&depthDesc, nullptr, &shadowMap.depthTexture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow map depth texture");
        return;
    }
    
    // Create depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    
    hr = device_->CreateDepthStencilView(shadowMap.depthTexture, &dsvDesc, &shadowMap.depthStencilView);
    if (FAILED(hr)) {
        Logger::Error("Failed to create shadow map depth stencil view");
        return;
    }
    
    // Initialize matrices
    shadowMap.lightViewMatrix = XMMatrixIdentity();
    shadowMap.lightProjectionMatrix = XMMatrixIdentity();
    
    shadowMaps_.push_back(shadowMap);
}

void LightingEngine::DestroyShadowMap(ShadowMap& shadowMap) {
    if (shadowMap.shaderResourceView) {
        shadowMap.shaderResourceView->Release();
        shadowMap.shaderResourceView = nullptr;
    }
    if (shadowMap.renderTargetView) {
        shadowMap.renderTargetView->Release();
        shadowMap.renderTargetView = nullptr;
    }
    if (shadowMap.depthStencilView) {
        shadowMap.depthStencilView->Release();
        shadowMap.depthStencilView = nullptr;
    }
    if (shadowMap.texture) {
        shadowMap.texture->Release();
        shadowMap.texture = nullptr;
    }
    if (shadowMap.depthTexture) {
        shadowMap.depthTexture->Release();
        shadowMap.depthTexture = nullptr;
    }
}

void LightingEngine::UpdateSelfShadowMaps() {
    // Update self-shadowing shadow maps
    // This is a placeholder implementation
    for (auto& shadowMap : shadowMaps_) {
        // Render to shadow map
        context_->OMSetRenderTargets(1, &shadowMap.renderTargetView, shadowMap.depthStencilView);
        
        // Clear shadow map
        float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        context_->ClearRenderTargetView(shadowMap.renderTargetView, clearColor);
        context_->ClearDepthStencilView(shadowMap.depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        
        // Render scene from light's perspective
        // This would be implemented based on the specific rendering pipeline
    }
}

void LightingEngine::SetSettings(const LightingSettings& settings) {
    settings_ = settings;
}

LightingSettings LightingEngine::GetSettings() const {
    return settings_;
}

} // namespace Nexus
