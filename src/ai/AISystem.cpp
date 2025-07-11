#include "AISystem.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Nexus {

// AIBehaviorTree implementation
AIBehaviorTree::AIBehaviorTree() : rootNode_(nullptr) {}

void AIBehaviorTree::SetRootNode(std::shared_ptr<Node> root) {
    rootNode_ = root;
}

AIBehaviorTree::NodeStatus AIBehaviorTree::Execute(AIEntity* entity) {
    if (!rootNode_) {
        return NodeStatus::Failure;
    }
    return ExecuteNode(rootNode_, entity);
}

AIBehaviorTree::NodeStatus AIBehaviorTree::ExecuteNode(std::shared_ptr<Node> node, AIEntity* entity) {
    if (!node) {
        return NodeStatus::Failure;
    }
    
    if (node->execute) {
        return node->execute(entity);
    }
    
    return NodeStatus::Failure;
}

std::shared_ptr<AIBehaviorTree::Node> AIBehaviorTree::CreatePatrolNode() {
    auto node = std::make_shared<Node>();
    node->type = NodeType::Leaf;
    node->name = "Patrol";
    node->execute = [](AIEntity* entity) -> NodeStatus {
        // Implement patrol behavior
        auto position = entity->GetPosition();
        
        // Simple patrol logic - move in a circle
        static float angle = 0.0f;
        angle += 0.01f;
        if (angle > 6.28f) angle = 0.0f;
        
        DirectX::XMFLOAT3 newPos;
        newPos.x = position.x + std::cos(angle) * 0.1f;
        newPos.y = position.y;
        newPos.z = position.z + std::sin(angle) * 0.1f;
        
        entity->MoveTo(newPos);
        return NodeStatus::Running;
    };
    return node;
}

std::shared_ptr<AIBehaviorTree::Node> AIBehaviorTree::CreateCombatNode() {
    auto node = std::make_shared<Node>();
    node->type = NodeType::Sequence;
    node->name = "Combat";
    node->execute = [](AIEntity* entity) -> NodeStatus {
        // Implement combat behavior
        if (entity->GetCurrentState() == AIState::Attack) {
            // Continue attacking
            return NodeStatus::Running;
        } else if (entity->GetCurrentState() == AIState::Chase) {
            // Switch to attack when close enough
            // This would check distance to target
            return NodeStatus::Success;
        }
        return NodeStatus::Failure;
    };
    return node;
}

std::shared_ptr<AIBehaviorTree::Node> AIBehaviorTree::CreateFlankingNode() {
    auto node = std::make_shared<Node>();
    node->type = NodeType::Leaf;
    node->name = "Flanking";
    node->execute = [](AIEntity* entity) -> NodeStatus {
        // Implement flanking behavior
        // This would calculate flanking positions and move to them
        Logger::Debug("AI entity attempting flanking maneuver");
        return NodeStatus::Running;
    };
    return node;
}

std::shared_ptr<AIBehaviorTree::Node> AIBehaviorTree::CreateCoverSeekingNode() {
    auto node = std::make_shared<Node>();
    node->type = NodeType::Leaf;
    node->name = "SeekCover";
    node->execute = [](AIEntity* entity) -> NodeStatus {
        // Implement cover seeking behavior
        Logger::Debug("AI entity seeking cover");
        return NodeStatus::Running;
    };
    return node;
}

std::shared_ptr<AIBehaviorTree::Node> AIBehaviorTree::CreateGroupCoordinationNode() {
    auto node = std::make_shared<Node>();
    node->type = NodeType::Parallel;
    node->name = "GroupCoordination";
    node->execute = [](AIEntity* entity) -> NodeStatus {
        // Implement group coordination behavior
        Logger::Debug("AI entity coordinating with group");
        return NodeStatus::Running;
    };
    return node;
}

// AIStateMachine implementation
AIStateMachine::AIStateMachine() 
    : currentState_(AIState::Idle), previousState_(AIState::Idle) {}

void AIStateMachine::AddState(const State& state) {
    states_[state.stateType] = state;
}

