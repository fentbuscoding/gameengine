#include "GraphicsDevice.h"
#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include "Shader.h"
#include "Logger.h"
#include <iostream>
#include <algorithm>

namespace Nexus {

// Constants for sphere generation
const int SPHERE_RINGS = 20;
const int SPHERE_SECTORS = 36;

GraphicsDevice::GraphicsDevice()
    : device_(nullptr)
    , context_(nullptr)
    , swapChain_(nullptr)
    , renderTargetView_(nullptr)
    , depthStencilView_(nullptr)
    , width_(0)
    , height_(0)
    , fullscreen_(false)
    , bloomEnabled_(false)
    , heatHazeEnabled_(false)
    , bloomThreshold_(0.8f)
    , bloomIntensity_(1.2f)
    , shadowsEnabled_(false)
    , shadowMapSize_(1024)
    , bloomTexture_(nullptr)
    , bloomRenderTarget_(nullptr)
    , heatHazeTexture_(nullptr)
    , heatHazeRenderTarget_(nullptr)
    , shadowMap_(nullptr)
    , shadowMapDepth_(nullptr)
    , boxVertexBuffer_(nullptr)
    , boxIndexBuffer_(nullptr)
    , sphereVertexBuffer_(nullptr)
    , sphereIndexBuffer_(nullptr)
    , constantBuffer_(nullptr)
    , basicVertexShader_(nullptr)
    , basicPixelShader_(nullptr)
    , basicInputLayout_(nullptr)
{
}

GraphicsDevice::~GraphicsDevice() {
    Shutdown();
}

bool GraphicsDevice::Initialize(HWND hwnd, int width, int height, bool fullscreen) {
    width_ = width;
    height_ = height;
    fullscreen_ = fullscreen;

    // DirectX 11 initialization
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = width_;
    swapChainDesc.BufferDesc.Height = height_;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = !fullscreen_;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &swapChain_,
        &device_,
        &featureLevel,
        &context_
    );

    if (FAILED(hr)) {
        Logger::Error("Failed to create DirectX 11 device and swap chain");
        return false;
    }

    // Create render target view
    ID3D11Texture2D* backBufferTexture;
    hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture);
    if (FAILED(hr)) {
        Logger::Error("Failed to get back buffer");
        return false;
    }

    hr = device_->CreateRenderTargetView(backBufferTexture, nullptr, &renderTargetView_);
    if (FAILED(hr)) {
        backBufferTexture->Release();
        Logger::Error("Failed to create render target view");
        return false;
    }

    // Create depth stencil buffer
    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    depthBufferDesc.Width = width_;
    depthBufferDesc.Height = height_;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    ID3D11Texture2D* depthStencilBuffer;
    hr = device_->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer);
    if (FAILED(hr)) {
        backBufferTexture->Release();
        Logger::Error("Failed to create depth stencil buffer");
        return false;
    }

    hr = device_->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView_);
    depthStencilBuffer->Release();
    backBufferTexture->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create depth stencil view");
        return false;
    }

    // Set render targets
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)width_;
    viewport.Height = (float)height_;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    context_->RSSetViewports(1, &viewport);

    // Initialize post-processing
    InitializePostProcessing();

    // Initialize shadow mapping
    InitializeShadowMapping();

    // Create default shaders
    CreateDefaultShaders();

    // Initialize primitive rendering
    InitializePrimitiveRendering();

    Logger::Info("DirectX 11 initialized successfully");
    Logger::Info("Resolution: " + std::to_string(width_) + "x" + std::to_string(height_));
    Logger::Info("Fullscreen: " + std::string(fullscreen_ ? "Yes" : "No"));

    return true;
}
void GraphicsDevice::InitializePostProcessing() {
    // Create bloom render target
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width_;
    textureDesc.Height = height_;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    if (SUCCEEDED(device_->CreateTexture2D(&textureDesc, nullptr, &bloomTexture_))) {
        device_->CreateRenderTargetView(bloomTexture_, nullptr, &bloomRenderTarget_);
    }

    // Create heat haze render target
    if (SUCCEEDED(device_->CreateTexture2D(&textureDesc, nullptr, &heatHazeTexture_))) {
        device_->CreateRenderTargetView(heatHazeTexture_, nullptr, &heatHazeRenderTarget_);
    }
}

