#include "GraphicsDevice.h"
#include "Logger.h"
#include "UnrealTextureLoader.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <cstring>

namespace Nexus {

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
    Logger::Info("Initializing Graphics Device...");
    
    width_ = width;
    height_ = height;
    fullscreen_ = fullscreen;
    
    // Create DXGI factory
    IDXGIFactory* factory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(hr)) {
        Logger::Error("Failed to create DXGI factory");
        return false;
    }
    
    // Create device and swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = !fullscreen;
    
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &swapChain_, &device_, &featureLevel, &context_
    );
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create D3D11 device and swap chain");
        factory->Release();
        return false;
    }
    
    factory->Release();
    
    // Create render target view
    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) {
        Logger::Error("Failed to get back buffer");
        return false;
    }
    
    hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    backBuffer->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create render target view");
        return false;
    }
    
    // Create depth stencil buffer
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;
    
    ID3D11Texture2D* depthStencilBuffer = nullptr;
    hr = device_->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
    if (FAILED(hr)) {
        Logger::Error("Failed to create depth stencil buffer");
        return false;
    }
    
    hr = device_->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView_);
    depthStencilBuffer->Release();
    
    if (FAILED(hr)) {
        Logger::Error("Failed to create depth stencil view");
        return false;
    }
    
    // Set render targets
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
    
    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (float)width;
    viewport.Height = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    
    // Set projection matrix
    float aspectRatio = (float)width / (float)height;
    DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XM_PIDIV4, aspectRatio, 0.1f, 1000.0f
    );
    DirectX::XMStoreFloat4x4(&projectionMatrix_, projection);
    
    // Initialize primitive rendering
    InitializePrimitiveRendering();
    
    Logger::Info("Graphics Device initialized successfully");
    return true;
}

void GraphicsDevice::Shutdown() {
    // Clean up DirectX resources
    if (basicInputLayout_) { basicInputLayout_->Release(); basicInputLayout_ = nullptr; }
    if (basicPixelShader_) { basicPixelShader_->Release(); basicPixelShader_ = nullptr; }
    if (basicVertexShader_) { basicVertexShader_->Release(); basicVertexShader_ = nullptr; }
    if (constantBuffer_) { constantBuffer_->Release(); constantBuffer_ = nullptr; }
    if (sphereIndexBuffer_) { sphereIndexBuffer_->Release(); sphereIndexBuffer_ = nullptr; }
    if (sphereVertexBuffer_) { sphereVertexBuffer_->Release(); sphereVertexBuffer_ = nullptr; }
    if (boxIndexBuffer_) { boxIndexBuffer_->Release(); boxIndexBuffer_ = nullptr; }
    if (boxVertexBuffer_) { boxVertexBuffer_->Release(); boxVertexBuffer_ = nullptr; }
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
}

void GraphicsDevice::BeginFrame() {
    static bool firstBegin = true;
    if (firstBegin) {
        Logger::Info("GraphicsDevice::BeginFrame() - First call");
        firstBegin = false;
    }
    // Nothing needed for basic implementation
}

void GraphicsDevice::EndFrame() {
    // Nothing needed for basic implementation
}

void GraphicsDevice::Present() {
    static bool firstPresent = true;
    if (firstPresent) {
        Logger::Info("GraphicsDevice::Present() - First call");
        firstPresent = false;
    }
    
    if (swapChain_) {
        HRESULT hr = swapChain_->Present(0, 0);
        if (FAILED(hr)) {
            Logger::Error("Present failed with HRESULT: 0x" + std::to_string(hr));
        } else if (firstPresent) {
            Logger::Info("Present succeeded");
        }
    } else {
        Logger::Error("SwapChain is null in Present()");
    }
}

