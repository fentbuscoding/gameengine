#pragma once

#include "Platform.h"
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <random>
#include <functional>

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

namespace Nexus {

class Texture;
class Camera;
class Mesh;

/**
 * Advanced particle system with GPU acceleration and complex behaviors
 */
class ParticleSystem {
public:
    enum class ParticleType {
        Point,
        Sprite,
        Mesh,
        Trail,
        Ribbon,
        Volumetric
    };

    enum class EmissionShape {
        Point,
        Sphere,
        Box,
        Cone,
        Circle,
        Mesh,
        Edge,
        Custom
    };

    enum class SimulationSpace {
        Local,
        World
    };

    enum class RenderMode {
        Billboard,
        HorizontalBillboard,
        VerticalBillboard,
        Mesh,
        Stretched,
        VelocityAligned
    };

    enum class BlendMode {
        Opaque,
        Alpha,
        Additive,
        Multiply,
        Subtractive,
        Screen,
        Overlay
    };

    struct Particle {
        XMFLOAT3 position;
        XMFLOAT3 velocity;
        XMFLOAT3 acceleration;
        XMFLOAT4 color;
        XMFLOAT2 size;
        float rotation;
        float angularVelocity;
        float life;
        float maxLife;
        float mass;
        int textureIndex;
        
        // Animation support
        XMFLOAT2 uvOffset;
        XMFLOAT2 uvScale;
        float animationTime;
        int animationFrame;
        
        // Custom data for extended behaviors
        XMFLOAT4 customData;
        
        bool IsAlive() const { return life > 0.0f; }
        float GetNormalizedAge() const { return 1.0f - (life / maxLife); }
    };

    struct ParticleEmitter {
        std::string name;
        bool isActive;
        bool isLooping;
        
        // Transform
        XMFLOAT3 position;
        XMFLOAT3 rotation;
        XMFLOAT3 scale;
        XMMATRIX transform;
        
        // Emission properties
        EmissionShape shape;
        float emissionRate;
        float emissionBurst;
        float emissionDuration;
        float emissionDelay;
        XMFLOAT3 shapeScale;
        
        // Particle properties
        ParticleType particleType;
        int maxParticles;
        float startLifetime;
        float startLifetimeVariation;
        XMFLOAT3 startVelocity;
        XMFLOAT3 startVelocityVariation;
        XMFLOAT4 startColor;
        XMFLOAT4 startColorVariation;
        XMFLOAT2 startSize;
        XMFLOAT2 startSizeVariation;
        float startRotation;
        float startRotationVariation;
        float startAngularVelocity;
        float startMass;
        
        // Over lifetime curves
        std::vector<std::pair<float, float>> velocityOverLifetime;
        std::vector<std::pair<float, XMFLOAT4>> colorOverLifetime;
        std::vector<std::pair<float, XMFLOAT2>> sizeOverLifetime;
        std::vector<std::pair<float, float>> rotationOverLifetime;
        
        // Forces
        XMFLOAT3 gravity;
        float drag;
        float turbulence;
        XMFLOAT3 constantForce;
        
        // Rendering
        RenderMode renderMode;
        BlendMode blendMode;
        std::shared_ptr<Texture> texture;
        std::vector<std::shared_ptr<Texture>> textureSheets;
        XMFLOAT2 textureSheetTiles;
        float textureSheetFrameRate;
        bool useTextureSheetAnimation;
        
        // Collision
        bool enableCollision;
        float bounciness;
        float friction;
        std::vector<XMFLOAT4> collisionPlanes;
        
        // Sub-emitters
        std::vector<std::shared_ptr<ParticleEmitter>> subEmitters;
        
        // Trails
        bool enableTrails;
        float trailWidth;
        float trailLifetime;
        int trailSegments;
        XMFLOAT4 trailColor;
        
        // Noise
        bool enableNoise;
        float noiseStrength;
        float noiseFrequency;
        XMFLOAT3 noiseOffset;
        
        // LOD
        float lodDistance;
        float lodFadeDistance;
        int lodMaxParticles;
        
        // Custom update function
        std::function<void(Particle&, float)> customUpdateFunction;
        
        // Runtime data
        std::vector<Particle> particles;
        std::vector<XMFLOAT3> trailPositions;
        float emissionTimer;
        float systemTime;
        int nextParticleIndex;
        bool hasEmittedBurst;
        
        // Statistics
        int aliveParticleCount;
        int emittedParticleCount;
    };

    struct ParticleForce {
        enum class ForceType {
            Constant,
            Radial,
            Vortex,
            Turbulence,
            Magnetic,
            Custom
        };
        