void AIStateMachine::SetCurrentState(AIState state) {
    if (states_.find(state) != states_.end()) {
        auto& currentStateData = states_[currentState_];
        if (currentStateData.onExit) {
            currentStateData.onExit(nullptr); // Entity would be passed here
        }
        
        previousState_ = currentState_;
        currentState_ = state;
        
        auto& newStateData = states_[currentState_];
        if (newStateData.onEnter) {
            newStateData.onEnter(nullptr); // Entity would be passed here
        }
    }
}

void AIStateMachine::Update(AIEntity* entity, float deltaTime) {
    auto it = states_.find(currentState_);
    if (it != states_.end()) {
        auto& state = it->second;
        if (state.onUpdate) {
            state.onUpdate(entity, deltaTime);
        }
        
        // Check transitions
        for (const auto& transition : state.transitions) {
            if (transition.second(entity)) {
                SetCurrentState(transition.first);
                break;
            }
        }
    }
}

// AIPathfinding implementation
std::vector<DirectX::XMFLOAT3> AIPathfinding::FindPath(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& goal) {
    std::vector<DirectX::XMFLOAT3> path;
    
    // Simple straight-line path for now
    path.push_back(start);
    
    // Add intermediate points
    DirectX::XMFLOAT3 direction;
    direction.x = goal.x - start.x;
    direction.y = goal.y - start.y;
    direction.z = goal.z - start.z;
    
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (distance > 0.0f) {
        direction.x /= distance;
        direction.y /= distance;
        direction.z /= distance;
        
        int steps = (int)(distance / 2.0f); // 2 unit steps
        for (int i = 1; i < steps; ++i) {
            DirectX::XMFLOAT3 waypoint;
            waypoint.x = start.x + direction.x * i * 2.0f;
            waypoint.y = start.y + direction.y * i * 2.0f;
            waypoint.z = start.z + direction.z * i * 2.0f;
            path.push_back(waypoint);
        }
    }
    
    path.push_back(goal);
    return path;
}

// AIEntity implementation
AIEntity::AIEntity() 
    : position_(0.0f, 0.0f, 0.0f)
    , rotation_(0.0f, 0.0f, 0.0f, 1.0f)
    , personality_(AIPersonality::Tactical)
    , aggression_(0.5f)
    , cautiousness_(0.5f)
    , intelligence_(0.5f)
    , accuracy_(0.7f)
    , reactionTime_(0.3f)
    , health_(100.0f)
    , maxHealth_(100.0f)
    , isAlive_(true)
    , isAlert_(false)
    , debugMode_(false)
    , moveSpeed_(5.0f)
    , turnSpeed_(3.0f) {}

AIPathfinding::AIPathfinding() {
    Logger::Debug("AIPathfinding: Initialized");
}

void AIEntity::Initialize(const DirectX::XMFLOAT3& position) {
    position_ = position;
    
    // Initialize AI systems
    behaviorTree_ = std::make_shared<AIBehaviorTree>();
    stateMachine_ = std::make_shared<AIStateMachine>();
    pathfinding_ = std::make_shared<AIPathfinding>();
    
    // Set up default behavior tree
    auto patrolNode = behaviorTree_->CreatePatrolNode();
    behaviorTree_->SetRootNode(patrolNode);
    
    // Set up state machine
    SetupStateMachine();
    
    isAlive_ = true;
    Logger::Info("AI Entity initialized at position (" + 
                std::to_string(position.x) + ", " + 
                std::to_string(position.y) + ", " + 
                std::to_string(position.z) + ")");
}