void GraphicsDevice::InitializeShadowMapping() {
    // Create shadow map texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = shadowMapSize_;
    textureDesc.Height = shadowMapSize_;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    if (SUCCEEDED(device_->CreateTexture2D(&textureDesc, nullptr, &shadowMap_))) {
        // Create shadow map depth stencil buffer
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = shadowMapSize_;
        depthDesc.Height = shadowMapSize_;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthDesc.CPUAccessFlags = 0;
        depthDesc.MiscFlags = 0;

        ID3D11Texture2D* shadowDepthTexture = nullptr;
        if (SUCCEEDED(device_->CreateTexture2D(&depthDesc, nullptr, &shadowDepthTexture))) {
            device_->CreateDepthStencilView(shadowDepthTexture, nullptr, &shadowMapDepth_);
            shadowDepthTexture->Release();
        }
    }
}

void GraphicsDevice::CreateDefaultShaders() {
    // This would load actual HLSL shader files
    // For now, we'll create placeholder shaders
    defaultShader_ = std::make_unique<Shader>();
    normalMapShader_ = std::make_unique<Shader>();
    bloomShader_ = std::make_unique<Shader>();
}

void GraphicsDevice::BeginFrame() {
    if (!context_) return;

    // Clear the render target and depth stencil
    float clearColor[4] = { 0.25f, 0.5f, 1.0f, 1.0f }; // Light blue
    context_->ClearRenderTargetView(renderTargetView_, clearColor);
    context_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void GraphicsDevice::EndFrame() {
    if (!context_) return;

    // Apply post-processing effects
    if (bloomEnabled_) {
        RenderBloomPass();
    }

    if (heatHazeEnabled_) {
        RenderHeatHazePass();
    }
}

void GraphicsDevice::Present() {
    if (!swapChain_) return;

    HRESULT hr = swapChain_->Present(0, 0);
    
    if (FAILED(hr)) {
        Logger::Warning("Present failed");
    }
}

void GraphicsDevice::RenderBloomPass() {
    // Implementation of bloom post-processing
    // This would involve rendering to the bloom texture and then
    // blending it back to the main render target
    if (!bloomRenderTarget_) return;

    // Set bloom render target
    context_->OMSetRenderTargets(1, &bloomRenderTarget_, nullptr);
    
    // Clear bloom buffer
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->ClearRenderTargetView(bloomRenderTarget_, clearColor);
    
    // Render bloom effect (simplified)
    // ... bloom rendering logic would go here ...
    
    // Restore main render target
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
}

void GraphicsDevice::RenderHeatHazePass() {
    // Implementation of heat haze distortion
    // This would apply a distortion effect based on a noise texture
    if (!heatHazeRenderTarget_) return;

    // Set heat haze render target
    context_->OMSetRenderTargets(1, &heatHazeRenderTarget_, nullptr);
    
    // Clear heat haze buffer
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->ClearRenderTargetView(heatHazeRenderTarget_, clearColor);
    
    // Render heat haze effect (simplified)
    // ... heat haze rendering logic would go here ...
    
    // Restore main render target
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
}

void GraphicsDevice::SetShadowMapSize(int size) {
    shadowMapSize_ = size;
    
    // Release existing shadow map resources
    if (shadowMapDepth_) { shadowMapDepth_->Release(); shadowMapDepth_ = nullptr; }
    if (shadowMap_) { shadowMap_->Release(); shadowMap_ = nullptr; }
    
    // Recreate shadow map with new size
    InitializeShadowMapping();
}

void GraphicsDevice::BeginShadowPass(const Light& light) {
    if (!shadowsEnabled_ || !shadowMap_) return;

    // Create render target view for shadow map if not exists
    ID3D11RenderTargetView* shadowRTV = nullptr;
    device_->CreateRenderTargetView(shadowMap_, nullptr, &shadowRTV);
    
    // Set shadow map as render target
    context_->OMSetRenderTargets(1, &shadowRTV, shadowMapDepth_);

    // Clear shadow map
    float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    context_->ClearRenderTargetView(shadowRTV, clearColor);
    if (shadowMapDepth_) {
        context_->ClearDepthStencilView(shadowMapDepth_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    // Set shadow map viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)shadowMapSize_;
    viewport.Height = (float)shadowMapSize_;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);

    // Clean up temporary views
    if (shadowRTV) shadowRTV->Release();
}

void GraphicsDevice::EndShadowPass() {
    if (!shadowsEnabled_) return;

    // Restore main render target
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
    
    // Restore main viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)width_;
    viewport.Height = (float)height_;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
}

void GraphicsDevice::Shutdown() {
    // Release primitive rendering resources
    if (basicInputLayout_) { basicInputLayout_->Release(); basicInputLayout_ = nullptr; }
    if (basicPixelShader_) { basicPixelShader_->Release(); basicPixelShader_ = nullptr; }
    if (basicVertexShader_) { basicVertexShader_->Release(); basicVertexShader_ = nullptr; }
    if (constantBuffer_) { constantBuffer_->Release(); constantBuffer_ = nullptr; }
    if (sphereIndexBuffer_) { sphereIndexBuffer_->Release(); sphereIndexBuffer_ = nullptr; }
    if (sphereVertexBuffer_) { sphereVertexBuffer_->Release(); sphereVertexBuffer_ = nullptr; }
    if (boxIndexBuffer_) { boxIndexBuffer_->Release(); boxIndexBuffer_ = nullptr; }
    if (boxVertexBuffer_) { boxVertexBuffer_->Release(); boxVertexBuffer_ = nullptr; }
    
    // Release all DirectX 11 resources
    if (shadowMapDepth_) { shadowMapDepth_->Release(); shadowMapDepth_ = nullptr; }
    if (shadowMap_) { shadowMap_->Release(); shadowMap_ = nullptr; }
    if (heatHazeRenderTarget_) { heatHazeRenderTarget_->Release(); heatHazeRenderTarget_ = nullptr; }
    if (heatHazeTexture_) { heatHazeTexture_->Release(); heatHazeTexture_ = nullptr; }
    if (bloomRenderTarget_) { bloomRenderTarget_->Release(); bloomRenderTarget_ = nullptr; }
    if (bloomTexture_) { bloomTexture_->Release(); bloomTexture_ = nullptr; }
    if (depthStencilView_) { depthStencilView_->Release(); depthStencilView_ = nullptr; }
    if (renderTargetView_) { renderTargetView_->Release(); renderTargetView_ = nullptr; }
    if (swapChain_) { swapChain_->Release(); swapChain_ = nullptr; }
    if (context_) { context_->Release(); context_ = nullptr; }
    if (device_) { device_->Release(); device_ = nullptr; }

    Logger::Info("Graphics device shutdown complete");
}

bool GraphicsDevice::IsDeviceLost() const {
    if (!device_) return true;
    
    HRESULT hr = device_->GetDeviceRemovedReason();
    return FAILED(hr);
}

bool GraphicsDevice::ResetDevice() {
    // DirectX 11 doesn't support device reset like DirectX 9
    // Instead, we need to recreate the device and all resources
    Logger::Warning("GraphicsDevice: Device reset not supported in DirectX 11");
    Logger::Warning("GraphicsDevice: Consider recreating the device instead");
    
    // For now, just return success if device is valid
    return device_ != nullptr;
}

void GraphicsDevice::SetViewport(int x, int y, int width, int height) {
    if (!context_) return;
    
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = static_cast<FLOAT>(x);
    viewport.TopLeftY = static_cast<FLOAT>(y);
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    
    context_->RSSetViewports(1, &viewport);
}

void GraphicsDevice::Clear(const XMFLOAT4& color) {
    if (!context_ || !renderTargetView_) return;
    
    float clearColor[4] = { color.x, color.y, color.z, color.w };
    context_->ClearRenderTargetView(renderTargetView_, clearColor);
    
    if (depthStencilView_) {
        context_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
}

void GraphicsDevice::InitializePrimitiveRendering() {
    // Initialize matrices
    XMStoreFloat4x4(&projectionMatrix_, XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)width_ / (float)height_, 0.1f, 1000.0f));
    
    // Setup default camera
    SetupBasicCamera(XMFLOAT3(0.0f, 5.0f, -15.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
    
    // Create geometry
    CreateBoxGeometry();
    CreateSphereGeometry();
    
    // Create shaders
    CreateBasicShaders();
    
    // Create constant buffer
    CreateConstantBuffer();
    
    Logger::Info("Primitive rendering initialized successfully");
}

void GraphicsDevice::CreateBoxGeometry() {
    // Box vertices
    Vertex boxVertices[] = {
        // Front face
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
        
        // Back face
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        
        // Left face
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        
        // Right face
        { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        
        // Top face
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        
        // Bottom face
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
    };
    
    // Box indices
    UINT boxIndices[] = {
        0, 1, 2, 0, 2, 3,    // Front face
        4, 6, 5, 4, 7, 6,    // Back face
        8, 9, 10, 8, 10, 11, // Left face
        12, 13, 14, 12, 14, 15, // Right face
        16, 17, 18, 16, 18, 19, // Top face
        20, 21, 22, 20, 22, 23  // Bottom face
    };
    
    // Create vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(boxVertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = boxVertices;
    
    device_->CreateBuffer(&bufferDesc, &vertexData, &boxVertexBuffer_);
    
    // Create index buffer
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(boxIndices);
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = boxIndices;
    
    device_->CreateBuffer(&bufferDesc, &indexData, &boxIndexBuffer_);
}

void GraphicsDevice::CreateSphereGeometry() {
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;
    
    // Generate sphere vertices
    for (int i = 0; i <= SPHERE_RINGS; i++) {
        float phi = XM_PI * (float)i / (float)SPHERE_RINGS;
        for (int j = 0; j <= SPHERE_SECTORS; j++) {
            float theta = 2.0f * XM_PI * (float)j / (float)SPHERE_SECTORS;
            
            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);
            
            vertices.push_back({ XMFLOAT3(x, y, z), XMFLOAT3(x, y, z) });
        }
    }
    
    // Generate sphere indices
    for (int i = 0; i < SPHERE_RINGS; i++) {
        for (int j = 0; j < SPHERE_SECTORS; j++) {
            int first = i * (SPHERE_SECTORS + 1) + j;
            int second = first + SPHERE_SECTORS + 1;
            
            // First triangle
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            // Second triangle
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    // Create vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(Vertex) * vertices.size();
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices.data();
    
    device_->CreateBuffer(&bufferDesc, &vertexData, &sphereVertexBuffer_);
    
    // Create index buffer
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * indices.size();
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    
    device_->CreateBuffer(&bufferDesc, &indexData, &sphereIndexBuffer_);
}

void GraphicsDevice::CreateBasicShaders() {
    // Basic vertex shader
    const char* vertexShaderSource = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
            float4 Color;
        };
        
        struct VS_INPUT
        {
            float3 Position : POSITION;
            float3 Normal : NORMAL;
        };
        
        struct VS_OUTPUT
        {
            float4 Position : SV_POSITION;
            float3 Normal : NORMAL;
            float4 Color : COLOR;
        };
        
        VS_OUTPUT main(VS_INPUT input)
        {
            VS_OUTPUT output;
            
            matrix wvp = mul(mul(World, View), Projection);
            output.Position = mul(float4(input.Position, 1.0f), wvp);
            output.Normal = normalize(mul(input.Normal, (float3x3)World));
            output.Color = Color;
            
            return output;
        }
    )";
    
    // Basic pixel shader
    const char* pixelShaderSource = R"(
        struct PS_INPUT
        {
            float4 Position : SV_POSITION;
            float3 Normal : NORMAL;
            float4 Color : COLOR;
        };
        
        float4 main(PS_INPUT input) : SV_TARGET
        {
            float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
            float3 normal = normalize(input.Normal);
            float lighting = saturate(dot(normal, lightDir)) * 0.8f + 0.2f;
            
            return float4(input.Color.rgb * lighting, input.Color.a);
        }
    )";
    
    // Compile vertex shader
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    
    HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            Logger::Error("Vertex shader compilation error: " + std::string((char*)errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        Logger::Error("Failed to compile vertex shader");
        return;
    }
    
    hr = device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &basicVertexShader_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create vertex shader");
        vsBlob->Release();
        return;
    }
    
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    
    hr = device_->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &basicInputLayout_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create input layout");
    }
    
    vsBlob->Release();
    
    // Compile pixel shader
    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_DEBUG, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            Logger::Error("Pixel shader compilation error: " + std::string((char*)errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        Logger::Error("Failed to compile pixel shader");
        return;
    }
    
    hr = device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &basicPixelShader_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create pixel shader");
    }
    
    psBlob->Release();
    
    Logger::Info("Basic shaders compiled successfully");
}

void GraphicsDevice::CreateConstantBuffer() {
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(ConstantBufferData);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    
    device_->CreateBuffer(&bufferDesc, nullptr, &constantBuffer_);
}

void GraphicsDevice::SetupBasicCamera(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up) {
    XMVECTOR eyePos = XMVectorSet(position.x, position.y, position.z, 1.0f);
    XMVECTOR targetPos = XMVectorSet(target.x, target.y, target.z, 1.0f);
    XMVECTOR upVector = XMVectorSet(up.x, up.y, up.z, 0.0f);
    
    XMStoreFloat4x4(&viewMatrix_, XMMatrixLookAtLH(eyePos, targetPos, upVector));
}

void GraphicsDevice::RenderBox(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& color) {
    if (!boxVertexBuffer_ || !boxIndexBuffer_) return;
    
    // Setup world matrix
    XMMATRIX world = XMMatrixScaling(size.x, size.y, size.z) * XMMatrixTranslation(position.x, position.y, position.z);
    
    // Update constant buffer
    ConstantBufferData cbData;
    XMStoreFloat4x4(&cbData.world, world);
    cbData.view = viewMatrix_;
    cbData.projection = projectionMatrix_;
    cbData.color = color;
    
    context_->UpdateSubresource(constantBuffer_, 0, nullptr, &cbData, 0, 0);
    
    // Set shaders and input layout
    context_->VSSetShader(basicVertexShader_, nullptr, 0);
    context_->PSSetShader(basicPixelShader_, nullptr, 0);
    context_->IASetInputLayout(basicInputLayout_);
    
    // Set vertex and index buffers
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context_->IASetVertexBuffers(0, 1, &boxVertexBuffer_, &stride, &offset);
    context_->IASetIndexBuffer(boxIndexBuffer_, DXGI_FORMAT_R32_UINT, 0);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Set constant buffer
    context_->VSSetConstantBuffers(0, 1, &constantBuffer_);
    
    // Draw
    context_->DrawIndexed(36, 0, 0);
}

void GraphicsDevice::RenderSphere(const XMFLOAT3& position, float radius, const XMFLOAT4& color) {
    if (!sphereVertexBuffer_ || !sphereIndexBuffer_) return;
    
    // Setup world matrix
    XMMATRIX world = XMMatrixScaling(radius, radius, radius) * XMMatrixTranslation(position.x, position.y, position.z);
    
    // Update constant buffer
    ConstantBufferData cbData;
    XMStoreFloat4x4(&cbData.world, world);
    cbData.view = viewMatrix_;
    cbData.projection = projectionMatrix_;
    cbData.color = color;
    
    context_->UpdateSubresource(constantBuffer_, 0, nullptr, &cbData, 0, 0);
    
    // Set shaders and input layout
    context_->VSSetShader(basicVertexShader_, nullptr, 0);
    context_->PSSetShader(basicPixelShader_, nullptr, 0);
    context_->IASetInputLayout(basicInputLayout_);
    
    // Set vertex and index buffers
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context_->IASetVertexBuffers(0, 1, &sphereVertexBuffer_, &stride, &offset);
    context_->IASetIndexBuffer(sphereIndexBuffer_, DXGI_FORMAT_R32_UINT, 0);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Set constant buffer
    context_->VSSetConstantBuffers(0, 1, &constantBuffer_);
    
    // Draw
    context_->DrawIndexed(SPHERE_RINGS * SPHERE_SECTORS * 6, 0, 0);
}

void GraphicsDevice::RenderCapsule(const XMFLOAT3& position, float radius, float height, const XMFLOAT4& color) {
    // For now, render as a scaled sphere - can be improved later
    RenderSphere(position, radius, color);
}

// Texture loading implementations
bool GraphicsDevice::LoadTexture(const std::string& filename, ID3D11ShaderResourceView** srv) {
    if (!device_) return false;
    
    // Try different formats
    if (filename.find(".dds") != std::string::npos) {
        return LoadDDSTexture(filename, srv);
    } else if (filename.find(".tga") != std::string::npos) {
        return LoadTGATexture(filename, srv);
    } else if (filename.find(".bmp") != std::string::npos) {
        return LoadBMPTexture(filename, srv);
    } else {
        return LoadUnrealTexture(filename, srv);
    }
}

bool GraphicsDevice::LoadUnrealTexture(const std::string& filename, ID3D11ShaderResourceView** srv) {
    if (!device_ || !srv) return false;
    
    Logger::Info("Loading Unreal Engine texture: " + filename);
    
    // Unreal Engine textures are typically stored in .uasset files
    // For now, create a placeholder texture with a checkerboard pattern
    // In a full implementation, you would need to parse the UE4/UE5 asset format
    
    const int width = 256;
    const int height = 256;
    const int channels = 4; // RGBA
    
    std::vector<unsigned char> data(width * height * channels);
    
    // Create checkerboard pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * channels;
            bool isWhite = ((x / 32) + (y / 32)) % 2 == 0;
            
            if (isWhite) {
                data[index] = 255;     // R
                data[index + 1] = 255; // G
                data[index + 2] = 255; // B
            } else {
                data[index] = 128;     // R
                data[index + 1] = 128; // G
                data[index + 2] = 128; // B
            }
            data[index + 3] = 255;     // A
        }
    }
    
    // Create texture description
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = data.data();
    textureData.SysMemPitch = width * channels;
    textureData.SysMemSlicePitch = 0;
    
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device_->CreateTexture2D(&textureDesc, &textureData, &texture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create Unreal texture");
        return false;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    hr = device_->CreateShaderResourceView(texture, &srvDesc, srv);
    texture->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create shader resource view for Unreal texture");
        return false;
    }
    
    Logger::Info("Successfully loaded Unreal texture (placeholder)");
    return true;
}

