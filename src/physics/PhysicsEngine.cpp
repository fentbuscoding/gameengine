#include "PhysicsEngine.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Nexus {

PhysicsEngine::PhysicsEngine() 
    : initialized_(false)
{
}

PhysicsEngine::~PhysicsEngine() {
    Shutdown();
}

bool PhysicsEngine::Initialize() {
    Logger::Info("Initializing simplified physics engine...");
    
    // Create basic physics demo objects
    CreatePhysicsDemo();
    
    initialized_ = true;
    Logger::Info("Physics engine initialized successfully");
    return true;
}

void PhysicsEngine::Shutdown() {
    if (!initialized_) return;
    
    Logger::Info("Shutting down physics engine...");
    
    renderObjects_.clear();
    physicsObjects_.clear();
    
    initialized_ = false;
    Logger::Info("Physics engine shut down");
}

void PhysicsEngine::StepSimulation(float deltaTime) {
    if (!initialized_) return;
    
    // Simple physics simulation
    for (auto& obj : physicsObjects_) {
        // Apply gravity
        obj.velocity.y -= 9.81f * deltaTime;
        
        // Update position
        obj.position.x += obj.velocity.x * deltaTime;
        obj.position.y += obj.velocity.y * deltaTime;
        obj.position.z += obj.velocity.z * deltaTime;
        
        // Simple ground collision
        if (obj.position.y < 0.0f) {
            obj.position.y = 0.0f;
            obj.velocity.y = std::abs(obj.velocity.y) * 0.8f; // Bounce with damping
        }
    }
    
    // Update render objects
    for (size_t i = 0; i < physicsObjects_.size() && i < renderObjects_.size(); ++i) {
        renderObjects_[i].position = physicsObjects_[i].position;
    }
}

void PhysicsEngine::CreatePhysicsDemo() {
    Logger::Info("Creating physics demo scene...");
    
    // Clear existing objects
    physicsObjects_.clear();
    renderObjects_.clear();
    
    // Create ground boxes
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            SimplePhysicsObject physObj;
            physObj.position = XMFLOAT3(i * 2.0f - 4.0f, 0.5f, j * 2.0f - 4.0f);
            physObj.velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
            physObj.mass = 1.0f;
            physicsObjects_.push_back(physObj);
            
            RenderObject renderObj;
            renderObj.position = physObj.position;
            renderObj.scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
            renderObj.color = XMFLOAT4(0.8f, 0.4f, 0.2f, 1.0f);
            renderObj.shapeType = CollisionShape::Type::Box;
            renderObjects_.push_back(renderObj);
        }
    }
    
    // Create stacked boxes
    for (int i = 0; i < 3; ++i) {
        SimplePhysicsObject physObj;
        physObj.position = XMFLOAT3(0.0f, 2.0f + i * 2.0f, 0.0f);
        physObj.velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
        physObj.mass = 1.0f;
        physicsObjects_.push_back(physObj);
        
        RenderObject renderObj;
        renderObj.position = physObj.position;
        renderObj.scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
        renderObj.color = XMFLOAT4(0.2f, 0.8f, 0.4f, 1.0f);
        renderObj.shapeType = CollisionShape::Type::Box;
        renderObjects_.push_back(renderObj);
    }
    
    // Create spheres
    for (int i = 0; i < 3; ++i) {
        SimplePhysicsObject physObj;
        physObj.position = XMFLOAT3(3.0f + i * 1.5f, 5.0f, 0.0f);
        physObj.velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
        physObj.mass = 0.5f;
        physicsObjects_.push_back(physObj);
        
        RenderObject renderObj;
        renderObj.position = physObj.position;
        renderObj.scale = XMFLOAT3(0.5f, 0.5f, 0.5f);
        renderObj.color = XMFLOAT4(0.4f, 0.2f, 0.8f, 1.0f);
        renderObj.shapeType = CollisionShape::Type::Sphere;
        renderObjects_.push_back(renderObj);
    }
    
    Logger::Info("Created " + std::to_string(physicsObjects_.size()) + " physics objects");
}

void PhysicsEngine::ApplyExplosion(const XMFLOAT3& center, float force, float radius) {
    Logger::Info("Applying explosion at (" + std::to_string(center.x) + ", " + 
                 std::to_string(center.y) + ", " + std::to_string(center.z) + ")");
    
    for (auto& obj : physicsObjects_) {
        // Calculate distance from explosion center
        float dx = obj.position.x - center.x;
        float dy = obj.position.y - center.y;
        float dz = obj.position.z - center.z;
        float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance < radius && distance > 0.1f) {
            // Calculate explosion force
            float explosionForce = force / (distance * distance);
            
            // Normalize direction
            float invDistance = 1.0f / distance;
            dx *= invDistance;
            dy *= invDistance;
            dz *= invDistance;
            
            // Apply force
            obj.velocity.x += dx * explosionForce;
            obj.velocity.y += dy * explosionForce;
            obj.velocity.z += dz * explosionForce;
        }
    }
}

std::vector<RenderObject> PhysicsEngine::GetRenderObjects() const {
    return renderObjects_;
}

void PhysicsEngine::Update(float deltaTime) {
    StepSimulation(deltaTime);
}

} // namespace Nexus