void GraphicsDevice::Clear(const DirectX::XMFLOAT4& color) {
    static bool firstClear = true;
    if (firstClear) {
        Logger::Info("GraphicsDevice::Clear() - First call");
        firstClear = false;
    }
    
    float clearColor[4] = { color.x, color.y, color.z, color.w };
    context_->ClearRenderTargetView(renderTargetView_, clearColor);
    context_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void GraphicsDevice::SetViewport(int x, int y, int width, int height) {
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = (float)x;
    viewport.TopLeftY = (float)y;
    viewport.Width = (float)width;
    viewport.Height = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
}

bool GraphicsDevice::IsDeviceLost() {
    return false; // Simplified implementation
}

bool GraphicsDevice::ResetDevice() {
    return true; // Simplified implementation
}

// Texture loading functions (now using UnrealTextureLoader)
ID3D11Texture2D* GraphicsDevice::LoadTexture(const std::string& filename) {
    Logger::Info("Loading texture: " + filename);
    
    auto textureData = UnrealTextureLoader::LoadUnrealTexture(filename);
    if (!textureData || !textureData->IsValid()) {
        Logger::Error("Failed to load texture: " + filename);
        return nullptr;
    }
    
    Logger::Info("Texture loaded successfully: " + std::to_string(textureData->metadata.width) + "x" + 
                 std::to_string(textureData->metadata.height));
    
    // Create D3D11 texture from loaded data
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = textureData->metadata.width;
    textureDesc.Height = textureData->metadata.height;
    textureDesc.MipLevels = textureData->metadata.mipLevels;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA textureSubData = {};
    textureSubData.pSysMem = textureData->data.data();
    textureSubData.SysMemPitch = textureData->metadata.width * 4;
    textureSubData.SysMemSlicePitch = 0;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device_->CreateTexture2D(&textureDesc, &textureSubData, &texture);
    if (FAILED(hr)) {
        Logger::Error("Failed to create D3D11 texture");
        return nullptr;
    }

    return texture;
}

ID3D11Texture2D* GraphicsDevice::LoadUnrealTexture(const std::string& filename) {
    Logger::Info("Loading Unreal texture: " + filename);
    return LoadTexture(filename);
}

ID3D11Texture2D* GraphicsDevice::LoadDDSTexture(const std::string& filename) {
    Logger::Info("Loading DDS texture: " + filename);
    
    auto textureData = UnrealTextureLoader::LoadDDS(filename);
    if (!textureData || !textureData->IsValid()) {
        Logger::Error("Failed to load DDS texture: " + filename);
        return nullptr;
    }
    
    return LoadTexture(filename);
}

ID3D11Texture2D* GraphicsDevice::LoadTGATexture(const std::string& filename) {
    Logger::Info("Loading TGA texture: " + filename);
    
    auto textureData = UnrealTextureLoader::LoadTGA(filename);
    if (!textureData || !textureData->IsValid()) {
        Logger::Error("Failed to load TGA texture: " + filename);
        return nullptr;
    }
    
    return LoadTexture(filename);
}

ID3D11Texture2D* GraphicsDevice::LoadBMPTexture(const std::string& filename) {
    Logger::Info("Loading BMP texture: " + filename);
    
    auto textureData = UnrealTextureLoader::LoadBMP(filename);
    if (!textureData || !textureData->IsValid()) {
        Logger::Error("Failed to load BMP texture: " + filename);
        return nullptr;
    }
    
    return LoadTexture(filename);
}

bool GraphicsDevice::LoadUnrealAsset(const std::string& filename) {
    Logger::Info("Loading Unreal asset: " + filename);
    
    auto assetData = UnrealAssetLoader::LoadUAsset(filename);
    if (!assetData || !assetData->isValid) {
        Logger::Error("Failed to load Unreal asset: " + filename);
        return false;
    }
    
    Logger::Info("Successfully loaded Unreal asset: " + filename);
    Logger::Info("Asset contains " + std::to_string(assetData->meshes.size()) + " meshes and " + 
                 std::to_string(assetData->materials.size()) + " materials");
    
    return true;
}

bool GraphicsDevice::LoadFBX(const std::string& filename) {
    Logger::Info("Loading FBX: " + filename);
    
    auto assetData = UnrealAssetLoader::LoadFBX(filename);
    if (!assetData || !assetData->isValid) {
        Logger::Error("Failed to load FBX: " + filename);
        return false;
    }
    
    Logger::Info("Successfully loaded FBX: " + filename);
    Logger::Info("FBX contains " + std::to_string(assetData->meshes.size()) + " meshes and " + 
                 std::to_string(assetData->materials.size()) + " materials");
    
    return true;
}

bool GraphicsDevice::LoadOBJ(const std::string& filename) {
    Logger::Info("Loading OBJ: " + filename);
    
    auto assetData = UnrealAssetLoader::LoadOBJ(filename);
    if (!assetData || !assetData->isValid) {
        Logger::Error("Failed to load OBJ: " + filename);
        return false;
    }
    
    Logger::Info("Successfully loaded OBJ: " + filename);
    Logger::Info("OBJ contains " + std::to_string(assetData->meshes.size()) + " meshes and " + 
                 std::to_string(assetData->materials.size()) + " materials");
    
    return true;
}

void GraphicsDevice::InitializePrimitiveRendering() {
    Logger::Info("Initializing primitive rendering...");
    
    // Set up basic camera
    SetupBasicCamera(DirectX::XMFLOAT3(0.0f, 5.0f, -15.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));
    
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
    // Simple box geometry
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
    };
    
    Vertex boxVertices[] = {
        // Front face
        { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { DirectX::XMFLOAT3( 1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f) },
        { DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f) },
        
        // Back face
        { DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3( 1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f) },
        
        // Left face
        { DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f) },
        
        // Right face
        { DirectX::XMFLOAT3( 1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f) },
        
        // Top face
        { DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f) },
        
        // Bottom face
        { DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f) },
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
    // Simple sphere geometry (placeholder)
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
    };
    
    std::vector<Vertex> sphereVertices;
    std::vector<UINT> sphereIndices;
    
    // Generate sphere vertices
    const int rings = 16;
    const int sectors = 32;
    const float radius = 1.0f;
    
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = DirectX::XM_PI * (float)ring / (float)rings;
        for (int sector = 0; sector <= sectors; ++sector) {
            float theta = 2 * DirectX::XM_PI * (float)sector / (float)sectors;
            
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            
            Vertex vertex;
            vertex.position = DirectX::XMFLOAT3(x, y, z);
            vertex.normal = DirectX::XMFLOAT3(x, y, z);
            sphereVertices.push_back(vertex);
        }
    }
    
    // Generate sphere indices
    for (int ring = 0; ring < rings; ++ring) {
        for (int sector = 0; sector < sectors; ++sector) {
            int current = ring * (sectors + 1) + sector;
            int next = current + sectors + 1;
            
            sphereIndices.push_back(current);
            sphereIndices.push_back(next);
            sphereIndices.push_back(current + 1);
            
            sphereIndices.push_back(current + 1);
            sphereIndices.push_back(next);
            sphereIndices.push_back(next + 1);
        }
    }
    
    // Create vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(Vertex) * sphereVertices.size();
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = sphereVertices.data();
    
    device_->CreateBuffer(&bufferDesc, &vertexData, &sphereVertexBuffer_);
    
    // Create index buffer
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * sphereIndices.size();
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = sphereIndices.data();
    
    device_->CreateBuffer(&bufferDesc, &indexData, &sphereIndexBuffer_);
    
    sphereIndexCount_ = sphereIndices.size();
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
    
    HRESULT hr = device_->CreateBuffer(&bufferDesc, nullptr, &constantBuffer_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create constant buffer");
    }
}