bool GraphicsDevice::LoadDDSTexture(const std::string& filename, ID3D11ShaderResourceView** srv) {
    if (!device_ || !srv) return false;
    
    Logger::Info("Loading DDS texture: " + filename);
    
    // For now, create a placeholder blue texture
    // In a full implementation, you would use DirectXTex library to load DDS files
    
    const int width = 128;
    const int height = 128;
    const int channels = 4;
    
    std::vector<unsigned char> data(width * height * channels);
    
    // Fill with blue color
    for (int i = 0; i < width * height; i++) {
        data[i * channels] = 64;      // R
        data[i * channels + 1] = 128; // G
        data[i * channels + 2] = 255; // B
        data[i * channels + 3] = 255; // A
    }
    
    // Create texture (same process as Unreal texture)
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = data.data();
    textureData.SysMemPitch = width * channels;
    textureData.SysMemSlicePitch = 0;
    
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device_->CreateTexture2D(&textureDesc, &textureData, &texture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create DDS texture");
        return false;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    hr = device_->CreateShaderResourceView(texture, &srvDesc, srv);
    texture->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create shader resource view for DDS texture");
        return false;
    }
    
    Logger::Info("Successfully loaded DDS texture (placeholder)");
    return true;
}

bool GraphicsDevice::LoadTGATexture(const std::string& filename, ID3D11ShaderResourceView** srv) {
    if (!device_ || !srv) return false;
    
    Logger::Info("Loading TGA texture: " + filename);
    
    // Create a placeholder red texture
    const int width = 64;
    const int height = 64;
    const int channels = 4;
    
    std::vector<unsigned char> data(width * height * channels);
    
    // Fill with red color
    for (int i = 0; i < width * height; i++) {
        data[i * channels] = 255;     // R
        data[i * channels + 1] = 64;  // G
        data[i * channels + 2] = 64;  // B
        data[i * channels + 3] = 255; // A
    }
    
    // Create texture (same process)
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = data.data();
    textureData.SysMemPitch = width * channels;
    textureData.SysMemSlicePitch = 0;
    
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device_->CreateTexture2D(&textureDesc, &textureData, &texture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create TGA texture");
        return false;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    hr = device_->CreateShaderResourceView(texture, &srvDesc, srv);
    texture->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create shader resource view for TGA texture");
        return false;
    }
    
    Logger::Info("Successfully loaded TGA texture (placeholder)");
    return true;
}

