#pragma once

#include "Platform.h"
#include "Light.h"
#include <vector>
#include <memory>
#include <map>
#include <string>

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

namespace Nexus {

class Camera;
class Mesh;
class Shader;
class Texture;

/**
 * Advanced lighting engine with multiple rendering techniques
 */
class LightingEngine {
public:
    enum class ShadowQuality {
        Low = 512,
        Medium = 1024,
        High = 2048,
        Ultra = 4096
    };

    enum class LightingModel {
        Phong,
        BlinnPhong,
        PBR,
        Toon
    };

    struct LightingSettings {
        LightingModel model = LightingModel::BlinnPhong;
        bool enableShadows = true;
        bool enableSelfShadowing = true;
        bool enableLightBloom = true;
        bool enableHeatHaze = false;
        bool enableNormalMapping = true;
        bool enableSpecularMapping = true;
        ShadowQuality shadowQuality = ShadowQuality::Medium;
        float bloomThreshold = 0.8f;
        float bloomIntensity = 1.0f;
        float ambientIntensity = 0.1f;
        XMFLOAT3 ambientColor = XMFLOAT3(0.2f, 0.2f, 0.3f);
    };

    struct ShadowMap {
        ID3D11Texture2D* texture;
        ID3D11Texture2D* depthTexture;
        ID3D11RenderTargetView* renderTargetView;
        ID3D11DepthStencilView* depthStencilView;
        ID3D11ShaderResourceView* shaderResourceView;
        XMMATRIX lightViewMatrix;
        XMMATRIX lightProjectionMatrix;
        int lightId;
        int size;
        bool isDirty;
        
        // DirectX 9 compatibility
        ID3D11DepthStencilView* depthSurface;
    };

    struct LightCascade {
        std::vector<float> cascadeSplits;
        std::vector<ShadowMap> cascadeMaps;
        int numCascades;
    };

public:
    LightingEngine();
    ~LightingEngine();

    // Initialization
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int screenWidth, int screenHeight);
    void Shutdown();
    
    // Update
    void Update(float deltaTime);

    // Settings
    void SetLightingSettings(const LightingSettings& settings);
    const LightingSettings& GetLightingSettings() const { return settings_; }

    // Light management
    void AddLight(std::shared_ptr<Light> light);
    void AddLight(const Light& light);
    void RemoveLight(std::shared_ptr<Light> light);
    void RemoveLight(int lightId);
    void UpdateLight(int lightId, const Light& light);
    void ClearLights();
    const std::vector<std::shared_ptr<Light>>& GetLights() const { return lights_; }

    // Rendering
    void BeginFrame(Camera* camera);
    void BeginFrame();
    void RenderShadowMaps(Camera* camera, const std::vector<Mesh*>& meshes);
    void RenderLightPass(Camera* camera, const std::vector<Mesh*>& meshes);
    void RenderPostProcess();
    void EndFrame();
    void RenderLight(const Light& light);
    void PerformDeferredLightingPass();
    void ApplyBloomEffect();
    void ApplyHeatHazeEffect();
    void UpdateSelfShadowMaps();

    // Shadow mapping
    void EnableShadowMapping(bool enable);
    void SetShadowQuality(ShadowQuality quality);
    void UpdateShadowMaps(Camera* camera, const std::vector<Mesh*>& meshes);

    // Bloom effects
    void EnableBloom(bool enable);
    void SetBloomParameters(float threshold, float intensity);
    void RenderBloomPass();

    // Heat haze effects
    void EnableHeatHaze(bool enable);
    void SetHeatHazeParameters(float strength, float frequency);
    void RenderHeatHazePass();

    // Normal mapping
    void EnableNormalMapping(bool enable);
    
    // Self-shadowing
    void EnableSelfShadowing(bool enable);

    // Cascaded shadow mapping for directional lights
    void SetupCascadedShadowMaps(int numCascades);
    void UpdateCascadedShadowMaps(Camera* camera, Light* directionalLight);

    // Dynamic lighting
    void SetDynamicLightingEnabled(bool enabled);
    void UpdateDynamicLights(float deltaTime);

    // Light culling for performance
    void CullLights(Camera* camera);
    void SetMaxLightsPerPass(int maxLights);

    // Deferred rendering support
    void EnableDeferredRendering(bool enable);
    void SetupGBuffer();
    void RenderGBuffer(const std::vector<Mesh*>& meshes);
    void RenderDeferredLighting();

    // Volumetric lighting
    void EnableVolumetricLighting(bool enable);
    void RenderVolumetricLighting();

    // SSAO (Screen Space Ambient Occlusion)
    void EnableSSAO(bool enable);
    void RenderSSAO();

private:
    // Core rendering
    bool CreateRenderTargets();
    bool CreateShaders();
    void CreateSamplers();
    void UpdateLightConstants();
    void SetupLightingShaders();

    // Shadow mapping implementation
    void CreateShadowMap(ShadowMap& shadowMap, int size);
    void CreateShadowMap(int lightId, int size);
    void DestroyShadowMap(ShadowMap& shadowMap);
    void RenderShadowMapForLight(Light* light, const std::vector<Mesh*>& meshes);
    void SetupShadowMatrices(Light* light, Camera* camera);