void GraphicsDevice::SetupBasicCamera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up) {
    // Set up view matrix
    DirectX::XMVECTOR eyePos = DirectX::XMVectorSet(position.x, position.y, position.z, 1.0f);
    DirectX::XMVECTOR lookAt = DirectX::XMVectorSet(target.x, target.y, target.z, 1.0f);
    DirectX::XMVECTOR upVec = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);
    
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eyePos, lookAt, upVec);
    DirectX::XMStoreFloat4x4(&viewMatrix_, view);
    
    // Set up projection matrix
    DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, (float)width_ / (float)height_, 0.1f, 1000.0f);
    DirectX::XMStoreFloat4x4(&projectionMatrix_, projection);
}

void GraphicsDevice::RenderBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& size, const DirectX::XMFLOAT4& color) {
    if (!boxVertexBuffer_ || !boxIndexBuffer_ || !basicVertexShader_ || !basicPixelShader_) return;
    
    // Setup world matrix
    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(size.x, size.y, size.z) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    
    // Update constant buffer
    ConstantBufferData cbData;
    DirectX::XMStoreFloat4x4(&cbData.world, world);
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

void GraphicsDevice::RenderSphere(const DirectX::XMFLOAT3& position, float radius, const DirectX::XMFLOAT4& color) {
    if (!sphereVertexBuffer_ || !sphereIndexBuffer_ || !basicVertexShader_ || !basicPixelShader_) return;
    
    // Setup world matrix
    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(radius, radius, radius) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    
    // Update constant buffer
    ConstantBufferData cbData;
    DirectX::XMStoreFloat4x4(&cbData.world, world);
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
    context_->DrawIndexed(sphereIndexCount_, 0, 0);
}

void GraphicsDevice::RenderCapsule(const DirectX::XMFLOAT3& position, float radius, float height, const DirectX::XMFLOAT4& color) {
    // For now, just render as a stretched sphere
    RenderSphere(position, radius, color);
}

void GraphicsDevice::InitializePostProcessing() {
    Logger::Info("Initializing post-processing effects...");
    
    // Create bloom render target
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width_ / 2;  // Half resolution for bloom
    textureDesc.Height = height_ / 2;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    HRESULT hr = device_->CreateTexture2D(&textureDesc, nullptr, &bloomTexture_);
    if (SUCCEEDED(hr)) {
        device_->CreateRenderTargetView(bloomTexture_, nullptr, &bloomRenderTarget_);
        Logger::Info("Bloom render target created successfully");
    }

    // Create heat haze render target
    textureDesc.Width = width_;
    textureDesc.Height = height_;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &heatHazeTexture_);
    if (SUCCEEDED(hr)) {
        device_->CreateRenderTargetView(heatHazeTexture_, nullptr, &heatHazeRenderTarget_);
        Logger::Info("Heat haze render target created successfully");
    }

    // Create shadow map
    textureDesc.Width = shadowMapSize_;
    textureDesc.Height = shadowMapSize_;
    textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    
    hr = device_->CreateTexture2D(&textureDesc, nullptr, &shadowMap_);
    if (SUCCEEDED(hr)) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        
        device_->CreateDepthStencilView(shadowMap_, &dsvDesc, &shadowMapDepth_);
        Logger::Info("Shadow map created successfully");
    }
}