bool GraphicsDevice::LoadBMPTexture(const std::string& filename, ID3D11ShaderResourceView** srv) {
    if (!device_ || !srv) return false;
    
    Logger::Info("Loading BMP texture: " + filename);
    
    // Create a placeholder green texture
    const int width = 64;
    const int height = 64;
    const int channels = 4;
    
    std::vector<unsigned char> data(width * height * channels);
    
    // Fill with green color
    for (int i = 0; i < width * height; i++) {
        data[i * channels] = 64;      // R
        data[i * channels + 1] = 255; // G
        data[i * channels + 2] = 64;  // B
        data[i * channels + 3] = 255; // A
    }
    
    // Create texture (same process)
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    
    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = data.data();
    textureData.SysMemPitch = width * channels;
    textureData.SysMemSlicePitch = 0;
    
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device_->CreateTexture2D(&textureDesc, &textureData, &texture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create BMP texture");
        return false;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    hr = device_->CreateShaderResourceView(texture, &srvDesc, srv);
    texture->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create shader resource view for BMP texture");
        return false;
    }
    
    Logger::Info("Successfully loaded BMP texture (placeholder)");
    return true;
}

bool GraphicsDevice::LoadUnrealAsset(const std::string& filePath) {
    Logger::Info("Loading Unreal Engine asset: " + filePath);
    
    // Placeholder implementation for Unreal Engine asset loading
    // In a full implementation, you would need to parse UE4/UE5 asset formats
    // This could include .uasset, .umap, .pak files
    
    std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "uasset") {
        Logger::Info("Detected Unreal Engine asset file");
        // Parse UAsset format (placeholder)
        return true;
    } else if (extension == "umap") {
        Logger::Info("Detected Unreal Engine map file");
        // Parse UMap format (placeholder)
        return true;
    } else if (extension == "pak") {
        Logger::Info("Detected Unreal Engine package file");
        // Parse PAK format (placeholder)
        return true;
    }
    
    Logger::Warning("Unsupported Unreal Engine asset format: " + extension);
    return false;
}

bool GraphicsDevice::LoadFBX(const std::string& filePath) {
    Logger::Info("Loading FBX file: " + filePath);
    
    // Placeholder implementation for FBX loading
    // In a full implementation, you would use FBX SDK or Assimp
    
    Logger::Info("FBX loading not yet implemented (placeholder)");
    return true;
}

bool GraphicsDevice::LoadOBJ(const std::string& filePath) {
    Logger::Info("Loading OBJ file: " + filePath);
    
    // Placeholder implementation for OBJ loading
    // This is a simple text-based format that's easier to parse
    
    Logger::Info("OBJ loading not yet implemented (placeholder)");
    return true;
}
