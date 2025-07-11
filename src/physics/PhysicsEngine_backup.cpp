#include "PhysicsEngine.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Nexus {

PhysicsEngine::PhysicsEngine() 
    : initialized_(false)
{
}
    , dispatcher_(nullptr)
    , broadphase_(nullptr)
    , solver_(nullptr)
    , nextRigidBodyId_(1)
    , nextConstraintId_(1)
    , nextRagdollId_(1)
    , timeStep_(1.0f / 60.0f)
    , maxSubSteps_(10)
    , substeps_(1)
    , timeScale_(1.0f)
    , bulletPenetrationEnabled_(false)
    , debugDrawEnabled_(false)
    , isInitialized_(false)
    , debugDrawer_(nullptr)
    , gravity_(0.0f, -9.81f, 0.0f)
{
}

PhysicsEngine::~PhysicsEngine() {
    Shutdown();
}

bool PhysicsEngine::Initialize() {
    // Initialize Bullet Physics
    collisionConfig_ = new btDefaultCollisionConfiguration();
    dispatcher_ = new btCollisionDispatcher(collisionConfig_);
    broadphase_ = new btDbvtBroadphase();
    solver_ = new btSequentialImpulseConstraintSolver();
    
    dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, broadphase_, solver_, collisionConfig_);
    
    // Set default gravity
    dynamicsWorld_->setGravity(btVector3(gravity_.x, gravity_.y, gravity_.z));
    
    isInitialized_ = true;
    Logger::Info("PhysicsEngine initialized successfully");
    return true;
}

void PhysicsEngine::Shutdown() {
    if (!isInitialized_) return;
    
    // Clean up rigid bodies
    for (auto& pair : rigidBodies_) {
        if (pair.second) {
            dynamicsWorld_->removeRigidBody(pair.second);
            delete pair.second->getMotionState();
            delete pair.second;
        }
    }
    rigidBodies_.clear();
    
    // Clean up constraints
    for (auto& pair : constraints_) {
        if (pair.second) {
            dynamicsWorld_->removeConstraint(pair.second);
            delete pair.second;
        }
    }
    constraints_.clear();
    
    // Clean up collision shapes
    for (auto shape : collisionShapes_) {
        delete shape;
    }
    collisionShapes_.clear();
    
    // Clean up ragdolls
    for (auto& pair : ragdolls_) {
        for (auto constraint : pair.second.constraints) {
            dynamicsWorld_->removeConstraint(constraint);
            delete constraint;
        }
        for (auto body : pair.second.bodies) {
            dynamicsWorld_->removeRigidBody(body);
            delete body->getMotionState();
            delete body;
        }
    }
    ragdolls_.clear();
    
    // Clean up Bullet Physics world
    if (dynamicsWorld_) {
        delete dynamicsWorld_;
        dynamicsWorld_ = nullptr;
    }
    
    if (solver_) {
        delete solver_;
        solver_ = nullptr;
    }
    
    if (broadphase_) {
        delete broadphase_;
        broadphase_ = nullptr;
    }
    
    if (dispatcher_) {
        delete dispatcher_;
        dispatcher_ = nullptr;
    }
    
    if (collisionConfig_) {
        delete collisionConfig_;
        collisionConfig_ = nullptr;
    }
    
    isInitialized_ = false;
    Logger::Info("PhysicsEngine shutdown complete");
}

void PhysicsEngine::StepSimulation(float deltaTime) {
    if (!isInitialized_ || !dynamicsWorld_) return;
    
    float scaledDeltaTime = deltaTime * timeScale_;
    dynamicsWorld_->stepSimulation(scaledDeltaTime, maxSubSteps_, timeStep_);
}

RigidBodyID PhysicsEngine::CreateRigidBody(CollisionShape::Type shapeType, const PhysicsVector3& size, 
                                          const PhysicsVector3& position, const PhysicsQuaternion& rotation,
                                          float mass, float friction, float restitution) {
    if (!isInitialized_) return 0;
    
    // Create collision shape
    btCollisionShape* shape = nullptr;
    
    switch (shapeType) {
        case CollisionShape::Type::Box:
            shape = new btBoxShape(btVector3(size.x, size.y, size.z));
            break;
        case CollisionShape::Type::Sphere:
            shape = new btSphereShape(size.x); // Use x as radius
            break;
        case CollisionShape::Type::Capsule:
            shape = new btCapsuleShape(size.x, size.y); // radius, height
            break;
        default:
            shape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));
            break;
    }
    
    collisionShapes_.push_back(shape);
    
    // Create rigid body
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
    
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    
    btVector3 inertia(0, 0, 0);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, inertia);
    }
    
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
    rigidBodyCI.m_friction = friction;
    rigidBodyCI.m_restitution = restitution;
    
    btRigidBody* body = new btRigidBody(rigidBodyCI);
    dynamicsWorld_->addRigidBody(body);
    
    RigidBodyID id = nextRigidBodyId_++;
    rigidBodies_[id] = body;
    
    return id;
}

