#pragma once

#include <d3d11.h>
#include <d3d12.h>
#include <dxr.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <thread>
#include <queue>
#include <mutex>

using namespace DirectX;

namespace Nexus {

/**
 * Advanced Texture Streaming Engine with GPU decompression and intelligent caching
 */
class TextureStreamingEngine {
public:
    enum class CompressionFormat {
        None,
        BC1,    // DXT1
        BC3,    // DXT5
        BC4,    // Single channel
        BC5,    // Normal maps
        BC6H,   // HDR
        BC7,    // High quality
        ASTC,   // Mobile
        ETC2,   // Mobile
        Custom
    };

    enum class StreamingPriority {
        Critical,   // UI, immediate viewport
        High,       // Near viewport objects
        Medium,     // Medium distance objects
        Low,        // Far distance objects
        Background  // Preloading
    };

    struct TextureDesc {
        std::string filePath;
        int width = 0;
        int height = 0;
        int mipLevels = 0;
        CompressionFormat format = CompressionFormat::BC7;
        bool sRGB = false;
        bool generateMips = true;
        bool streaming = true;
        int maxResidentMips = 8;
        StreamingPriority priority = StreamingPriority::Medium;
        float distanceFromCamera = 0.0f;
        uint64_t estimatedMemoryUsage = 0;
    };

    struct StreamingStats {
        uint64_t totalTextureMemory = 0;
        uint64_t streamingPoolSize = 0;
        uint64_t residentMemory = 0;
        int pendingLoads = 0;
        int texturesInFlight = 0;
        float hitRate = 0.0f;
        float avgLoadTime = 0.0f;
        int droppedFrames = 0;
    };

public:
    TextureStreamingEngine();
    ~TextureStreamingEngine();

    // Initialization
    bool Initialize(ID3D11Device* device, uint64_t poolSizeBytes = 512 * 1024 * 1024);
    void Shutdown();

    // Texture Management
    uint32_t LoadTexture(const std::string& filePath, const TextureDesc& desc = TextureDesc{});
    void UnloadTexture(uint32_t textureId);
    ID3D11ShaderResourceView* GetTextureSRV(uint32_t textureId);
    bool IsTextureResident(uint32_t textureId, int mipLevel = 0);

    // Streaming Control
    void SetStreamingPriority(uint32_t textureId, StreamingPriority priority);
    void RequestMipLevel(uint32_t textureId, int mipLevel);
    void PreloadTexture(uint32_t textureId);
    void EvictTexture(uint32_t textureId);

    // Camera-based LOD
    void UpdateCameraPosition(const XMFLOAT3& position, const XMFLOAT3& direction);
    void RegisterTextureUsage(uint32_t textureId, const XMFLOAT3& worldPosition, 
                             float screenSize);

    // Memory Management
    void SetMemoryBudget(uint64_t budgetBytes);
    uint64_t GetMemoryUsage() const;
    void TrimMemory(uint64_t targetBytes);
    void SetLRUEnabled(bool enabled) { lruEnabled_ = enabled; }

    // Compression and Decompression
    void SetGPUDecompressionEnabled(bool enabled);
    bool CompressTexture(const std::string& inputPath, const std::string& outputPath,
                        CompressionFormat format);
    void SetCompressionQuality(int quality); // 0-100

    // Update
    void Update(float deltaTime);
    void ProcessAsyncOperations();

    // Statistics
    StreamingStats GetStreamingStats() const { return stats_; }
    void ResetStatistics();

    // Debug
    void EnableDebugOverlay(bool enable) { debugOverlayEnabled_ = enable; }
    std::vector<TextureDebugInfo> GetDebugInfo();

private:
    struct StreamingTexture {
        TextureDesc desc;
        ID3D11Texture2D* texture = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        std::vector<bool> residentMips;
        uint64_t lastAccessTime = 0;
        StreamingPriority priority = StreamingPriority::Medium;
        bool loadInProgress = false;
        float distanceFromCamera = 0.0f;
        uint64_t memoryUsage = 0;
    };

    struct LoadRequest {
        uint32_t textureId;
        int mipLevel;
        StreamingPriority priority;
        uint64_t requestTime;
    };

    // Core streaming
    void ProcessLoadQueue();
    void LoadTextureMip(uint32_t textureId, int mipLevel);
    void UnloadTextureMip(uint32_t textureId, int mipLevel);
    
    // Memory management
    void EnforceMemoryBudget();
    uint32_t FindLRUTexture();
    void UpdateLRU(uint32_t textureId);

    // GPU decompression
    void InitializeGPUDecompression();
    void DecompressTextureGPU(uint32_t textureId, int mipLevel);

