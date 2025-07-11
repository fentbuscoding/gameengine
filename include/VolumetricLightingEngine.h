#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <map>

using namespace DirectX;

namespace Nexus {

class Camera;
class Light;
class Texture;

/**
 * Advanced Volumetric Lighting Engine with full-screen volumetrics
 * Supports fog, atmospheric scattering, god rays, and particle lighting
 */
class VolumetricLightingEngine {
public:
    enum class VolumetricTechnique {
        RayMarching,
        Billboards,
        VolumeTiles,
        Froxels  // Frustum Voxels
    };

    enum class ScatteringModel {
        Rayleigh,
        Mie,
        HenyeyGreenstein,
        CornetteShanks
    };

    struct VolumetricSettings {
        bool enableVolumetrics = true;
        bool enableAtmosphericScattering = true;
        bool enableGodRays = true;
        bool enableVolumetricFog = true;
        bool enableVolumetricShadows = true;
        
        VolumetricTechnique technique = VolumetricTechnique::RayMarching;
        ScatteringModel scatteringModel = ScatteringModel::HenyeyGreenstein;
        
        float density = 0.02f;
        float scatteringCoeff = 0.8f;
        float absorptionCoeff = 0.1f;
        float anisotropy = 0.3f;
        float extinction = 0.9f;
        
        XMFLOAT3 fogColor = XMFLOAT3(0.6f, 0.7f, 0.8f);
        float fogDensity = 0.015f;
        float fogHeight = 100.0f;
        float fogFalloff = 0.1f;
        
        int rayMarchingSteps = 64;
        int froxelResolutionX = 160;
        int froxelResolutionY = 90;
        int froxelResolutionZ = 64;
        
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        
        // God rays settings
        float godRayDecay = 0.96875f;
        float godRayDensity = 0.926f;
        float godRayWeight = 0.587f;
        float godRayExposure = 0.2f;
        int godRaySamples = 100;
    };

    struct AtmosphereSettings {
        XMFLOAT3 rayleighScattering = XMFLOAT3(0.0054f, 0.0135f, 0.0331f);
        XMFLOAT3 mieScattering = XMFLOAT3(0.004f, 0.004f, 0.004f);
        XMFLOAT3 ozoneAbsorption = XMFLOAT3(0.00065f, 0.00188f, 0.000085f);
        
        float atmosphereRadius = 6420e3f;
        float planetRadius = 6360e3f;
        float rayleighScaleHeight = 8e3f;
        float mieScaleHeight = 1.2e3f;
        float ozoneLayerCenter = 25e3f;
        float ozoneLayerWidth = 15e3f;
        
        float sunIntensity = 20.0f;
        XMFLOAT3 sunDirection = XMFLOAT3(0.0f, 1.0f, 0.2f);
        
        int scatteringLUTSize = 64;
        int transmittanceLUTWidth = 256;
        int transmittanceLUTHeight = 64;
        int multiScatteringLUTSize = 32;
    };

public:
    VolumetricLightingEngine();
    ~VolumetricLightingEngine();

    // Initialization
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int screenWidth, int screenHeight);
    void Shutdown();
    void Resize(int newWidth, int newHeight);

    // Settings
    void SetVolumetricSettings(const VolumetricSettings& settings);
    void SetAtmosphereSettings(const AtmosphereSettings& settings);
    const VolumetricSettings& GetVolumetricSettings() const { return volumetricSettings_; }
    const AtmosphereSettings& GetAtmosphereSettings() const { return atmosphereSettings_; }

    // Rendering
    void BeginFrame(Camera* camera);
    void RenderVolumetrics(const std::vector<std::shared_ptr<Light>>& lights);
    void RenderAtmosphericScattering(const XMFLOAT3& sunDirection);
    void RenderGodRays(const XMFLOAT3& lightPosition, const XMFLOAT3& lightDirection);
    void RenderVolumetricFog();
    void RenderVolumetricShadows(const std::vector<std::shared_ptr<Light>>& lights);
    void EndFrame();

    // Advanced features
    void AddVolumetricParticleSystem(const XMFLOAT3& position, float radius, float density);
    void UpdateParticleSystems(float deltaTime);
    void SetTimeOfDay(float hours); // 0-24
    void SetWeatherConditions(float humidity, float pollution, float cloudCoverage);

