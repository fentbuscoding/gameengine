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
    void Clear(const XMFLOAT4& color);
    void SetViewport(int x, int y, int width, int height);

    // Render states
    void SetSamplerState(int stage, D3D11_SAMPLER_DESC& desc);
    void SetBlendState(ID3D11BlendState* blendState);
    void SetDepthStencilState(ID3D11DepthStencilState* depthStencilState);
    void SetRasterizerState(ID3D11RasterizerState* rasterizerState);

    // Resource management
    bool CreateBuffer(const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA* initData, ID3D11Buffer** buffer);
    bool CreateTexture2D(const D3D11_TEXTURE2D_DESC& desc, const D3D11_SUBRESOURCE_DATA* initData, ID3D11Texture2D** texture);
    bool CreateShaderResourceView(ID3D11Resource* resource, const D3D11_SHADER_RESOURCE_VIEW_DESC* desc, ID3D11ShaderResourceView** srv);
    bool CreateRenderTargetView(ID3D11Resource* resource, const D3D11_RENDER_TARGET_VIEW_DESC* desc, ID3D11RenderTargetView** rtv);
    bool CreateDepthStencilView(ID3D11Resource* resource, const D3D11_DEPTH_STENCIL_VIEW_DESC* desc, ID3D11DepthStencilView** dsv);

    // Shader management
    bool CreateVertexShader(const void* bytecode, size_t bytecodeLength, ID3D11VertexShader** shader);
    bool CreatePixelShader(const void* bytecode, size_t bytecodeLength, ID3D11PixelShader** shader);
    bool CreateGeometryShader(const void* bytecode, size_t bytecodeLength, ID3D11GeometryShader** shader);
    bool CreateHullShader(const void* bytecode, size_t bytecodeLength, ID3D11HullShader** shader);
    bool CreateDomainShader(const void* bytecode, size_t bytecodeLength, ID3D11DomainShader** shader);
    bool CreateComputeShader(const void* bytecode, size_t bytecodeLength, ID3D11ComputeShader** shader);

    // Input layout
    bool CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* elements, UINT numElements, 
                          const void* bytecode, size_t bytecodeLength, ID3D11InputLayout** layout);

    // Rendering pipeline
    void SetVertexShader(ID3D11VertexShader* shader);
    void SetPixelShader(ID3D11PixelShader* shader);
    void SetGeometryShader(ID3D11GeometryShader* shader);
    void SetHullShader(ID3D11HullShader* shader);
    void SetDomainShader(ID3D11DomainShader* shader);
    void SetComputeShader(ID3D11ComputeShader* shader);
    void SetInputLayout(ID3D11InputLayout* layout);

    // Drawing
    void DrawIndexed(UINT indexCount, UINT startIndexLocation, INT baseVertexLocation);
    void Draw(UINT vertexCount, UINT startVertexLocation);
    void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
    void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation);

    // Compute shaders
    void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);

    // Resource binding
    void SetVertexBuffers(UINT startSlot, UINT numBuffers, ID3D11Buffer* const* buffers, const UINT* strides, const UINT* offsets);
    void SetIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT format, UINT offset);
    void SetVertexShaderConstantBuffers(UINT startSlot, UINT numBuffers, ID3D11Buffer* const* buffers);
    void SetPixelShaderConstantBuffers(UINT startSlot, UINT numBuffers, ID3D11Buffer* const* buffers);
    void SetVertexShaderResources(UINT startSlot, UINT numViews, ID3D11ShaderResourceView* const* views);
    void SetPixelShaderResources(UINT startSlot, UINT numViews, ID3D11ShaderResourceView* const* views);
    void SetVertexShaderSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState* const* samplers);
    void SetPixelShaderSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState* const* samplers);

    // Render targets
    void SetRenderTargets(UINT numViews, ID3D11RenderTargetView* const* renderTargetViews, ID3D11DepthStencilView* depthStencilView);

    // Feature support
    bool CheckFeatureSupport(D3D11_FEATURE feature, void* featureSupportData, UINT featureSupportDataSize);
    D3D_FEATURE_LEVEL GetFeatureLevel() const { return featureLevel_; }

    // Debug and profiling
    void BeginEvent(const std::string& name);
    void EndEvent();
    void SetMarker(const std::string& name);

    // High-level rendering methods
    void RenderMesh(const Mesh& mesh, const Shader& shader);
    
    // Physics object rendering
    void RenderBox(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    void RenderSphere(const XMFLOAT3& position, float radius, const XMFLOAT4& color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    void RenderCapsule(const XMFLOAT3& position, float radius, float height, const XMFLOAT4& color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    void SetupBasicCamera(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up);
    
    // Texture loading (supports Unreal Engine formats)
    bool LoadTexture(const std::string& filePath, ID3D11ShaderResourceView** textureView);
    bool LoadUnrealTexture(const std::string& filePath, ID3D11ShaderResourceView** textureView);
    bool LoadDDSTexture(const std::string& filePath, ID3D11ShaderResourceView** textureView);
    bool LoadTGATexture(const std::string& filePath, ID3D11ShaderResourceView** textureView);
    bool LoadBMPTexture(const std::string& filePath, ID3D11ShaderResourceView** textureView);
    
    // Asset loading
    bool LoadUnrealAsset(const std::string& filePath);
    bool LoadFBX(const std::string& filePath);
    bool LoadOBJ(const std::string& filePath);
    
    // Post-processing effects
    void SetBloomEnabled(bool enabled);
    void SetHeatHazeEnabled(bool enabled);
    void SetShadowsEnabled(bool enabled);
    
    // Shadow mapping
    void SetShadowMapSize(int size);
    void BeginShadowPass(const Light& light);
    void EndShadowPass();
    
    // Device management
    bool IsDeviceLost() const;
    bool ResetDevice();

private:
    // Core D3D11 objects
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    IDXGISwapChain* swapChain_;
    ID3D11RenderTargetView* backBufferRTV_;
    ID3D11RenderTargetView* renderTargetView_; // Alias for compatibility
    ID3D11DepthStencilView* depthStencilView_;
    ID3D11Texture2D* depthStencilBuffer_;

    // Device capabilities
    D3D_FEATURE_LEVEL featureLevel_;
    D3D11_FEATURE_DATA_THREADING threadingSupport_;
    D3D11_FEATURE_DATA_DOUBLES doubleSupport_;
    D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT minPrecisionSupport_;

    // Render states
    ID3D11RasterizerState* defaultRasterizerState_;
    ID3D11BlendState* defaultBlendState_;
    ID3D11DepthStencilState* defaultDepthStencilState_;
    ID3D11SamplerState* defaultSamplerState_;

    // Screen dimensions
    int screenWidth_;
    int screenHeight_;
    int width_, height_; // Compatibility aliases
    bool isFullscreen_;
    bool fullscreen_; // Compatibility alias

    // Debug interface
    ID3D11Debug* debug_;
    void* annotation_; // Use void* to avoid forward declaration issues

    // Helper methods
    bool CreateDeviceAndSwapChain(HWND windowHandle, int width, int height, bool fullscreen);
    bool CreateBackBufferRTV();
    bool CreateDepthStencilBuffer(int width, int height);
    bool CreateDefaultRenderStates();
    void CleanupDeviceResources();

    // Feature detection
    void DetectFeatureSupport();
    bool IsFeatureSupported(D3D11_FEATURE feature);

    // Error handling
    void CheckDeviceRemoved();
    std::string GetHRESULTString(HRESULT hr);

    // Post-processing and effects
    void InitializePostProcessing();
    void InitializeShadowMapping();
    void RenderBloomPass();
    void RenderHeatHazePass();
    void CreateDefaultShaders();

    // Post-processing
    bool bloomEnabled_;
    bool heatHazeEnabled_;
    float bloomThreshold_;
    float bloomIntensity_;
    
    ID3D11Texture2D* backBuffer_;
    ID3D11Texture2D* bloomTexture_;
    ID3D11RenderTargetView* bloomRenderTarget_;
    ID3D11Texture2D* heatHazeTexture_;
    ID3D11RenderTargetView* heatHazeRenderTarget_;

    // Shadow mapping
    bool shadowsEnabled_;
    int shadowMapSize_;
    
    ID3D11Texture2D* shadowMap_;
    ID3D11DepthStencilView* shadowMapDepth_;

    // Default shaders
    std::unique_ptr<Shader> defaultShader_;
    std::unique_ptr<Shader> normalMapShader_;
    std::unique_ptr<Shader> bloomShader_;
    
    // Basic primitive rendering resources
    ID3D11Buffer* boxVertexBuffer_;
    ID3D11Buffer* boxIndexBuffer_;
    ID3D11Buffer* sphereVertexBuffer_;
    ID3D11Buffer* sphereIndexBuffer_;
    ID3D11Buffer* constantBuffer_;
    ID3D11VertexShader* basicVertexShader_;
    ID3D11PixelShader* basicPixelShader_;
    ID3D11InputLayout* basicInputLayout_;
    
    // Camera matrices
    XMFLOAT4X4 viewMatrix_;
    XMFLOAT4X4 projectionMatrix_;
    
    // Primitive rendering setup
    void InitializePrimitiveRendering();
    void CreateBoxGeometry();
    void CreateSphereGeometry();
    void CreateBasicShaders();
    void CreateConstantBuffer();
    
    // Constant buffer structure
    struct ConstantBufferData {
        XMFLOAT4X4 world;
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
        XMFLOAT4 color;
    };
    
    // Vertex structure
    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT3 normal;
    };
    
    // Sphere generation parameters
    static const int SPHERE_RINGS = 16;
    static const int SPHERE_SECTORS = 32;
};

} // namespace Nexus
