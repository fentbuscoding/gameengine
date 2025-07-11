#pragma once

#include "Platform.h"
#include <vector>
#include <memory>
#include <functional>

// Forward declarations for Bullet Physics types - these are in global scope
class btDiscreteDynamicsWorld;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btRigidBody;
class btCollisionShape;
class btTransform;
class btTypedConstraint;
class btVector3;
class btQuaternion;

namespace Nexus {

// Use DirectX math types consistently
using PhysicsVector3 = DirectX::XMFLOAT3;
using PhysicsQuaternion = DirectX::XMFLOAT4;

// Physics transform structure
struct PhysicsTransform {
    PhysicsVector3 position;
    PhysicsQuaternion rotation;
    PhysicsVector3 scale;
    
    PhysicsTransform() 
        : position(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f, 1.0f)
        , scale(1.0f, 1.0f, 1.0f) {}
    
    PhysicsTransform(const PhysicsVector3& pos, const PhysicsQuaternion& rot, const PhysicsVector3& scl = {1.0f, 1.0f, 1.0f})
        : position(pos), rotation(rot), scale(scl) {}
};

// Type aliases for physics system
using RigidBodyID = uintptr_t;
using RagdollID = int;
using ConstraintID = int;

struct CollisionShape {
    enum class Type {
        Box,
        Sphere,
        Capsule,
        Mesh
    };
    
    Type type;
    
    struct BoxData {
        PhysicsVector3 halfExtents;
    } box;
    
    struct SphereData {
        float radius;
    } sphere;
    
    struct CapsuleData {
        float radius;
        float height;
    } capsule;
};

struct RigidBodyDesc {
    CollisionShape shape;
    float mass;
    PhysicsVector3 position;
    float friction;
    float restitution;
    float linearDamping;
    float angularDamping;
    bool isKinematic;
};

enum class CollisionShapeType {
    Box,
    Sphere,
    Capsule,
    Cylinder,
    Cone,
    Mesh,
    ConvexHull,
    HeightField
};

enum class RigidBodyType {
    Static,
    Kinematic,
    Dynamic
};

struct RigidBodyInfo {
    CollisionShapeType shapeType;
    RigidBodyType bodyType;
    PhysicsVector3 dimensions;
    float mass;
    float friction;
    float restitution;
    PhysicsVector3 position;
    PhysicsQuaternion rotation;
    PhysicsVector3 transform;
    float linearDamping;
    float angularDamping;
    bool enableBulletPenetration;
    float bulletVelocityThreshold;
};

struct BulletPenetrationData {
    float penetrationDepth;
    PhysicsVector3 penetrationDirection;
    float damage;
    bool hasExitWound;
    PhysicsVector3 exitPoint;
};

struct RagdollJoint {
    int parentBoneIndex;
    int childBoneIndex;
    PhysicsVector3 anchor;
    PhysicsVector3 minAngles;
    PhysicsVector3 maxAngles;
    float breakForce;
    bool isActive;
};

struct Ragdoll {
    std::vector<btRigidBody*> bodies;
    std::vector<btTypedConstraint*> constraints;
    PhysicsVector3 position;
    bool isActive;
    
    Ragdoll() : position(0.0f, 0.0f, 0.0f), isActive(true) {}
};

class RigidBody {
public:
    RigidBody(const RigidBodyInfo& info);
    ~RigidBody();
    
    void SetPosition(const PhysicsVector3& position);
    void SetRotation(const PhysicsQuaternion& rotation);
    PhysicsVector3 GetPosition() const;
    PhysicsQuaternion GetRotation() const;
    
    void SetLinearVelocity(const PhysicsVector3& velocity);
    void SetAngularVelocity(const PhysicsVector3& velocity);
    PhysicsVector3 GetLinearVelocity() const;
    PhysicsVector3 GetAngularVelocity() const;
    
    void ApplyForce(const PhysicsVector3& force);
    void ApplyImpulse(const PhysicsVector3& impulse);
    void ApplyTorque(const PhysicsVector3& torque);
    
    void SetMass(float mass);
    float GetMass() const;
    
    void SetFriction(float friction);
    void SetRestitution(float restitution);
    
    // Bullet penetration
    void EnableBulletPenetration(bool enable);
    bool ProcessBulletPenetration(const PhysicsVector3& bulletStart, const PhysicsVector3& bulletEnd, 
                                  float bulletVelocity, BulletPenetrationData& outData);
    
    // Ragdoll physics
    void ConvertToRagdoll(const std::vector<RagdollJoint>& joints);
    void SetRagdollActive(bool active);
    bool IsRagdollActive() const;
    
    btRigidBody* GetBulletBody() const { return bulletBody_; }
    
private:
    btRigidBody* bulletBody_;
    btCollisionShape* collisionShape_;
    bool isBulletPenetrationEnabled_;
    bool isRagdollActive_;
    std::vector<RagdollJoint> ragdollJoints_;
    
    // Additional members for implementation
    RigidBodyID id;
    PhysicsVector3 transform;
    float mass;
    bool isStatic;
    
    void CreateBulletBody(const RigidBodyInfo& info);
    void CreateCollisionShape(CollisionShapeType type, const PhysicsVector3& dimensions);
};

struct ContactPoint {
    PhysicsVector3 position;
    PhysicsVector3 normal;
    float distance;
    RigidBody* bodyA;
    RigidBody* bodyB;
    float impulse;
};

class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine();
    
