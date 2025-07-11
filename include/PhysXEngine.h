#pragma once

#include <PxPhysicsAPI.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <map>
#include <functional>

using namespace DirectX;
using namespace physx;

namespace Nexus {

class Mesh;
class Camera;

/**
 * Advanced PhysX-based Physics Engine with GPU acceleration and console support
 * Supports rigid bodies, soft bodies, fluids, particles, and destruction
 */
class PhysXEngine {
public:
    enum class ShapeType {
        Box,
        Sphere,
        Capsule,
        Convex,
        TriangleMesh,
        Heightfield,
        Plane
    };

    enum class MaterialType {
        Default,
        Metal,
        Wood,
        Stone,
        Rubber,
        Ice,
        Mud,
        Sand,
        Water,
        Custom
    };

    enum class SimulationMode {
        CPU,
        GPU,
        Hybrid
    };

    struct PhysicsSettings {
        SimulationMode simulationMode = SimulationMode::GPU;
        bool enableCCD = true; // Continuous Collision Detection
        bool enablePBD = true; // Position Based Dynamics
        bool enableGPUDynamics = true;
        bool enableMultithreading = true;
        
        float timeStep = 1.0f / 60.0f;
        int maxSubSteps = 4;
        int solverIterations = 8;
        int velocityIterations = 1;
        
        XMFLOAT3 gravity = XMFLOAT3(0.0f, -9.81f, 0.0f);
        float sleepThreshold = 0.5f;
        float bounceThreshold = 0.2f;
        
        // GPU settings
        int gpuMaxRigidContactCount = 524288;
        int gpuMaxRigidPatchCount = 81920;
        int gpuHeapCapacity = 64 * 1024 * 1024;
        int gpuTempBufferCapacity = 16 * 1024 * 1024;
        
        // Broad phase settings
        int dynamicTreeRebuildRateHint = 100;
        bool enableStabilization = true;
        bool enableKinematicStaticPairs = false;
        bool enableKinematicKinematicPairs = false;
        
        // Advanced features
        bool enableArticulations = true;
        bool enableSoftBodies = true;
        bool enableParticles = true;
        bool enableFluids = true;
        bool enableDestruction = true;
        bool enableVehicles = true;
        bool enableCharacterController = true;
        
        // Performance optimization
        bool enableAdaptiveForce = true;
        bool enableFrictionEveryIteration = true;
        bool enablePCM = true; // Persistent Contact Manifold
        bool enableTGS = true; // Temporal Gauss Seidel solver
    };

    struct RigidBodyDesc {
        ShapeType shapeType = ShapeType::Box;
        XMFLOAT3 dimensions = XMFLOAT3(1.0f, 1.0f, 1.0f);
        XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
        XMFLOAT4 rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
        XMFLOAT3 angularVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
        
        float mass = 1.0f;
        float density = 1000.0f;
        MaterialType materialType = MaterialType::Default;
        
        bool isKinematic = false;
        bool isTrigger = false;
        bool enableGravity = true;
        bool enableCCD = false;
        
        // Advanced properties
        float linearDamping = 0.1f;
        float angularDamping = 0.05f;
        float maxLinearVelocity = 1000.0f;
        float maxAngularVelocity = 100.0f;
        
        // Collision filtering
        uint32_t collisionGroup = 0xFFFFFFFF;
        uint32_t collisionMask = 0xFFFFFFFF;
        
        // User data
        void* userData = nullptr;
        std::string name;
    };

    struct CollisionEvent {
        PxRigidActor* actor1;
        PxRigidActor* actor2;
        XMFLOAT3 contactPoint;
        XMFLOAT3 contactNormal;
        float impulse;
        float separationDistance;
    };

    struct TriggerEvent {
        PxRigidActor* triggerActor;
        PxRigidActor* otherActor;
        bool isEntering;
    };

    // Callback types
    using CollisionCallback = std::function<void(const CollisionEvent&)>;
    using TriggerCallback = std::function<void(const TriggerEvent&)>;

public:
    PhysXEngine();
    ~PhysXEngine();

    // Initialization
    bool Initialize(const PhysicsSettings& settings = PhysicsSettings{});
    void Shutdown();
    void Reset();

    // Settings
    void SetPhysicsSettings(const PhysicsSettings& settings);
    const PhysicsSettings& GetPhysicsSettings() const { return settings_; }