void AIEntity::SetupStateMachine() {
    // Idle state
    AIStateMachine::State idleState;
    idleState.stateType = AIState::Idle;
    idleState.onEnter = [](AIEntity* entity) {
        Logger::Debug("AI entering Idle state");
    };
    idleState.onUpdate = [](AIEntity* entity, float deltaTime) {
        // Check for targets or patrol triggers
    };
    idleState.transitions.push_back({AIState::Patrol, [](AIEntity* entity) {
        return true; // Always transition to patrol after idle
    }});
    stateMachine_->AddState(idleState);
    
    // Patrol state
    AIStateMachine::State patrolState;
    patrolState.stateType = AIState::Patrol;
    patrolState.onEnter = [](AIEntity* entity) {
        Logger::Debug("AI entering Patrol state");
    };
    patrolState.onUpdate = [](AIEntity* entity, float deltaTime) {
        // Execute patrol behavior
        if (entity->behaviorTree_) {
            entity->behaviorTree_->Execute(entity);
        }
    };
    patrolState.transitions.push_back({AIState::Chase, [](AIEntity* entity) {
        return entity->isAlert_; // Transition to chase if alert
    }});
    stateMachine_->AddState(patrolState);
    
    // Chase state
    AIStateMachine::State chaseState;
    chaseState.stateType = AIState::Chase;
    chaseState.onEnter = [](AIEntity* entity) {
        Logger::Debug("AI entering Chase state");
    };
    chaseState.onUpdate = [](AIEntity* entity, float deltaTime) {
        // Execute chase behavior
    };
    chaseState.transitions.push_back({AIState::Attack, [](AIEntity* entity) {
        return true; // Simple transition to attack
    }});
    stateMachine_->AddState(chaseState);
    
    // Attack state
    AIStateMachine::State attackState;
    attackState.stateType = AIState::Attack;
    attackState.onEnter = [](AIEntity* entity) {
        Logger::Debug("AI entering Attack state");
    };
    attackState.onUpdate = [](AIEntity* entity, float deltaTime) {
        // Execute attack behavior
    };
    attackState.transitions.push_back({AIState::Patrol, [](AIEntity* entity) {
        return !entity->isAlert_; // Return to patrol when not alert
    }});
    stateMachine_->AddState(attackState);
    
    stateMachine_->SetCurrentState(AIState::Idle);
}

void AIEntity::Update(float deltaTime) {
    if (!isAlive_) return;
    
    // Update AI systems
    if (behaviorTree_) {
        behaviorTree_->Execute(this);
    }
    
    if (stateMachine_) {
        stateMachine_->Update(this, deltaTime);
    }
    
    // Update pathfinding
    if (!currentPath_.empty()) {
        UpdateMovement(deltaTime);
    }
}

void AIEntity::UpdateMovement(float deltaTime) {
    if (currentPath_.empty()) return;
    
    DirectX::XMFLOAT3 target = currentPath_[0];
    DirectX::XMFLOAT3 direction;
    direction.x = target.x - position_.x;
    direction.y = target.y - position_.y;
    direction.z = target.z - position_.z;
    
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    
    if (distance < 0.5f) {
        // Reached waypoint
        currentPath_.erase(currentPath_.begin());
        if (currentPath_.empty()) {
            Logger::Debug("AI reached destination");
        }
    } else {
        // Move toward waypoint
        direction.x /= distance;
        direction.y /= distance;
        direction.z /= distance;
        
        position_.x += direction.x * moveSpeed_ * deltaTime;
        position_.y += direction.y * moveSpeed_ * deltaTime;
        position_.z += direction.z * moveSpeed_ * deltaTime;
    }
}

void AIEntity::MoveTo(const DirectX::XMFLOAT3& target) {
    if (pathfinding_) {
        currentPath_ = pathfinding_->FindPath(position_, target);
    }
}

void AIEntity::TakeDamage(float damage) {
    if (!isAlive_) return;
    
    health_ -= damage;
    isAlert_ = true;
    
    if (health_ <= 0.0f) {
        health_ = 0.0f;
        isAlive_ = false;
        stateMachine_->SetCurrentState(AIState::Dead);
        Logger::Info("AI Entity died");
    } else {
        Logger::Debug("AI Entity took " + std::to_string(damage) + " damage. Health: " + std::to_string(health_));
    }
}

// AIManager implementation
AIManager::AIManager()
    : difficultyLevel_(0.5f)
    , globalAccuracyModifier_(1.0f)
    , globalReactionTime_(0.5f)
    , maxAIEntities_(100)
    , nearLODDistance_(50.0f)
    , farLODDistance_(200.0f)
    , occlusionEnabled_(true)
    , debugVisualization_(false)
    , lastKnownPlayerPosition_{0, 0, 0}
    , lastPlayerPositionTime_(0.0f)
{
    Logger::Info("AIManager: Initializing...");
}

AIManager::~AIManager() {
    Shutdown();
}

bool AIManager::Initialize() {
    Logger::Info("AIManager: Initialized successfully");
    return true;
}

void AIManager::Shutdown() {
    Logger::Info("AIManager: Shutting down...");
    aiEntities_.clear();
    coverPoints_.clear();
    Logger::Info("AIManager: Shutdown complete");
}