    // Post-processing
    void CreatePostProcessTargets();
    void RenderFullScreenQuad();
    void ApplyToneMapping();

    // Light culling
    struct LightFrustum {
        XMFLOAT4 planes[6];
        XMFLOAT3 corners[8];
    };
    void BuildLightFrustum(Light* light, LightFrustum& frustum);
    bool TestLightFrustum(const LightFrustum& frustum, const XMFLOAT3& point, float radius);

    // Deferred rendering
    struct GBuffer {
        ID3D11Texture2D* albedoTexture;
        ID3D11Texture2D* normalTexture;
        ID3D11Texture2D* depthTexture;
        ID3D11Texture2D* materialTexture;
        ID3D11Texture2D* positionTexture;
        ID3D11RenderTargetView* albedoRTV;
        ID3D11RenderTargetView* normalRTV;
        ID3D11RenderTargetView* depthRTV;
        ID3D11RenderTargetView* materialRTV;
        ID3D11RenderTargetView* positionRTV;
        ID3D11ShaderResourceView* albedoSRV;
        ID3D11ShaderResourceView* normalSRV;
        ID3D11ShaderResourceView* depthSRV;
        ID3D11ShaderResourceView* materialSRV;
        ID3D11ShaderResourceView* positionSRV;
        
        // DirectX 9 compatibility surface aliases
        ID3D11RenderTargetView* albedoSurface;
        ID3D11RenderTargetView* normalSurface;
        ID3D11RenderTargetView* depthSurface;
        ID3D11RenderTargetView* materialSurface;
    };

private:
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    LightingSettings settings_;
    
    // Lights
    std::vector<std::shared_ptr<Light>> lights_;
    std::vector<Light> lightsVector_;  // For compatibility with implementation
    std::vector<std::shared_ptr<Light>> culledLights_;
    int maxLightsPerPass_;
    
    // Shadow mapping
    std::map<Light*, ShadowMap> shadowMaps_;
    std::vector<ShadowMap> shadowMapsVector_;  // For compatibility with implementation
    LightCascade cascadedShadowMap_;
    bool shadowMappingEnabled_;
    
    // Render targets
    ID3D11Texture2D* sceneTexture_;
    ID3D11RenderTargetView* sceneRTV_;
    ID3D11ShaderResourceView* sceneSRV_;
    ID3D11Texture2D* bloomTexture_;
    ID3D11RenderTargetView* bloomRTV_;
    ID3D11ShaderResourceView* bloomSRV_;
    ID3D11ShaderResourceView* bloomTextureSRV_;
    ID3D11Texture2D* heatHazeTexture_;
    ID3D11RenderTargetView* heatHazeRTV_;
    ID3D11ShaderResourceView* heatHazeSRV_;
    ID3D11ShaderResourceView* heatHazeTextureSRV_;
    
    // DirectX 9 compatibility surface aliases
    ID3D11RenderTargetView* sceneSurface_;
    ID3D11RenderTargetView* bloomSurface_;
    ID3D11RenderTargetView* heatHazeSurface_;
    ID3D11RenderTargetView* volumetricSurface_;
    ID3D11RenderTargetView* ssaoSurface_;
    
    // Additional compatibility textures and surfaces
    ID3D11Texture2D* normalTexture_;
    ID3D11RenderTargetView* normalSurface_;
    ID3D11Texture2D* depthTexture_;
    ID3D11RenderTargetView* depthSurface_;
    ID3D11Texture2D* shadowTexture_;
    ID3D11RenderTargetView* shadowSurface_;
    ID3D11Texture2D* shadowDepthTexture_;
    ID3D11DepthStencilView* shadowDepthSurface_;
    
    // Shaders
    std::map<std::string, std::shared_ptr<Shader>> shaders_;
    
    // Deferred rendering
    bool deferredRenderingEnabled_;
    GBuffer gBuffer_;
    
    // Screen dimensions
    int screenWidth_;
    int screenHeight_;
    
    // Performance counters
    int lightsRendered_;
    int shadowMapsUpdated_;
    
    // Volumetric lighting
    bool volumetricLightingEnabled_;
    ID3D11Texture2D* volumetricTexture_;
    ID3D11RenderTargetView* volumetricRTV_;
    ID3D11ShaderResourceView* volumetricSRV_;
    
    // SSAO
    bool ssaoEnabled_;
    ID3D11Texture2D* ssaoTexture_;
    ID3D11RenderTargetView* ssaoRTV_;
    ID3D11ShaderResourceView* ssaoSRV_;
    ID3D11Texture2D* ssaoNoiseTexture_;
    ID3D11ShaderResourceView* ssaoNoiseSRV_;
    std::vector<XMFLOAT3> ssaoKernel_;
    
    // Dynamic lighting
    bool dynamicLightingEnabled_;
    float lightAnimationTime_;
    
private:
    // Internal helper methods
    bool CreateShadowMaps();
    bool CreateGBuffer();
    void DestroyGBuffer();
};

} // namespace Nexus