        ForceType type;
        XMFLOAT3 position;
        XMFLOAT3 direction;
        float strength;
        float radius;
        float falloff;
        bool affectVelocity;
        bool affectPosition;
        
        std::function<XMFLOAT3(const Particle&, const XMFLOAT3&)> customForceFunction;
    };

    struct ParticleCollider {
        enum class ColliderType {
            Plane,
            Sphere,
            Box,
            Capsule,
            Mesh
        };
        
        ColliderType type;
        XMMATRIX transform;
        XMFLOAT3 size;
        float radius;
        float height;
        bool isTrigger;
        float bounciness;
        float friction;
        
        std::shared_ptr<Mesh> collisionMesh;
        
        bool TestCollision(const XMFLOAT3& position, float radius) const;
        XMFLOAT3 GetCollisionResponse(const XMFLOAT3& position, 
                                     const XMFLOAT3& velocity) const;
    };

    // GPU particle system for high-performance rendering
    struct GPUParticleSystem {
        ID3D11Buffer* vertexBuffer;
        ID3D11Buffer* indexBuffer;
        ID3D11Texture2D* particleTexture;
        ID3D11ShaderResourceView* particleTextureSRV;
        ID3D11InputLayout* inputLayout;
        
        // Compute shader for particle updates (if supported)
        ID3D11ComputeShader* updateComputeShader;
        ID3D11VertexShader* renderVertexShader;
        ID3D11PixelShader* renderPixelShader;
        
        // Buffer for compute shader data
        ID3D11Buffer* computeBuffer;
        ID3D11ShaderResourceView* computeBufferSRV;
        ID3D11UnorderedAccessView* computeBufferUAV;
        
        // Vertex buffer streaming
        void* mappedVertices;
        int maxVertices;
        int currentVertexCount;
        
        bool useGPUSimulation;
        bool useInstancing;
        
        void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int maxParticles);
        void UpdateGPUBuffer(const std::vector<Particle>& particles);
        void Render(ID3D11DeviceContext* context, Camera* camera);
        void Cleanup();
    };

    // Particle system manager
    struct ParticleSystemManager {
        std::map<std::string, std::shared_ptr<ParticleEmitter>> emitters;
        std::vector<ParticleForce> globalForces;
        std::vector<ParticleCollider> colliders;
        
        // Performance settings
        int maxParticlesPerSystem;
        int maxTotalParticles;
        float updateFrequency;
        bool useMultithreading;
        
        // LOD settings
        float lodNearDistance;
        float lodFarDistance;
        float lodCullingDistance;
        
        // Statistics
        int totalParticles;
        int activeEmitters;
        float lastUpdateTime;
        
        void Update(float deltaTime);
        void Render(ID3D11DeviceContext* context, Camera* camera);
        void AddGlobalForce(const ParticleForce& force);
        void AddCollider(const ParticleCollider& collider);
        void SetLODSettings(float nearDist, float farDist, float cullDist);
    };

public:
    ParticleSystem();
    ~ParticleSystem();

    // Initialization
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Emitter management
    std::shared_ptr<ParticleEmitter> CreateEmitter(const std::string& name);
    void RemoveEmitter(const std::string& name);
    std::shared_ptr<ParticleEmitter> GetEmitter(const std::string& name);
    void ClearEmitters();

    // Emitter control
    void StartEmission(const std::string& name);
    void StopEmission(const std::string& name);
    void BurstEmission(const std::string& name, int count);
    void SetEmitterPosition(const std::string& name, const XMFLOAT3& position);
    void SetEmitterActive(const std::string& name, bool active);

    // Particle effects presets
    void CreateFireEffect(const std::string& name, const XMFLOAT3& position);
    void CreateSmokeEffect(const std::string& name, const XMFLOAT3& position);
    void CreateExplosionEffect(const std::string& name, const XMFLOAT3& position);
    void CreateSparkEffect(const std::string& name, const XMFLOAT3& position);
    void CreateMagicEffect(const std::string& name, const XMFLOAT3& position);
    void CreateBloodEffect(const std::string& name, const XMFLOAT3& position);
    void CreateRainEffect(const std::string& name, const XMFLOAT3& position);
    void CreateSnowEffect(const std::string& name, const XMFLOAT3& position);

    // Global forces
    void AddGlobalForce(const ParticleForce& force);
    void RemoveGlobalForce(int index);
    void ClearGlobalForces();

    // Collision
    void AddCollider(const ParticleCollider& collider);
    void RemoveCollider(int index);
    void ClearColliders();

    // Performance and LOD
    void SetMaxParticles(int maxParticles);
    void SetUpdateFrequency(float frequency);
    void EnableLOD(bool enable);
    void SetLODDistances(float nearDist, float farDist, float cullDist);
    void EnableMultithreading(bool enable);
    void EnableGPUSimulation(bool enable);