    // Performance optimization
    void SetTemporalUpsampling(bool enable);
    void SetAdaptiveQuality(bool enable);
    void SetFroxelCaching(bool enable);

private:
    // Core rendering
    bool CreateRenderTargets();
    bool CreateShaders();
    bool CreateBuffers();
    bool CreateSamplers();
    void CreateLookupTables();
    
    // Froxel management
    void InitializeFroxels();
    void UpdateFroxels(Camera* camera);
    void RenderFroxelLighting(const std::vector<std::shared_ptr<Light>>& lights);
    
    // Ray marching
    void RenderRayMarchedVolumetrics(const std::vector<std::shared_ptr<Light>>& lights);
    void PerformRayMarching(Camera* camera, const XMFLOAT3& lightPos, const XMFLOAT3& lightColor);
    
    // Atmospheric scattering
    void PrecomputeAtmosphere();
    void GenerateTransmittanceLUT();
    void GenerateScatteringLUT();
    void GenerateMultiScatteringLUT();
    void RenderSkybox();
    
    // God rays implementation
    void ExtractLightShafts();
    void BlurLightShafts();
    void CompositeGodRays();
    
    // Temporal optimization
    void PerformTemporalUpsampling();
    void UpdateTemporalData();
    
    // Utilities
    float ComputeScattering(float cosTheta, float g);
    XMFLOAT3 ComputeInScattering(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDirection, float rayLength);
    float ComputePhaseFunction(float cosTheta, float g);
    
private:
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    VolumetricSettings volumetricSettings_;
    AtmosphereSettings atmosphereSettings_;
    
    int screenWidth_;
    int screenHeight_;
    
    // Render targets
    ID3D11Texture2D* volumetricTexture_;
    ID3D11RenderTargetView* volumetricRTV_;
    ID3D11ShaderResourceView* volumetricSRV_;
    
    ID3D11Texture2D* froxelTexture_;
    ID3D11RenderTargetView* froxelRTV_;
    ID3D11ShaderResourceView* froxelSRV_;
    ID3D11UnorderedAccessView* froxelUAV_;
    
    ID3D11Texture2D* godRaysTexture_;
    ID3D11RenderTargetView* godRaysRTV_;
    ID3D11ShaderResourceView* godRaysSRV_;
    
    ID3D11Texture2D* atmosphereTexture_;
    ID3D11RenderTargetView* atmosphereRTV_;
    ID3D11ShaderResourceView* atmosphereSRV_;
    
    // Lookup tables
    ID3D11Texture2D* transmittanceLUT_;
    ID3D11ShaderResourceView* transmittanceSRV_;
    ID3D11Texture3D* scatteringLUT_;
    ID3D11ShaderResourceView* scatteringSRV_;
    ID3D11Texture2D* multiScatteringLUT_;
    ID3D11ShaderResourceView* multiScatteringSRV_;
    
    // Temporal data
    ID3D11Texture2D* previousFrameTexture_;
    ID3D11ShaderResourceView* previousFrameSRV_;
    ID3D11Texture2D* motionVectorTexture_;
    ID3D11ShaderResourceView* motionVectorSRV_;
    
    // Constant buffers
    ID3D11Buffer* volumetricConstantBuffer_;
    ID3D11Buffer* atmosphereConstantBuffer_;
    ID3D11Buffer* godRaysConstantBuffer_;
    ID3D11Buffer* froxelConstantBuffer_;
    
    // Shaders
    std::map<std::string, ID3D11ComputeShader*> computeShaders_;
    std::map<std::string, ID3D11VertexShader*> vertexShaders_;
    std::map<std::string, ID3D11PixelShader*> pixelShaders_;
    
    // Samplers
    ID3D11SamplerState* linearSampler_;
    ID3D11SamplerState* pointSampler_;
    ID3D11SamplerState* atmosphereSampler_;
    
    // Performance tracking
    bool temporalUpsamplingEnabled_;
    bool adaptiveQualityEnabled_;
    bool froxelCachingEnabled_;
    float adaptiveQualityFactor_;
    
    // Particle systems
    struct VolumetricParticle {
        XMFLOAT3 position;
        float radius;
        float density;
        float lifetime;
        XMFLOAT3 velocity;
    };
    std::vector<VolumetricParticle> particles_;
    
    // Weather simulation
    float currentHumidity_;
    float currentPollution_;
    float currentCloudCoverage_;
    float timeOfDay_;
};

} // namespace Nexus