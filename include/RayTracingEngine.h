#pragma once

#include <d3d12.h>
#include <dxr.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <unordered_map>

using namespace DirectX;

namespace Nexus {

/**
 * Advanced ray tracing engine with real-time reflections, global illumination, and shadows
 */
class RayTracingEngine {
public:
    struct RayTracingSettings {
        bool enableReflections = true;
        bool enableGlobalIllumination = true;
        bool enableSoftShadows = true;
        bool enableAmbientOcclusion = true;
        int maxRayDepth = 8;
        int samplesPerPixel = 1;
        float rayBias = 0.001f;
        bool enableDenoising = true;
        bool enableTemporalAccumulation = true;
        float temporalAlpha = 0.9f;
    };

    struct RTGeometry {
        ID3D12Resource* vertexBuffer;
        ID3D12Resource* indexBuffer;
        UINT vertexCount;
        UINT indexCount;
        UINT vertexStride;
        DXGI_FORMAT indexFormat;
        D3D12_RAYTRACING_GEOMETRY_FLAGS flags;
    };

    struct RTInstance {
        int geometryId;
        XMMATRIX transform;
        UINT instanceId;
        UINT instanceMask;
        UINT instanceContributionToHitGroupIndex;
        D3D12_RAYTRACING_INSTANCE_FLAGS flags;
    };

    struct RTMaterial {
        XMFLOAT3 albedo = XMFLOAT3(1, 1, 1);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float emission = 0.0f;
        XMFLOAT3 emissionColor = XMFLOAT3(0, 0, 0);
        int albedoTextureIndex = -1;
        int normalTextureIndex = -1;
        int metallicRoughnessTextureIndex = -1;
        int emissionTextureIndex = -1;
    };

    class RTScene {
    public:
        int AddGeometry(const RTGeometry& geometry);
        void RemoveGeometry(int geometryId);
        int AddInstance(const RTInstance& instance);
        void RemoveInstance(int instanceId);
        void UpdateInstance(int instanceId, const XMMATRIX& transform);
        void SetMaterial(int instanceId, const RTMaterial& material);
        bool BuildAccelerationStructures(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList);

    private:
        std::vector<RTGeometry> geometries_;
        std::vector<RTInstance> instances_;
        std::vector<RTMaterial> materials_;
        std::unordered_map<int, int> instanceToMaterial_;
        
        // Acceleration structures
        ID3D12Resource* topLevelAS_;
        ID3D12Resource* bottomLevelAS_;
        bool needsRebuild_;
    };

    // Global illumination system
    class GlobalIllumination {
    public:
        struct GISettings {
            int numBounces = 4;
            float indirectIntensity = 1.0f;
            bool enableCaustics = false;
            bool enableVolumetrics = false;
            float volumetricDensity = 0.1f;
        };

        void SetSettings(const GISettings& settings) { settings_ = settings; }
        void ComputeGlobalIllumination(ID3D12GraphicsCommandList4* commandList, 
                                     const RTScene& scene, Camera* camera);

    private:
        GISettings settings_;
        ID3D12Resource* giTexture_;
        ID3D12Resource* irradianceCache_;
    };

    // Denoising system
    class Denoiser {
    public:
        enum class DenoiseType {
            Reflections,
            GlobalIllumination,
            Shadows,
            AmbientOcclusion
        };

        bool Initialize(ID3D12Device5* device);
        void Denoise(ID3D12GraphicsCommandList4* commandList, 
                    ID3D12Resource* noisyTexture, ID3D12Resource* denoisedTexture,
                    DenoiseType type);

    private:
        // NVIDIA OptiX or Intel OIDN integration
        void* denoiserHandle_;
        ID3D12Resource* temporaryBuffers_[4];
    };

public:
    RayTracingEngine();
    ~RayTracingEngine();

    bool Initialize(ID3D12Device5* device, ID3D12CommandQueue* commandQueue, 
                   int screenWidth, int screenHeight);
    void Shutdown();
    void Resize(int width, int height);

    // Hardware capability check
    static bool IsRayTracingSupported(ID3D12Device* device);
    static D3D12_RAYTRACING_TIER GetRayTracingTier(ID3D12Device* device);

    // Scene management
    RTScene* CreateScene();
    void DestroyScene(RTScene* scene);
    void SetActiveScene(RTScene* scene);

    // Rendering
    void RenderReflections(ID3D12GraphicsCommandList4* commandList, Camera* camera, 
                          ID3D12Resource* outputTexture);
    void RenderGlobalIllumination(ID3D12GraphicsCommandList4* commandList, Camera* camera,
                                 ID3D12Resource* outputTexture);
    void RenderSoftShadows(ID3D12GraphicsCommandList4* commandList, 
                          const std::vector<Light*>& lights, Camera* camera,
                          ID3D12Resource* outputTexture);
    void RenderAmbientOcclusion(ID3D12GraphicsCommandList4* commandList, Camera* camera,
                               ID3D12Resource* outputTexture);

    // Settings
    void SetRayTracingSettings(const RayTracingSettings& settings);
    const RayTracingSettings& GetRayTracingSettings() const { return settings_; }

    // Global illumination
    GlobalIllumination* GetGISystem() { return globalIllumination_.get(); }

    // Denoising
    Denoiser* GetDenoiser() { return denoiser_.get(); }
    void EnableDenoising(bool enable) { settings_.enableDenoising = enable; }

    // Performance
    void SetQualityLevel(int level); // 0 = Low, 1 = Medium, 2 = High, 3 = Ultra
    float GetLastFrameTime() const { return lastFrameTime_; }

    // Debug and visualization
    void EnableRayVisualization(bool enable) { visualizeRays_ = enable; }
    void SetDebugMode(int mode) { debugMode_ = mode; } // 0 = Off, 1 = Heatmap, 2 = Wireframe
    void RenderDebugInfo();

private:
    bool CreateRootSignatures();
    bool CreateRayTracingPipeline();
    bool CreateShaderBindingTable();
    void UpdateShaderBindingTable();
    void DispatchRays(ID3D12GraphicsCommandList4* commandList, UINT width, UINT height,
                     const wchar_t* rayGenShader);

    ID3D12Device5* device_;
    ID3D12CommandQueue* commandQueue_;
    ID3D12GraphicsCommandList4* commandList_;

    // Ray tracing pipeline
    ID3D12RootSignature* globalRootSignature_;
    ID3D12RootSignature* localRootSignature_;
    ID3D12StateObject* rtPipelineState_;
    ID3D12Resource* shaderBindingTable_;

    // Output textures
    ID3D12Resource* reflectionTexture_;
    ID3D12Resource* giTexture_;
    ID3D12Resource* shadowTexture_;
    ID3D12Resource* aoTexture_;

    // Temporal accumulation
    ID3D12Resource* historyTextures_[4];
    XMMATRIX previousViewMatrix_;
    XMMATRIX previousProjectionMatrix_;

    // Scene
    RTScene* activeScene_;

    // Systems
    std::unique_ptr<GlobalIllumination> globalIllumination_;
    std::unique_ptr<Denoiser> denoiser_;

    // Settings
    RayTracingSettings settings_;
    int screenWidth_, screenHeight_;

    // Performance
    float lastFrameTime_;
    ID3D12QueryHeap* queryHeap_;
    ID3D12Resource* queryResult_;

    // Debug
    bool visualizeRays_;
    int debugMode_;
};

} // namespace Nexus