    // Rendering
    void SetSortParticles(bool sort);
    void SetDepthTesting(bool enable);
    void SetDepthWriting(bool enable);
    void SetCulling(bool enable);

    // Update and render
    void Update(float deltaTime);
    void Render(Camera* camera);

    // Utility functions
    void WarmupEmitter(const std::string& name, float time);
    void PauseEmitter(const std::string& name, bool pause);
    void ResetEmitter(const std::string& name);

    // Statistics
    int GetTotalParticleCount() const;
    int GetActiveEmitterCount() const;
    float GetLastUpdateTime() const;
    void GetStatistics(int& totalParticles, int& activeEmitters, 
                      float& updateTime) const;

    // Debug visualization
    void EnableDebugVisualization(bool enable);
    void DrawEmitterBounds(const std::string& name);
    void DrawParticleVelocities(const std::string& name);
    void DrawColliders();

    // Save/Load particle system configurations
    bool SaveEmitterConfig(const std::string& name, const std::string& filePath);
    bool LoadEmitterConfig(const std::string& name, const std::string& filePath);

private:
    // Core particle simulation
    void UpdateEmitter(std::shared_ptr<ParticleEmitter> emitter, float deltaTime);
    void UpdateParticle(Particle& particle, float deltaTime);
    void EmitParticle(std::shared_ptr<ParticleEmitter> emitter);
    void ApplyForces(Particle& particle, float deltaTime);
    void HandleCollisions(Particle& particle);
    void UpdateTrails(std::shared_ptr<ParticleEmitter> emitter);

    // Interpolation and curves
    float InterpolateFloat(const std::vector<std::pair<float, float>>& curve, 
                          float time) const;
    D3DXVECTOR4 InterpolateColor(const std::vector<std::pair<float, D3DXVECTOR4>>& curve, 
                                float time) const;
    D3DXVECTOR2 InterpolateSize(const std::vector<std::pair<float, D3DXVECTOR2>>& curve, 
                               float time) const;

    // Emission shapes
    D3DXVECTOR3 GetEmissionPoint(const ParticleEmitter& emitter) const;
    D3DXVECTOR3 GetEmissionDirection(const ParticleEmitter& emitter, 
                                    const D3DXVECTOR3& position) const;

    // Rendering helpers
    void RenderParticles(std::shared_ptr<ParticleEmitter> emitter, Camera* camera);
    void RenderTrails(std::shared_ptr<ParticleEmitter> emitter, Camera* camera);
    void SetupRenderState(const ParticleEmitter& emitter);
    void SortParticles(std::shared_ptr<ParticleEmitter> emitter, Camera* camera);

    // LOD and culling
    float CalculateLODFactor(const ParticleEmitter& emitter, Camera* camera) const;
    bool ShouldCullEmitter(const ParticleEmitter& emitter, Camera* camera) const;
    void ApplyLOD(std::shared_ptr<ParticleEmitter> emitter, float lodFactor);

    // Random number generation
    float RandomFloat(float min, float max);
    D3DXVECTOR3 RandomVector3(const D3DXVECTOR3& min, const D3DXVECTOR3& max);
    D3DXVECTOR4 RandomColor(const D3DXVECTOR4& base, const D3DXVECTOR4& variation);

    // Noise functions
    float PerlinNoise(float x, float y, float z);
    D3DXVECTOR3 NoiseVector(const D3DXVECTOR3& position, float frequency);

private:
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    
    // Particle system manager
    std::unique_ptr<ParticleSystemManager> manager_;
    
    // GPU particle system
    std::unique_ptr<GPUParticleSystem> gpuSystem_;
    
    // Random number generator
    std::mt19937 randomGenerator_;
    
    // Performance settings
    bool lodEnabled_;
    bool multithreadingEnabled_;
    bool gpuSimulationEnabled_;
    bool sortParticles_;
    bool depthTesting_;
    bool depthWriting_;
    bool cullingEnabled_;
    
    // Debug settings
    bool debugVisualization_;
    
    // Render states
    DWORD savedAlphaBlend_;
    DWORD savedSrcBlend_;
    DWORD savedDestBlend_;
    DWORD savedZWrite_;
    DWORD savedZFunc_;
    DWORD savedCullMode_;
    
    // Statistics
    mutable int totalParticles_;
    mutable int activeEmitters_;
    mutable float lastUpdateTime_;
    
    // Threading
    std::vector<std::thread> workerThreads_;
    std::mutex emitterMutex_;
};

} // namespace Nexus
