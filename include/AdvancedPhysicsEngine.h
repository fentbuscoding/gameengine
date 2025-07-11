#pragma once

#include "PhysicsEngine.h"
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Nexus {

/**
 * Advanced physics engine with soft bodies, fluids, destruction, and cloth simulation
 */
class AdvancedPhysicsEngine : public PhysicsEngine {
public:
    struct PhysicsSettings {
        float gravity = -9.81f;
        int maxSubSteps = 10;
        float fixedTimeStep = 1.0f / 60.0f;
        bool enableCCD = true;  // Continuous Collision Detection
        bool enableMultithreading = true;
        int solverIterations = 10;
        float linearDamping = 0.1f;
        float angularDamping = 0.1f;
        bool enableGPUAcceleration = false;
    };

    struct SoftBodySettings {
        float totalMass = 1.0f;
        int clusters = 64;
        float stiffness = 0.8f;
        float damping = 0.1f;
        bool enableCollision = true;
        bool enableSelfCollision = false;
    };

    struct FluidSettings {
        float density = 1000.0f;
        float viscosity = 0.1f;
        float surfaceTension = 0.0728f;
        float gasConstant = 2000.0f;
        float restDistance = 0.1f;
        int maxParticles = 10000;
        XMFLOAT3 containerSize = XMFLOAT3(10.0f, 10.0f, 10.0f);
    };

public:
    AdvancedPhysicsEngine();
    ~AdvancedPhysicsEngine() override;

    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    // Rigid body physics (inherited)
    using PhysicsEngine::CreateRigidBody;
    using PhysicsEngine::RemoveRigidBody;
    using PhysicsEngine::ApplyForce;
    using PhysicsEngine::ApplyImpulse;

    // Soft body physics
    btSoftBody* CreateCloth(const XMFLOAT3& corner1, const XMFLOAT3& corner2, int resX, int resY);
    btSoftBody* CreateRope(const XMFLOAT3& from, const XMFLOAT3& to, int resolution);
    btSoftBody* CreateSoftBox(const XMFLOAT3& size, const XMFLOAT3& position);
    btSoftBody* CreateSoftSphere(float radius, const XMFLOAT3& position, int resolution = 16);
    void RemoveSoftBody(btSoftBody* softBody);

    // Fluid simulation (SPH - Smoothed Particle Hydrodynamics)
    class FluidSystem {
    public:
        struct Particle {
            XMFLOAT3 position;
            XMFLOAT3 velocity;
            XMFLOAT3 force;
            float density;
            float pressure;
            float mass;
            int id;
        };

        bool Initialize(const FluidSettings& settings);
        void Update(float deltaTime);
        void AddParticle(const XMFLOAT3& position, const XMFLOAT3& velocity = XMFLOAT3(0, 0, 0));
        void RemoveParticle(int particleId);
        const std::vector<Particle>& GetParticles() const { return particles_; }
        void SetBounds(const XMFLOAT3& min, const XMFLOAT3& max);

    private:
        void CalculateDensityPressure();
        void CalculateForces();
        void Integrate(float deltaTime);
        void HandleCollisions();
        float SmoothingKernel(float distance, float radius);
        XMFLOAT3 SmoothingKernelGradient(const XMFLOAT3& r, float radius);

        std::vector<Particle> particles_;
        FluidSettings settings_;
        XMFLOAT3 boundsMin_, boundsMax_;
        int nextParticleId_;
    };

    // Destruction system
    class DestructionSystem {
    public:
        struct FracturePoint {
            XMFLOAT3 position;
            XMFLOAT3 normal;
            float force;
            float radius;
        };

        bool Initialize(btDiscreteDynamicsWorld* world);
        void CreateFracture(btRigidBody* body, const FracturePoint& fracture);
        void CreateExplosion(const XMFLOAT3& center, float radius, float force);
        std::vector<btRigidBody*> FractureObject(btRigidBody* object, int numFragments);
        void Update(float deltaTime);

    private:
        btDiscreteDynamicsWorld* world_;
        std::vector<btRigidBody*> fragments_;
    };

    // Vehicle physics
    class VehicleSystem {
    public:
        struct VehicleSettings {
            float mass = 1500.0f;
            float engineForce = 3000.0f;
            float brakeForce = 100.0f;
            float maxSteerAngle = 0.3f;
            float suspensionStiffness = 20.0f;
            float suspensionDamping = 2.3f;
            float suspensionCompression = 4.4f;
            float wheelFriction = 1000.0f;
            float rollInfluence = 0.1f;
        };

        btRaycastVehicle* CreateVehicle(const VehicleSettings& settings, btRigidBody* chassisBody);
        void UpdateVehicle(btRaycastVehicle* vehicle, float steering, float engine, float brake);
        void AddWheel(btRaycastVehicle* vehicle, const XMFLOAT3& position, bool isFrontWheel);

    private:
        std::unique_ptr<btDefaultVehicleRaycaster> vehicleRaycaster_;
        std::vector<btRaycastVehicle*> vehicles_;
    };

    // Character controller
    class CharacterController {
    public:
        bool Initialize(btDiscreteDynamicsWorld* world, const XMFLOAT3& position, float radius, float height);
        void Move(const XMFLOAT3& direction, float deltaTime);
        void Jump(float force);
        void SetPosition(const XMFLOAT3& position);
        XMFLOAT3 GetPosition() const;
        bool IsOnGround() const;

    private:
        std::unique_ptr<btKinematicCharacterController> controller_;
        std::unique_ptr<btPairCachingGhostObject> ghostObject_;
        std::unique_ptr<btCapsuleShape> capsuleShape_;
        btDiscreteDynamicsWorld* world_;
    };

    // Advanced features
    FluidSystem* GetFluidSystem() { return fluidSystem_.get(); }
    DestructionSystem* GetDestructionSystem() { return destructionSystem_.get(); }
    VehicleSystem* GetVehicleSystem() { return vehicleSystem_.get(); }
    
    CharacterController* CreateCharacterController(const XMFLOAT3& position, float radius, float height);
    void RemoveCharacterController(CharacterController* controller);

    // Settings
    void SetPhysicsSettings(const PhysicsSettings& settings);
    void SetSoftBodySettings(const SoftBodySettings& settings);
    void SetFluidSettings(const FluidSettings& settings);

    // GPU acceleration (experimental)
    void EnableGPUAcceleration(bool enable);
    bool IsGPUAccelerationSupported() const;

    // Debug rendering
    void SetDebugMode(int mode) override;
    void RenderDebugWorld() override;

private:
    bool InitializeSoftBodyWorld();
    void UpdateSoftBodies(float deltaTime);
    void UpdateConstraints(float deltaTime);

    // Soft body world
    btSoftRigidDynamicsWorld* softBodyWorld_;
    btSoftBodyWorldInfo softBodyWorldInfo_;
    
    // Advanced systems
    std::unique_ptr<FluidSystem> fluidSystem_;
    std::unique_ptr<DestructionSystem> destructionSystem_;
    std::unique_ptr<VehicleSystem> vehicleSystem_;
    std::vector<std::unique_ptr<CharacterController>> characterControllers_;

    // Settings
    PhysicsSettings physicsSettings_;
    SoftBodySettings softBodySettings_;
    FluidSettings fluidSettings_;

    // Collections
    std::vector<btSoftBody*> softBodies_;
    std::vector<btTypedConstraint*> constraints_;
    
    // GPU acceleration
    bool gpuAccelerationEnabled_;
    void* gpuSolver_; // Platform-specific GPU solver
};

} // namespace Nexus