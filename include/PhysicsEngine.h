#pragma once

#include "Platform.h"
#include <vector>
#include <memory>
#include <functional>

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
        Cylinder,
        Cone,
        Mesh,
        Plane,
        Heightfield
    };
    
    Type type;
    DirectX::XMFLOAT3 dimensions;
    float radius;
    float height;
    
    CollisionShape() : type(Type::Box), dimensions(1.0f, 1.0f, 1.0f), radius(0.5f), height(1.0f) {}
    
    static CollisionShape CreateBox(const DirectX::XMFLOAT3& size) {
        CollisionShape shape;
        shape.type = Type::Box;
        shape.dimensions = size;
        return shape;
    }
    
    static CollisionShape CreateSphere(float radius) {
        CollisionShape shape;
        shape.type = Type::Sphere;
        shape.radius = radius;
        return shape;
    }
    
    static CollisionShape CreateCapsule(float radius, float height) {
        CollisionShape shape;
        shape.type = Type::Capsule;
        shape.radius = radius;
        shape.height = height;
        return shape;
    }
};

// Structure for objects that can be rendered
struct RenderObject {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT4 color;
    CollisionShape::Type shapeType;
    
    RenderObject() 
        : position(0.0f, 0.0f, 0.0f)
        , scale(1.0f, 1.0f, 1.0f)
        , color(1.0f, 1.0f, 1.0f, 1.0f)
        , shapeType(CollisionShape::Type::Box) {}
};

// Simple physics object for the simplified engine
struct SimplePhysicsObject {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    float mass;
    
    SimplePhysicsObject()
        : position(0.0f, 0.0f, 0.0f)
        , velocity(0.0f, 0.0f, 0.0f)
        , mass(1.0f) {}
};

// Physics material properties
struct PhysicsMaterial {
    float friction;
    float restitution;
    float density;
    
    PhysicsMaterial() : friction(0.5f), restitution(0.0f), density(1.0f) {}
    PhysicsMaterial(float f, float r, float d) : friction(f), restitution(r), density(d) {}
};

// Collision detection callback types
using CollisionCallback = std::function<void(RigidBodyID, RigidBodyID, const PhysicsVector3&)>;

// Main physics engine class
class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine();
    
    // Core functionality
    bool Initialize();
    void Shutdown();
    void StepSimulation(float deltaTime);
    void Update(float deltaTime);
    
    // Demo functionality
    void CreatePhysicsDemo();
    void ApplyExplosion(const DirectX::XMFLOAT3& center, float force, float radius);
    
    // Getters
    std::vector<RenderObject> GetRenderObjects() const;
    
    // Basic physics body creation
    RigidBodyID CreateRigidBody(const CollisionShape& shape, const PhysicsTransform& transform, 
                               float mass = 1.0f, const PhysicsMaterial& material = PhysicsMaterial());
    
    // Body manipulation
    void SetBodyTransform(RigidBodyID bodyId, const PhysicsTransform& transform);
    PhysicsTransform GetBodyTransform(RigidBodyID bodyId) const;
    void SetBodyVelocity(RigidBodyID bodyId, const PhysicsVector3& velocity);
    PhysicsVector3 GetBodyVelocity(RigidBodyID bodyId) const;
    void ApplyForce(RigidBodyID bodyId, const PhysicsVector3& force);
    void ApplyImpulse(RigidBodyID bodyId, const PhysicsVector3& impulse);
    
    // Body management
    void RemoveRigidBody(RigidBodyID bodyId);
    void SetBodyActive(RigidBodyID bodyId, bool active);
    bool IsBodyActive(RigidBodyID bodyId) const;
    
    // Collision detection
    void SetCollisionCallback(CollisionCallback callback);
    std::vector<RigidBodyID> GetCollidingBodies(RigidBodyID bodyId) const;
    
    // Ragdoll physics
    struct RagdollDefinition {
        struct Bone {
            std::string name;
            PhysicsTransform transform;
            CollisionShape shape;
            float mass;
            PhysicsMaterial material;
        };
        
        struct Joint {
            std::string bone1;
            std::string bone2;
            PhysicsVector3 anchor1;
            PhysicsVector3 anchor2;
            PhysicsVector3 axis1;
            PhysicsVector3 axis2;
            float minAngle;
            float maxAngle;
        };
        
        std::vector<Bone> bones;
        std::vector<Joint> joints;
    };
    
    RagdollID CreateRagdoll(const RagdollDefinition& definition, const PhysicsTransform& rootTransform);
    void DestroyRagdoll(RagdollID ragdollId);
    void SetRagdollActive(RagdollID ragdollId, bool active);
    std::vector<PhysicsTransform> GetRagdollBoneTransforms(RagdollID ragdollId) const;
    
    // Constraint system
    enum class ConstraintType {
        Point,
        Hinge,
        Slider,
        ConeTwist,
        Generic6DOF
    };
    
    ConstraintID CreateConstraint(ConstraintType type, RigidBodyID bodyA, RigidBodyID bodyB,
                                 const PhysicsTransform& frameA, const PhysicsTransform& frameB);
    void RemoveConstraint(ConstraintID constraintId);
    void SetConstraintLimits(ConstraintID constraintId, const PhysicsVector3& minLimits, const PhysicsVector3& maxLimits);
    
    // Ray casting
    struct RaycastResult {
        bool hit;
        RigidBodyID bodyId;
        PhysicsVector3 hitPoint;
        PhysicsVector3 hitNormal;
        float distance;
    };
    
    RaycastResult Raycast(const PhysicsVector3& from, const PhysicsVector3& to) const;
    std::vector<RaycastResult> RaycastAll(const PhysicsVector3& from, const PhysicsVector3& to) const;
    
    // Advanced features
    void SetGravity(const PhysicsVector3& gravity);
    PhysicsVector3 GetGravity() const;
    void SetWorldScale(float scale);
    float GetWorldScale() const;
    
    // Debug visualization
    void EnableDebugDrawing(bool enable);
    bool IsDebugDrawingEnabled() const;
    
private:
    bool initialized_;
    std::vector<SimplePhysicsObject> physicsObjects_;
    std::vector<RenderObject> renderObjects_;
    CollisionCallback collisionCallback_;
    PhysicsVector3 gravity_;
    float worldScale_;
    bool debugDrawing_;
    
    // Internal helpers
    void UpdateRenderObjects();
    void ProcessCollisions();
    void ApplyGravity(float deltaTime);
    void IntegrateVelocities(float deltaTime);
    void ResolveCollisions();
};

} // namespace Nexus