    // Simulation
    void StepSimulation(float deltaTime);
    void StepSimulation(); // Uses fixed timestep
    void Sync(); // Synchronize GPU results
    
    // Scene management
    void SetGravity(const XMFLOAT3& gravity);
    XMFLOAT3 GetGravity() const;
    void SetTimeStep(float timeStep);
    void PauseSimulation(bool pause);

    // Rigid body management
    PxRigidDynamic* CreateRigidDynamic(const RigidBodyDesc& desc);
    PxRigidStatic* CreateRigidStatic(const RigidBodyDesc& desc);
    void DestroyRigidActor(PxRigidActor* actor);
    
    // Shape creation
    PxShape* CreateBoxShape(const XMFLOAT3& halfExtents, const MaterialType& material = MaterialType::Default);
    PxShape* CreateSphereShape(float radius, const MaterialType& material = MaterialType::Default);
    PxShape* CreateCapsuleShape(float radius, float halfHeight, const MaterialType& material = MaterialType::Default);
    PxShape* CreateConvexMeshShape(Mesh* mesh, const MaterialType& material = MaterialType::Default);
    PxShape* CreateTriangleMeshShape(Mesh* mesh, const MaterialType& material = MaterialType::Default);
    PxShape* CreateHeightfieldShape(const std::vector<float>& heights, int width, int height, const XMFLOAT3& scale);

    // Materials
    PxMaterial* CreateMaterial(float staticFriction, float dynamicFriction, float restitution);
    PxMaterial* GetMaterial(MaterialType type);
    void SetMaterialProperties(MaterialType type, float staticFriction, float dynamicFriction, float restitution);

    // Forces and impulses
    void ApplyForce(PxRigidDynamic* actor, const XMFLOAT3& force, const XMFLOAT3& localPos = XMFLOAT3(0, 0, 0));
    void ApplyImpulse(PxRigidDynamic* actor, const XMFLOAT3& impulse, const XMFLOAT3& localPos = XMFLOAT3(0, 0, 0));
    void ApplyTorque(PxRigidDynamic* actor, const XMFLOAT3& torque);
    void ApplyTorqueImpulse(PxRigidDynamic* actor, const XMFLOAT3& torque);

    // Queries
    bool Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, PxRaycastHit& hit);
    std::vector<PxRaycastHit> RaycastAll(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance);
    bool Spherecast(const XMFLOAT3& origin, float radius, const XMFLOAT3& direction, float maxDistance, PxSweepHit& hit);
    bool Overlap(const XMFLOAT3& position, float radius, std::vector<PxOverlapHit>& overlaps);

    // Advanced features
    void EnableGPUDynamics(bool enable);
    void EnableSoftBodies(bool enable);
    void EnableParticles(bool enable);
    void EnableFluids(bool enable);
    void EnableDestruction(bool enable);

    // Soft bodies (requires PxSoftBody extension)
    PxSoftBody* CreateSoftBody(Mesh* mesh, const XMFLOAT3& position);
    void SetSoftBodyProperties(PxSoftBody* softBody, float youngsModulus, float poissonsRatio, float damping);

    // Particles
    void CreateParticleSystem(int maxParticles, const XMFLOAT3& position);
    void EmitParticles(int particleSystemId, const XMFLOAT3& position, const XMFLOAT3& velocity, int count);
    void SetParticleProperties(int particleSystemId, float mass, float radius, float damping);

    // Fluids
    void CreateFluidSystem(int maxParticles, const XMFLOAT3& position);
    void SetFluidProperties(int fluidSystemId, float density, float viscosity, float surfaceTension);

    // Vehicles (requires PxVehicle extension)
    void CreateVehicle(const XMFLOAT3& position, Mesh* chassisMesh);
    void SetVehicleInput(int vehicleId, float steering, float acceleration, float braking);

    // Character controller
    PxController* CreateCharacterController(const XMFLOAT3& position, float radius, float height);
    void MoveCharacter(PxController* controller, const XMFLOAT3& displacement, float deltaTime);

    // Destruction (requires PxDestruction extension)
    void CreateDestructibleMesh(Mesh* mesh, const XMFLOAT3& position, int maxDepth = 3);
    void ApplyDamage(PxRigidDynamic* actor, const XMFLOAT3& position, float radius, float damage);