    bool Initialize();
    void Shutdown();
    
    void StepSimulation(float deltaTime);
    void Update(float deltaTime);
    void FixedUpdate(float fixedDeltaTime);
    
    // World settings
    void SetGravity(const PhysicsVector3& gravity);
    PhysicsVector3 GetGravity() const;
    
    void SetTimeScale(float scale);
    float GetTimeScale() const;
    
    // Rigid body management
    std::shared_ptr<RigidBody> CreateRigidBody(const RigidBodyInfo& info);
    void DestroyRigidBody(std::shared_ptr<RigidBody> body);
    
    // Raycasting and collision detection
    bool Raycast(const PhysicsVector3& start, const PhysicsVector3& end, ContactPoint& hitInfo);
    std::vector<ContactPoint> RaycastAll(const PhysicsVector3& start, const PhysicsVector3& end);
    bool SphereCast(const PhysicsVector3& start, const PhysicsVector3& end, float radius, ContactPoint& hitInfo);
    
    // Physics settings
    void SetTimeStep(float timeStep);
    float GetTimeStep() const;
    void SetMaxSubSteps(int maxSubSteps);
    int GetMaxSubSteps() const;
    void RemoveRigidBody(std::shared_ptr<RigidBody> body);
    void EnableDebugDraw(bool enabled);
    bool IsDebugDrawEnabled() const;
    
    // Bullet penetration system
    void SetBulletPenetrationEnabled(bool enabled);
    bool ProcessBulletTrajectory(const PhysicsVector3& start, const PhysicsVector3& end, 
                                float velocity, float mass, 
                                std::vector<BulletPenetrationData>& penetrations);
    
    // Advanced physics features
    void SetSubsteps(int substeps);
    void SetSolverIterations(int iterations);
    
    // Collision callbacks
    using CollisionCallback = std::function<void(const ContactPoint&)>;
    void SetCollisionCallback(CollisionCallback callback);
    
    // Ragdoll system
    RagdollID CreateRagdoll(const PhysicsVector3& position);
    RagdollID CreateHumanoidRagdoll(const PhysicsVector3& position);
    void DestroyRagdoll(RagdollID ragdollId);
    void SetRagdollPose(RagdollID ragdollId, const std::vector<PhysicsTransform>& poses);
    std::vector<PhysicsTransform> GetRagdollPose(RagdollID ragdollId);
    void ApplyForceToRagdoll(RagdollID ragdollId, const PhysicsVector3& force, const PhysicsVector3& point);
    
    // Enhanced physics creation methods
    RigidBodyID CreateRigidBody(CollisionShape::Type shapeType, const PhysicsVector3& size, 
                               const PhysicsVector3& position, const PhysicsQuaternion& rotation,
                               float mass, float friction = 0.5f, float restitution = 0.3f);
    void CreateRagdollJoints(Ragdoll& ragdoll);
    
    // Soft body physics (for cloth, deformation)
    void EnableSoftBodySupport(bool enable);
    
    // Debug rendering
    void SetDebugDrawEnabled(bool enabled);
    
    // Rendering support
    struct RenderObject {
        PhysicsVector3 position;
        PhysicsQuaternion rotation;
        PhysicsVector3 scale;
        CollisionShape::Type shapeType;
        DirectX::XMFLOAT4 color;
    };
    
    std::vector<RenderObject> GetRenderObjects() const;
    void UpdateRenderObjects();
    
    // Demo and utility methods
    void CreatePhysicsDemo();
    void ApplyExplosion(const PhysicsVector3& center, float force, float radius);
    void DebugDrawWorld();
    
private:
    btDiscreteDynamicsWorld* dynamicsWorld_;
    btDefaultCollisionConfiguration* collisionConfig_;
    btCollisionDispatcher* dispatcher_;
    btDbvtBroadphase* broadphase_;
    btSequentialImpulseConstraintSolver* solver_;
    
    // Storage for rigid bodies and shapes
    std::map<RigidBodyID, btRigidBody*> rigidBodies_;
    std::vector<btCollisionShape*> collisionShapes_;
    CollisionCallback collisionCallback_;
    
    // Ragdoll system
    struct Ragdoll {
        RagdollID id;
        std::vector<btRigidBody*> bodies;
        std::vector<btTypedConstraint*> constraints;
        PhysicsVector3 position;
        bool isActive;
        
        Ragdoll() : id(0), position(0.0f, 0.0f, 0.0f), isActive(true) {}
    };
    std::map<RagdollID, Ragdoll> ragdolls_;
    int nextRagdollId_ = 1;
    
    PhysicsVector3 gravity_;
    float timeScale_;
    int substeps_;
    bool bulletPenetrationEnabled_;
    bool debugDrawEnabled_;
    
    // Additional member variables for implementation
    int nextRigidBodyId_;
    int nextConstraintId_;
    float timeStep_;
    int maxSubSteps_;
    bool isInitialized_;
    std::map<ConstraintID, btTypedConstraint*> constraints_;
    void* debugDrawer_;
    
    void ProcessCollisions();
    void UpdateRagdolls(float deltaTime);
    
    // Bullet penetration calculations
    float CalculatePenetrationDepth(float bulletVelocity, float bulletMass, 
                                   float materialDensity, float materialThickness);
    bool CalculateExitPoint(const PhysicsVector3& entryPoint, const PhysicsVector3& direction, 
                           RigidBody* body, PhysicsVector3& exitPoint);
};

} // namespace Nexus