void GraphicsDevice::RenderBloomPass() {
    if (!bloomRenderTarget_ || !bloomTexture_) return;

    // Set bloom render target
    context_->OMSetRenderTargets(1, &bloomRenderTarget_, nullptr);
    
    // Clear bloom buffer
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->ClearRenderTargetView(bloomRenderTarget_, clearColor);
    
    // Set viewport for bloom (half resolution)
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)width_ / 2;
    viewport.Height = (float)height_ / 2;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    
    // Render bright pixels only
    // This would typically use a bloom shader that extracts bright pixels
    // For now, we'll just clear the buffer
    
    // Restore main render target
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
    
    // Restore full viewport
    viewport.Width = (float)width_;
    viewport.Height = (float)height_;
    context_->RSSetViewports(1, &viewport);
    
    Logger::Debug("Bloom pass completed");
}

void GraphicsDevice::RenderHeatHazePass() {
    if (!heatHazeRenderTarget_ || !heatHazeTexture_) return;

    // Set heat haze render target
    context_->OMSetRenderTargets(1, &heatHazeRenderTarget_, nullptr);
    
    // Clear heat haze buffer
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context_->ClearRenderTargetView(heatHazeRenderTarget_, clearColor);
    
    // Apply heat haze distortion effect
    // This would typically use a noise texture and distortion shader
    // For now, we'll just prepare the buffer
    
    // Restore main render target
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
    
    Logger::Debug("Heat haze pass completed");
}

void GraphicsDevice::SetBloomEnabled(bool enabled) {
    bloomEnabled_ = enabled;
    Logger::Info("Bloom " + std::string(enabled ? "enabled" : "disabled"));
}

void GraphicsDevice::SetHeatHazeEnabled(bool enabled) {
    heatHazeEnabled_ = enabled;
    Logger::Info("Heat haze " + std::string(enabled ? "enabled" : "disabled"));
}

void GraphicsDevice::SetShadowsEnabled(bool enabled) {
    shadowsEnabled_ = enabled;
    Logger::Info("Shadows " + std::string(enabled ? "enabled" : "disabled"));
}

void GraphicsDevice::SetShadowMapSize(int size) {
    shadowMapSize_ = size;
    Logger::Info("Shadow map size set to " + std::to_string(size));
}

void GraphicsDevice::BeginShadowPass(const Light& light) {
    if (!shadowMapDepth_) return;
    
    // Set shadow map as render target
    context_->OMSetRenderTargets(0, nullptr, shadowMapDepth_);
    
    // Clear shadow map
    context_->ClearDepthStencilView(shadowMapDepth_, D3D11_CLEAR_DEPTH, 1.0f, 0);
    
    // Set shadow map viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)shadowMapSize_;
    viewport.Height = (float)shadowMapSize_;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    
    Logger::Debug("Shadow pass began");
}

void GraphicsDevice::EndShadowPass() {
    // Restore main render target
    context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
    
    // Restore main viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)width_;
    viewport.Height = (float)height_;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    
    Logger::Debug("Shadow pass ended");
}

void GraphicsDevice::RenderMesh(const Mesh& mesh, const Shader& shader) {
    // Implementation for mesh rendering
    Logger::Debug("Rendering mesh with shader");
    
    // This would typically involve:
    // 1. Set vertex/index buffers
    // 2. Set shader
    // 3. Set shader constants
    // 4. Draw
    
    // For now, just a placeholder
}

} // namespace Nexus