void AIManager::Update(float deltaTime) {
    // Update all AI entities
    for (auto& entity : aiEntities_) {
        if (entity && entity->IsActive()) {
            entity->Update(deltaTime);
        }
    }
    
    // Update LOD
    UpdateLOD();
    
    // Process global events
    ProcessGlobalEvents();
    
    // Optimize performance
    OptimizePerformance();
}

std::shared_ptr<AIEntity> AIManager::CreateAIEntity(AIPersonality personality) {
    if (aiEntities_.size() >= maxAIEntities_) {
        Logger::Warning("AIManager: Maximum AI entities reached");
        return nullptr;
    }
    
    auto entity = std::make_shared<AIEntity>();
    entity->SetPersonality(personality);
    aiEntities_.push_back(entity);
    
    return entity;
}

void AIManager::RemoveAIEntity(std::shared_ptr<AIEntity> entity) {
    auto it = std::find(aiEntities_.begin(), aiEntities_.end(), entity);
    if (it != aiEntities_.end()) {
        aiEntities_.erase(it);
    }
}

void AIManager::SetMaxAIEntities(int maxEntities) {
    maxAIEntities_ = maxEntities;
}

void AIManager::SetDifficultyLevel(float difficulty) {
    difficultyLevel_ = std::max(0.0f, std::min(1.0f, difficulty));
}

void AIManager::SetGlobalAccuracyModifier(float modifier) {
    globalAccuracyModifier_ = modifier;
}

void AIManager::SetGlobalReactionTime(float reactionTime) {
    globalReactionTime_ = reactionTime;
}

void AIManager::CreateSquad(const std::vector<std::shared_ptr<AIEntity>>& members) {
    // Basic squad creation
    // TODO: Implement squad behavior
}

void AIManager::DisbandSquad(const std::vector<std::shared_ptr<AIEntity>>& members) {
    // Basic squad disbanding
    // TODO: Implement squad management
}

void AIManager::SetSquadFormation(const std::vector<std::shared_ptr<AIEntity>>& squad, 
                                 const AIFormation& formation) {
    // Basic formation setting
    // TODO: Implement formations
}

void AIManager::SetNavMesh(std::shared_ptr<NavMesh> navMesh) {
    navMesh_ = navMesh;
}

void AIManager::UpdateNavMesh() {
    // TODO: Implement navmesh updates
}

void AIManager::AddCoverPoint(const AICoverPoint& cover) {
    coverPoints_.push_back(cover);
}

void AIManager::RemoveCoverPoint(const AIVector3& position) {
    coverPoints_.erase(
        std::remove_if(coverPoints_.begin(), coverPoints_.end(),
            [&position](const AICoverPoint& cover) {
                float distance = std::sqrt(
                    (cover.position.x - position.x) * (cover.position.x - position.x) +
                    (cover.position.y - position.y) * (cover.position.y - position.y) +
                    (cover.position.z - position.z) * (cover.position.z - position.z)
                );
                return distance < 1.0f;
            }),
        coverPoints_.end()
    );
}

std::vector<AICoverPoint> AIManager::FindCoverPoints(const AIVector3& position, float radius) {
    std::vector<AICoverPoint> nearCover;
    
    for (const auto& cover : coverPoints_) {
        float distance = std::sqrt(
            (cover.position.x - position.x) * (cover.position.x - position.x) +
            (cover.position.y - position.y) * (cover.position.y - position.y) +
            (cover.position.z - position.z) * (cover.position.z - position.z)
        );
        
        if (distance <= radius) {
            nearCover.push_back(cover);
        }
    }
    
    return nearCover;
}

void AIManager::NotifyPlayerPosition(const AIVector3& position) {
    lastKnownPlayerPosition_ = position;
    lastPlayerPositionTime_ = 0.0f;
    
    // Notify all AI entities
    for (auto& entity : aiEntities_) {
        if (entity) {
            entity->lastKnownPlayerPosition = position;
        }
    }
}