RagdollID PhysicsEngine::CreateHumanoidRagdoll(const PhysicsVector3& position) {
    if (!isInitialized_) return 0;
    
    RagdollID ragdollId = nextRagdollId_++;
    Ragdoll ragdoll;
    ragdoll.position = position;
    ragdoll.isActive = true;
    
    // Create torso
    btCapsuleShape* torsoShape = new btCapsuleShape(0.3f, 1.0f);
    collisionShapes_.push_back(torsoShape);
    
    btTransform torsoTransform;
    torsoTransform.setIdentity();
    torsoTransform.setOrigin(btVector3(position.x, position.y + 1.0f, position.z));
    
    btDefaultMotionState* torsoMotionState = new btDefaultMotionState(torsoTransform);
    btVector3 torsoInertia(0, 0, 0);
    torsoShape->calculateLocalInertia(10.0f, torsoInertia);
    
    btRigidBody::btRigidBodyConstructionInfo torsoCI(10.0f, torsoMotionState, torsoShape, torsoInertia);
    btRigidBody* torso = new btRigidBody(torsoCI);
    dynamicsWorld_->addRigidBody(torso);
    ragdoll.bodies.push_back(torso);
    
    // Create head
    btSphereShape* headShape = new btSphereShape(0.15f);
    collisionShapes_.push_back(headShape);
    
    btTransform headTransform;
    headTransform.setIdentity();
    headTransform.setOrigin(btVector3(position.x, position.y + 1.8f, position.z));
    
    btDefaultMotionState* headMotionState = new btDefaultMotionState(headTransform);
    btVector3 headInertia(0, 0, 0);
    headShape->calculateLocalInertia(3.0f, headInertia);
    
    btRigidBody::btRigidBodyConstructionInfo headCI(3.0f, headMotionState, headShape, headInertia);
    btRigidBody* head = new btRigidBody(headCI);
    dynamicsWorld_->addRigidBody(head);
    ragdoll.bodies.push_back(head);
    
    // Create simple joint between torso and head
    btPoint2PointConstraint* neckJoint = new btPoint2PointConstraint(*torso, *head, 
                                                                    btVector3(0, 0.5f, 0), 
                                                                    btVector3(0, -0.15f, 0));
    dynamicsWorld_->addConstraint(neckJoint, true);
    ragdoll.constraints.push_back(neckJoint);
    
    ragdolls_[ragdollId] = ragdoll;
    
    Logger::Info("Created humanoid ragdoll with ID: " + std::to_string(ragdollId));
    return ragdollId;
}

void PhysicsEngine::CreatePhysicsDemo() {
    if (!isInitialized_) return;
    
    Logger::Info("Creating physics demo scene...");
    
    // Create ground plane
    CreateRigidBody(CollisionShape::Type::Box, {50.0f, 0.1f, 50.0f}, 
                   {0.0f, -0.1f, 0.0f}, {0, 0, 0, 1}, 0.0f, 0.8f, 0.2f);
    
    // Create some stacked boxes
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 3; j++) {
            CreateRigidBody(CollisionShape::Type::Box, {0.5f, 0.5f, 0.5f},
                           {-3.0f + j * 1.1f, 1.0f + i * 1.1f, 0.0f},
                           {0, 0, 0, 1}, 1.0f, 0.6f, 0.4f);
        }
    }
    
    // Create some spheres
    for (int i = 0; i < 10; i++) {
        float x = (rand() % 200 - 100) / 10.0f;
        float z = (rand() % 200 - 100) / 10.0f;
        CreateRigidBody(CollisionShape::Type::Sphere, {0.3f, 0.3f, 0.3f},
                       {x, 5.0f + i * 0.8f, z}, {0, 0, 0, 1}, 0.5f, 0.4f, 0.7f);
    }
    
    // Create ragdoll characters
    CreateHumanoidRagdoll({5.0f, 3.0f, 0.0f});
    CreateHumanoidRagdoll({7.0f, 3.0f, 0.0f});
    CreateHumanoidRagdoll({9.0f, 3.0f, 0.0f});
    
    Logger::Info("Physics demo scene created with boxes, spheres, and ragdolls!");
}

