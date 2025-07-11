#pragma once

#include "Platform.h"
#include <memory>
#include <string>

namespace Nexus {

class Mesh;
class Texture;
class Shader;
class Camera;
class Light;

/**
 * DirectX 11 Graphics Device implementation
 */
class GraphicsDevice {
public:
    GraphicsDevice();
    ~GraphicsDevice();

    // Initialization
    bool Initialize(HWND windowHandle, int width, int height, bool fullscreen = false);
    void Shutdown();

    // Device management
    ID3D11Device* GetDevice() const { return device_; }
    ID3D11DeviceContext* GetContext() const { return context_; }
    IDXGISwapChain* GetSwapChain() const { return swapChain_; }

    // Rendering
    void BeginFrame();
    void EndFrame();
    void Present();
    void Clear(const DirectX::XMFLOAT4& color);
    void SetViewport(int x, int y, int width, int height);
    bool IsDeviceLost();
    bool ResetDevice();

    // Texture loading functions
    ID3D11Texture2D* LoadTexture(const std::string& filename);
    ID3D11Texture2D* LoadUnrealTexture(const std::string& filename);
    ID3D11Texture2D* LoadDDSTexture(const std::string& filename);
    ID3D11Texture2D* LoadTGATexture(const std::string& filename);
    ID3D11Texture2D* LoadBMPTexture(const std::string& filename);

    // Asset loading functions
    bool LoadUnrealAsset(const std::string& filename);
    bool LoadFBX(const std::string& filename);
    bool LoadOBJ(const std::string& filename);

    // Primitive rendering
    void InitializePrimitiveRendering();
    void SetupBasicCamera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);
    void RenderBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& size, const DirectX::XMFLOAT4& color);
    void RenderSphere(const DirectX::XMFLOAT3& position, float radius, const DirectX::XMFLOAT4& color);
    void RenderCapsule(const DirectX::XMFLOAT3& position, float radius, float height, const DirectX::XMFLOAT4& color);

    // Post-processing effects
    void SetBloomEnabled(bool enabled);
    void SetHeatHazeEnabled(bool enabled);
    void SetShadowsEnabled(bool enabled);
    void InitializePostProcessing();
    void RenderBloomPass();
    void RenderHeatHazePass();

    // Shadow mapping
    void SetShadowMapSize(int size);
    void BeginShadowPass(const Light& light);
    void EndShadowPass();

    // High-level rendering
    void RenderMesh(const Mesh& mesh, const Shader& shader);

private:
    // DirectX 11 objects
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    IDXGISwapChain* swapChain_;
    ID3D11RenderTargetView* renderTargetView_;
    ID3D11DepthStencilView* depthStencilView_;

    // Window and display properties
    int width_;
    int height_;
    bool fullscreen_;

    // Feature flags
    bool bloomEnabled_;
    bool heatHazeEnabled_;
    bool shadowsEnabled_;
    float bloomThreshold_;
    float bloomIntensity_;
    int shadowMapSize_;

    // Render target resources
    ID3D11Texture2D* bloomTexture_;
    ID3D11RenderTargetView* bloomRenderTarget_;
    ID3D11Texture2D* heatHazeTexture_;
    ID3D11RenderTargetView* heatHazeRenderTarget_;
    ID3D11Texture2D* shadowMap_;
    ID3D11DepthStencilView* shadowMapDepth_;

    // Primitive rendering resources
    ID3D11Buffer* boxVertexBuffer_;
    ID3D11Buffer* boxIndexBuffer_;
    ID3D11Buffer* sphereVertexBuffer_;
    ID3D11Buffer* sphereIndexBuffer_;
    ID3D11Buffer* constantBuffer_;
    ID3D11VertexShader* basicVertexShader_;
    ID3D11PixelShader* basicPixelShader_;
    ID3D11InputLayout* basicInputLayout_;
    int sphereIndexCount_;

    // Camera matrices
    DirectX::XMFLOAT4X4 viewMatrix_;
    DirectX::XMFLOAT4X4 projectionMatrix_;

    // Helper functions
    void CreateBoxGeometry();
    void CreateSphereGeometry();
    void CreateBasicShaders();
    void CreateConstantBuffer();

    // Structs for rendering
    struct ConstantBufferData {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4 color;
    };

    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
    };
};

} // namespace Nexus