void AIManager::NotifyGunshot(const AIVector3& position, float intensity) {
    // Alert nearby AI entities
    for (auto& entity : aiEntities_) {
        if (entity) {
            float distance = std::sqrt(
                (entity->GetPosition().x - position.x) * (entity->GetPosition().x - position.x) +
                (entity->GetPosition().y - position.y) * (entity->GetPosition().y - position.y) +
                (entity->GetPosition().z - position.z) * (entity->GetPosition().z - position.z)
            );
            
            if (distance <= intensity * 20.0f) {
                entity->SetCurrentState(AIState::Investigate);
            }
        }
    }
}

void AIManager::NotifyAlarmActivated() {
    // Set all AI to alert state
    for (auto& entity : aiEntities_) {
        if (entity) {
            entity->SetCurrentState(AIState::Search);
        }
    }
}

void AIManager::NotifyAIKilled(std::shared_ptr<AIEntity> entity) {
    // Remove from active entities
    RemoveAIEntity(entity);
    
    // Alert nearby AI
    for (auto& otherEntity : aiEntities_) {
        if (otherEntity && otherEntity != entity) {
            float distance = std::sqrt(
                (otherEntity->GetPosition().x - entity->GetPosition().x) * (otherEntity->GetPosition().x - entity->GetPosition().x) +
                (otherEntity->GetPosition().y - entity->GetPosition().y) * (otherEntity->GetPosition().y - entity->GetPosition().y) +
                (otherEntity->GetPosition().z - entity->GetPosition().z) * (otherEntity->GetPosition().z - entity->GetPosition().z)
            );
            
            if (distance <= 15.0f) {
                otherEntity->SetCurrentState(AIState::Investigate);
            }
        }
    }
}

void AIManager::SetLODDistances(float nearDistance, float farDistance) {
    nearLODDistance_ = nearDistance;
    farLODDistance_ = farDistance;
}

void AIManager::EnableOcclusion(bool enabled) {
    occlusionEnabled_ = enabled;
}

void AIManager::SetDebugVisualization(bool enabled) {
    debugVisualization_ = enabled;
    
    for (auto& entity : aiEntities_) {
        if (entity) {
            entity->SetDebugMode(enabled);
        }
    }
}

void AIManager::GetAIStatistics(int& totalAI, int& activeAI, int& alertAI) {
    totalAI = aiEntities_.size();
    activeAI = 0;
    alertAI = 0;
    
    for (const auto& entity : aiEntities_) {
        if (entity && entity->IsActive()) {
            activeAI++;
            if (entity->IsAlert()) {
                alertAI++;
            }
        }
    }
}

void AIManager::UpdateLOD() {
    // TODO: Implement LOD system
}

void AIManager::ProcessGlobalEvents() {
    // TODO: Process global AI events
}

void AIManager::OptimizePerformance() {
    // TODO: Implement performance optimizations
}

AIEntity::~AIEntity() {
    Logger::Debug("AIEntity: Destroyed");
}

void AIEntity::SetPersonality(AIPersonality personality) {
    personality_ = personality;
    
    // Adjust AI parameters based on personality
    switch (personality) {
        case AIPersonality::Aggressive:
            aggression_ = 0.9f;
            cautiousness_ = 0.2f;
            break;
        case AIPersonality::Defensive:
            aggression_ = 0.3f;
            cautiousness_ = 0.8f;
            break;
        case AIPersonality::Tactical:
            aggression_ = 0.6f;
            cautiousness_ = 0.6f;
            break;
        case AIPersonality::Coward:
            aggression_ = 0.1f;
            cautiousness_ = 0.9f;
            break;
        case AIPersonality::Berserker:
            aggression_ = 1.0f;
            cautiousness_ = 0.0f;
            break;
        case AIPersonality::Sniper:
            aggression_ = 0.4f;
            cautiousness_ = 0.8f;
            break;
        case AIPersonality::Support:
            aggression_ = 0.3f;
            cautiousness_ = 0.7f;
            break;
    }
    
    Logger::Debug("AIEntity: Personality set to " + std::to_string(static_cast<int>(personality)));
}

AIState AIEntity::GetCurrentState() const {
    if (stateMachine_) {
        return stateMachine_->GetCurrentState();
    }
    return AIState::Idle;
}

void AIEntity::SetCurrentState(AIState state) {
    if (stateMachine_) {
        stateMachine_->SetCurrentState(state);
    }
}

} // namespace Nexus