std::vector<PhysicsEngine::RenderObject> PhysicsEngine::GetRenderObjects() const {
    std::vector<RenderObject> renderObjects;
    
    if (!isInitialized_ || !dynamicsWorld_) {
        return renderObjects;
    }
    
    // Iterate through all rigid bodies in the world
    for (int i = 0; i < dynamicsWorld_->getNumCollisionObjects(); i++) {
        btCollisionObject* obj = dynamicsWorld_->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (!body) continue;
        
        RenderObject renderObj;
        
        // Get transform
        btTransform transform = body->getWorldTransform();
        btVector3 origin = transform.getOrigin();
        btQuaternion rotation = transform.getRotation();
        
        renderObj.position = { origin.x(), origin.y(), origin.z() };
        renderObj.rotation = { rotation.x(), rotation.y(), rotation.z(), rotation.w() };
        
        // Get shape info
        btCollisionShape* shape = body->getCollisionShape();
        
        if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE) {
            btBoxShape* boxShape = static_cast<btBoxShape*>(shape);
            btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();
            renderObj.shapeType = CollisionShape::Type::Box;
            renderObj.scale = { halfExtents.x(), halfExtents.y(), halfExtents.z() };
            renderObj.color = { 0.8f, 0.6f, 0.4f, 1.0f }; // Brown for boxes
        }
        else if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
            btSphereShape* sphereShape = static_cast<btSphereShape*>(shape);
            float radius = sphereShape->getRadius();
            renderObj.shapeType = CollisionShape::Type::Sphere;
            renderObj.scale = { radius, radius, radius };
            renderObj.color = { 0.4f, 0.7f, 0.9f, 1.0f }; // Blue for spheres
        }
        else if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE) {
            btCapsuleShape* capsuleShape = static_cast<btCapsuleShape*>(shape);
            float radius = capsuleShape->getRadius();
            float height = capsuleShape->getHalfHeight() * 2.0f;
            renderObj.shapeType = CollisionShape::Type::Capsule;
            renderObj.scale = { radius, height, radius };
            renderObj.color = { 0.9f, 0.4f, 0.4f, 1.0f }; // Red for capsules
        }
        else {
            // Default to box for unsupported shapes
            renderObj.shapeType = CollisionShape::Type::Box;
            renderObj.scale = { 1.0f, 1.0f, 1.0f };
            renderObj.color = { 0.5f, 0.5f, 0.5f, 1.0f }; // Gray for unknown
        }
        
        renderObjects.push_back(renderObj);
    }
    
    return renderObjects;
}

void PhysicsEngine::UpdateRenderObjects() {
    // This method can be used to update rendering data if needed
    // For now, we get fresh data each frame via GetRenderObjects()
}

void PhysicsEngine::ApplyExplosion(const PhysicsVector3& center, float force, float radius) {
    if (!isInitialized_ || !dynamicsWorld_) return;
    
    // Apply explosion force to all rigid bodies within radius
    for (int i = 0; i < dynamicsWorld_->getNumCollisionObjects(); i++) {
        btCollisionObject* obj = dynamicsWorld_->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (!body || body->getMass() == 0.0f) continue; // Skip static bodies
        
        btVector3 bodyPos = body->getWorldTransform().getOrigin();
        btVector3 explosionCenter(center.x, center.y, center.z);
        btVector3 direction = bodyPos - explosionCenter;
        float distance = direction.length();
        
        if (distance < radius && distance > 0.0f) {
            direction.normalize();
            float forceMagnitude = force * (1.0f - distance / radius);
            body->applyCentralImpulse(direction * forceMagnitude);
        }
    }
}

void PhysicsEngine::DebugDrawWorld() {
    if (!isInitialized_ || !dynamicsWorld_) return;
    
    // Debug drawing would be implemented here
    // This is a placeholder
}

void PhysicsEngine::SetGravity(const PhysicsVector3& gravity) {
    gravity_ = gravity;
    if (dynamicsWorld_) {
        dynamicsWorld_->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
    }
}

PhysicsVector3 PhysicsEngine::GetGravity() const {
    return gravity_;
}

} // namespace Nexus