    // Joints
    PxJoint* CreateFixedJoint(PxRigidActor* actor1, const XMFLOAT3& pos1, PxRigidActor* actor2, const XMFLOAT3& pos2);
    PxJoint* CreateRevoluteJoint(PxRigidActor* actor1, const XMFLOAT3& pos1, PxRigidActor* actor2, const XMFLOAT3& pos2, const XMFLOAT3& axis);
    PxJoint* CreatePrismaticJoint(PxRigidActor* actor1, const XMFLOAT3& pos1, PxRigidActor* actor2, const XMFLOAT3& pos2, const XMFLOAT3& axis);
    PxJoint* CreateSphericalJoint(PxRigidActor* actor1, const XMFLOAT3& pos1, PxRigidActor* actor2, const XMFLOAT3& pos2);
    PxJoint* CreateDistanceJoint(PxRigidActor* actor1, const XMFLOAT3& pos1, PxRigidActor* actor2, const XMFLOAT3& pos2);

    // Callbacks
    void SetCollisionCallback(CollisionCallback callback);
    void SetTriggerCallback(TriggerCallback callback);

    // Debug rendering
    void EnableDebugVisualization(bool enable);
    void SetDebugVisualizationParameter(PxVisualizationParameter::Enum param, float value);
    void RenderDebugData(Camera* camera);

    // Performance and statistics
    PxSimulationStatistics GetSimulationStatistics() const;
    void SetThreadCount(int numThreads);
    void OptimizeGPUMemory();

    // Console platform support
    void InitializeForPlayStation(void* psContext);
    void InitializeForXbox(void* xboxContext);
    void InitializeForNintendoSwitch(void* switchContext);

    // Memory management
    void SetMemoryAllocator(PxAllocatorCallback* allocator);
    void SetFoundation(PxFoundation* foundation);

    // Cooking (mesh preprocessing)
    bool CookConvexMesh(const std::vector<XMFLOAT3>& vertices, PxConvexMesh*& convexMesh);
    bool CookTriangleMesh(const std::vector<XMFLOAT3>& vertices, const std::vector<uint32_t>& indices, PxTriangleMesh*& triangleMesh);
    bool CookHeightfield(const std::vector<float>& heights, int width, int height, PxHeightField*& heightfield);

    // Serialization
    bool SaveScene(const std::string& filename);
    bool LoadScene(const std::string& filename);

    // Utility functions
    XMFLOAT3 PxVec3ToXMFLOAT3(const PxVec3& vec);
    PxVec3 XMFLOAT3ToPxVec3(const XMFLOAT3& vec);
    XMFLOAT4 PxQuatToXMFLOAT4(const PxQuat& quat);
    PxQuat XMFLOAT4ToPxQuat(const XMFLOAT4& quat);

private:
    // PhysX core objects
    PxFoundation* foundation_;
    PxPhysics* physics_;
    PxDefaultCpuDispatcher* dispatcher_;
    PxScene* scene_;
    PxMaterial* defaultMaterial_;
    PxPvd* pvd_;
    PxCooking* cooking_;

    // GPU dynamics
    PxCudaContextManager* cudaContextManager_;
    bool gpuDynamicsEnabled_;

    // Materials
    std::map<MaterialType, PxMaterial*> materials_;
    
    // Settings
    PhysicsSettings settings_;
    bool initialized_;
    bool paused_;
    float accumulator_;

    // Callbacks
    CollisionCallback collisionCallback_;
    TriggerCallback triggerCallback_;

    // Platform-specific
    void* platformContext_;

    // Internal helpers
    void InitializePhysX();
    void InitializeCooking();
    void InitializeGPU();
    void InitializeMaterials();
    void SetupScene();
    PxMaterial* GetOrCreateMaterial(MaterialType type);
    
    // Callback implementation
    class SimulationEventCallback : public PxSimulationEventCallback {
    public:
        SimulationEventCallback(PhysXEngine* engine) : engine_(engine) {}
        virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;
        virtual void onWake(PxActor** actors, PxU32 count) override;
        virtual void onSleep(PxActor** actors, PxU32 count) override;
        virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
        virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
        virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override;
    private:
        PhysXEngine* engine_;
    };

    std::unique_ptr<SimulationEventCallback> simulationCallback_;
};

} // namespace Nexus