    // File I/O
    bool LoadTextureFromFile(const std::string& filePath, TextureDesc& desc, 
                           std::vector<uint8_t>& data);
    void SaveCompressedTexture(const std::string& filePath, const TextureDesc& desc,
                              const std::vector<uint8_t>& data);

    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    
    // Texture storage
    std::unordered_map<uint32_t, std::unique_ptr<StreamingTexture>> textures_;
    uint32_t nextTextureId_;
    
    // Streaming management
    std::priority_queue<LoadRequest> loadQueue_;
    std::mutex loadQueueMutex_;
    std::vector<std::thread> loaderThreads_;
    std::atomic<bool> shutdownRequested_;
    
    // Memory management
    uint64_t memoryBudget_;
    uint64_t currentMemoryUsage_;
    bool lruEnabled_;
    std::vector<uint32_t> lruList_;
    
    // Camera tracking
    XMFLOAT3 cameraPosition_;
    XMFLOAT3 cameraDirection_;
    
    // GPU decompression
    bool gpuDecompressionEnabled_;
    ID3D11ComputeShader* decompressionCS_;
    ID3D11Buffer* decompressionConstants_;
    
    // Statistics
    StreamingStats stats_;
    bool debugOverlayEnabled_;
    
    // Performance settings
    int maxLoaderThreads_;
    int maxLoadRequestsPerFrame_;
    float mipBias_;
};

/**
 * Ray Tracing Engine for NVIDIA RTX support
 */
class RayTracingEngine {
public:
    enum class RayTracingAPI {
        DXR,        // DirectX Raytracing
        Vulkan,     // Vulkan RT
        OptiX,      // NVIDIA OptiX
        Software    // Software fallback
    };

    struct RayTracingSettings {
        RayTracingAPI api = RayTracingAPI::DXR;
        bool enableGlobalIllumination = true;
        bool enableReflections = true;
        bool enableShadows = true;
        bool enableAmbientOcclusion = true;
        int maxRayDepth = 4;
        int samplesPerPixel = 1;
        float rayTMax = 1000.0f;
        bool enableDenoising = true;
        bool enableTemporalAccumulation = true;
        bool enableAdaptiveSampling = true;
        float adaptiveThreshold = 0.1f;
    };

public:
    RayTracingEngine();
    ~RayTracingEngine();

    // Initialization
    bool Initialize(ID3D12Device5* device, const RayTracingSettings& settings = RayTracingSettings{});
    void Shutdown();

    // Scene Management
    void BuildAccelerationStructures();
    void UpdateAccelerationStructures();
    uint32_t AddMeshToScene(const std::vector<XMFLOAT3>& vertices, 
                           const std::vector<uint32_t>& indices);
    void RemoveMeshFromScene(uint32_t meshId);

    // Ray Tracing Passes
    void RenderGlobalIllumination(ID3D12GraphicsCommandList4* cmdList);
    void RenderReflections(ID3D12GraphicsCommandList4* cmdList);
    void RenderShadows(ID3D12GraphicsCommandList4* cmdList);
    void RenderAmbientOcclusion(ID3D12GraphicsCommandList4* cmdList);

    // Denoising
    void ApplyDenoising(ID3D12Resource* inputTexture, ID3D12Resource* outputTexture);
    void SetDenoisingStrength(float strength);

    // Settings
    void SetSettings(const RayTracingSettings& settings) { settings_ = settings; }
    const RayTracingSettings& GetSettings() const { return settings_; }

    // Performance
    RayTracingStats GetPerformanceStats() const { return stats_; }
    void SetLODEnabled(bool enabled) { lodEnabled_ = enabled; }

private:
    // DXR implementation
    bool InitializeDXR();
    void CreateRayTracingPipeline();
    void CreateShaderTables();

    // Acceleration structures
    void CreateBottomLevelAS();
    void CreateTopLevelAS();
    void UpdateTopLevelAS();

    // Shaders
    void LoadRayTracingShaders();
    void CreateRootSignature();

    ID3D12Device5* device_;
    ID3D12GraphicsCommandList4* commandList_;
    RayTracingSettings settings_;
    
    // Pipeline state
    ID3D12StateObject* rtPipelineState_;
    ID3D12RootSignature* globalRootSignature_;
    ID3D12RootSignature* localRootSignature_;
    
    // Acceleration structures
    ID3D12Resource* bottomLevelAS_;
    ID3D12Resource* topLevelAS_;
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances_;
    
    // Shader tables
    ID3D12Resource* rayGenShaderTable_;
    ID3D12Resource* missShaderTable_;
    ID3D12Resource* hitGroupShaderTable_;
    
    // Scene data
    std::vector<uint32_t> meshIds_;
    uint32_t nextMeshId_;
    
    // Denoising
    void* denoiser_; // OptiX denoiser or similar
    bool denoisingEnabled_;
    
    // Performance tracking
    RayTracingStats stats_;
    bool lodEnabled_;
};

} // namespace Nexus