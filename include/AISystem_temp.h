#pragma once

#include "Platform.h"
#include "PhysicsEngine.h"
#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <unordered_map>
#include <string>

namespace Nexus {

// Type aliases for DirectX 11 compatibility
using AIVector3 = DirectX::XMFLOAT3;
using AIQuaternion = DirectX::XMFLOAT4;

// Forward declarations
class AIEntity;
class NavMesh;
class BehaviorTree;
class StateMachine;

enum class AIState {
    Idle,
    Patrol,
    Search,
    Chase,
    Attack,
    Flee,
    Investigate,
    Cover,
    Reload,
    Dead,
    Stunned
};

enum class AIPersonality {
    Aggressive,
    Defensive,
    Tactical,
    Coward,
    Berserker,
    Sniper,
    Support
};

struct AIPerceptionData {
    AIVector3 lastKnownPlayerPosition;
    float lastSeenTime;
    float hearingRadius;
    float sightRadius;
    float sightAngle;
    bool canSeePlayer;
    bool canHearPlayer;
    std::vector<AIVector3> soundSources;
    std::vector<AIEntity*> visibleAllies;
    std::vector<AIEntity*> visibleEnemies;
};

struct AIWeaponData {
    std::string weaponType;
    float damage;
    float accuracy;
    float range;
    float fireRate;
    int ammoCount;
    int maxAmmo;
    float reloadTime;
    bool isSuppressed;
    AIVector3 muzzleOffset;
};

struct AICoverPoint {
    AIVector3 position;
    AIVector3 normal;
    float quality;
    bool isOccupied;
    bool providesConcealment;
    bool providesProtection;
    float distanceToPlayer;
};

struct AIFormation {
    std::string name;
    std::vector<AIVector3> relativePositions;
    AIVector3 centerPoint;
    float spacing;
    bool maintainFacing;
};

class AIBehaviorTree {
public:
    enum class NodeType {
        Selector,
        Sequence,
        Parallel,
        Decorator,
        Leaf
    };
    
    enum class NodeStatus {
        Success,
        Failure,
        Running
    };
    
    struct Node {
        NodeType type;
        std::string name;
        std::function<NodeStatus(AIEntity*)> execute;
        std::vector<std::shared_ptr<Node>> children;
        std::shared_ptr<Node> parent;
    };
    
    AIBehaviorTree();
    void SetRootNode(std::shared_ptr<Node> root);
    NodeStatus Execute(AIEntity* entity);
    
    // Pre-built behavior nodes
    std::shared_ptr<Node> CreatePatrolNode();
    std::shared_ptr<Node> CreateCombatNode();
    std::shared_ptr<Node> CreateFlankingNode();
    std::shared_ptr<Node> CreateCoverSeekingNode();
    std::shared_ptr<Node> CreateGroupCoordinationNode();
    
private:
    std::shared_ptr<Node> rootNode_;
    NodeStatus ExecuteNode(std::shared_ptr<Node> node, AIEntity* entity);
};

class AIStateMachine {
public:
    using StateUpdateFunc = std::function<void(AIEntity*, float)>;
    using StateEnterFunc = std::function<void(AIEntity*)>;
    using StateExitFunc = std::function<void(AIEntity*)>;
    using TransitionCondition = std::function<bool(AIEntity*)>;
    
    struct State {
        AIState stateType;
        StateUpdateFunc onUpdate;
        StateEnterFunc onEnter;
        StateExitFunc onExit;
        std::vector<std::pair<AIState, TransitionCondition>> transitions;
    };
    
    AIStateMachine();
    void AddState(const State& state);
    void SetCurrentState(AIState state);
    void Update(AIEntity* entity, float deltaTime);
    AIState GetCurrentState() const { return currentState_; }
    
private:
    std::unordered_map<AIState, State> states_;
    AIState currentState_;
    AIState previousState_;
};

class AIPathfinding {
public:
    struct PathNode {
        AIVector3 position;
        float gCost;
        float hCost;
        float fCost;
        std::shared_ptr<PathNode> parent;
        bool isWalkable;
        std::vector<std::shared_ptr<PathNode>> neighbors;
    };
    
    AIPathfinding();
    void SetNavMesh(std::shared_ptr<NavMesh> navMesh);
    
    std::vector<AIVector3> FindPath(const AIVector3& start, const AIVector3& goal);
    std::vector<AIVector3> FindPathWithCover(const AIVector3& start, const AIVector3& goal, 
                                          const AIVector3& threatPosition);
    bool IsPathClear(const AIVector3& start, const AIVector3& end);
    
    // Advanced pathfinding
    std::vector<AIVector3> FindFlankingPath(const AIVector3& start, const AIVector3& target, 
                                         const AIVector3& enemyPosition);
    AIVector3 FindNearestCoverPoint(const AIVector3& position, const AIVector3& threatDirection);
    
private:
    std::shared_ptr<NavMesh> navMesh_;
    std::vector<std::shared_ptr<PathNode>> nodes_;
    
    float CalculateHeuristic(const AIVector3& a, const AIVector3& b);
    std::vector<std::shared_ptr<PathNode>> GetNeighbors(std::shared_ptr<PathNode> node);
};

class AIEntity {
public:
    AIEntity();
    ~AIEntity();
    
    // Core AI systems
    void Initialize(const AIVector3& position);
    void Update(float deltaTime);
    void SetBehaviorTree(std::shared_ptr<AIBehaviorTree> behaviorTree);
    void SetStateMachine(std::shared_ptr<AIStateMachine> stateMachine);
    
    // Position and movement
    void SetPosition(const AIVector3& position);
    AIVector3 GetPosition() const { return position_; }
    void SetRotation(const AIQuaternion& rotation);
    AIQuaternion GetRotation() const { return rotation_; }
    
    void MoveTo(const AIVector3& target);
    void SetMoveSpeed(float speed);
    void SetTurnSpeed(float speed);
    
    // AI behavior configuration
    void SetPersonality(AIPersonality personality);
    AIPersonality GetPersonality() const { return personality_; }
    void SetAggression(float aggression);
    void SetCautiousness(float cautiousness);
    void SetIntelligence(float intelligence);
    
    // Perception system
    void UpdatePerception();
    const AIPerceptionData& GetPerceptionData() const { return perceptionData_; }
    bool CanSeeTarget(const AIVector3& targetPosition);
    bool CanHearSound(const AIVector3& soundPosition, float soundIntensity);
    
    // Combat system
    void SetWeapon(const AIWeaponData& weapon);
    bool CanAttackTarget(const AIVector3& targetPosition);
    void AttackTarget(const AIVector3& targetPosition);
    void SetAccuracyModifier(float modifier);
    
    // Group AI and coordination
    void SetSquad(const std::vector<AIEntity*>& squadMembers);
    void SetFormation(const AIFormation& formation);
    void CoordinateWithSquad();
    void ShareInformation(const AIVector3& playerPosition);
    
    // Cover system
    void FindCover(const AIVector3& threatDirection);
    void SetCurrentCover(const AICoverPoint& cover);
    bool IsInCover() const { return isInCover_; }
    
    // Advanced behaviors
    void StartFlanking(const AIVector3& targetPosition);
    void StartSuppression(const AIVector3& targetArea);
    void RequestBackup();
    void CallForMedic();
    
    // State queries
    AIState GetCurrentState() const;
    bool IsAlive() const { return health_ > 0; }
    bool IsAlert() const { return alertLevel_ > 0.5f; }
    float GetHealth() const { return health_; }
    void TakeDamage(float damage);
    
    // Debug
    void SetDebugMode(bool enabled) { debugMode_ = enabled; }
    void DrawDebugInfo();
    
private:
    // Core properties
    AIVector3 position_;
    AIQuaternion rotation_;
    AIPersonality personality_;
    
    // AI systems
    std::shared_ptr<AIBehaviorTree> behaviorTree_;
    std::shared_ptr<AIStateMachine> stateMachine_;
    std::shared_ptr<AIPathfinding> pathfinding_;
    
    // AI parameters
    float aggression_;
    float cautiousness_;
    float intelligence_;
    float alertLevel_;
    float health_;
    float maxHealth_;
    
    // Movement
    float moveSpeed_;
    float turnSpeed_;
    std::vector<AIVector3> currentPath_;
    int currentPathIndex_;
    
    // Perception
    AIPerceptionData perceptionData_;
    float lastUpdateTime_;
    
    // Combat
    AIWeaponData currentWeapon_;
    float lastAttackTime_;
    float accuracyModifier_;
    
    // Group coordination
    std::vector<AIEntity*> squadMembers_;
    AIFormation currentFormation_;
    bool isSquadLeader_;
    
    // Cover system
    AICoverPoint currentCover_;
    bool isInCover_;
    std::vector<AICoverPoint> knownCoverPoints_;
    
    // Advanced behavior state
    bool isFlanking_;
    bool isSuppressing_;
    AIVector3 flankingTarget_;
    AIVector3 suppressionArea_;
    
    bool debugMode_;
    
    // Internal methods
    void UpdateMovement(float deltaTime);
    void UpdateCombat(float deltaTime);
    void UpdateGroupCoordination(float deltaTime);
    void ProcessPerceptionData();
    void CalculateAccuracy(const AIVector3& target, float& accuracy);
    void ExecutePersonalityBehavior();
};

class AIManager {
public:
    AIManager();
    ~AIManager();
    
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    
    // AI entity management
    std::shared_ptr<AIEntity> CreateAIEntity(const AIVector3& position);
    void DestroyAIEntity(std::shared_ptr<AIEntity> entity);
    void SetMaxAIEntities(int maxEntities);
    
    // Global AI settings
    void SetDifficultyLevel(float difficulty); // 0.0 to 1.0
    void SetGlobalAccuracyModifier(float modifier);
    void SetGlobalReactionTime(float reactionTime);
    
    // Squad management
    void CreateSquad(const std::vector<std::shared_ptr<AIEntity>>& members);
    void DisbandSquad(const std::vector<std::shared_ptr<AIEntity>>& members);
    void SetSquadFormation(const std::vector<std::shared_ptr<AIEntity>>& squad, 
                          const AIFormation& formation);
    
    // Navigation mesh
    void SetNavMesh(std::shared_ptr<NavMesh> navMesh);
    void UpdateNavMesh();
    
    // Cover system
    void AddCoverPoint(const AICoverPoint& cover);
    void RemoveCoverPoint(const AIVector3& position);
    std::vector<AICoverPoint> FindCoverPoints(const AIVector3& position, float radius);
    
    // Global AI events
    void NotifyPlayerPosition(const AIVector3& position);
    void NotifyGunshot(const AIVector3& position, float intensity);
    void NotifyAlarmActivated();
    void NotifyAIKilled(std::shared_ptr<AIEntity> entity);
    
    // Performance optimization
    void SetLODDistances(float nearDistance, float farDistance);
    void EnableOcclusion(bool enabled);
    
    // Debug and analytics
    void SetDebugVisualization(bool enabled);
    void GetAIStatistics(int& totalAI, int& activeAI, int& alertAI);
    
private:
    std::vector<std::shared_ptr<AIEntity>> aiEntities_;
    std::shared_ptr<NavMesh> navMesh_;
    std::vector<AICoverPoint> coverPoints_;
    
    float difficultyLevel_;
    float globalAccuracyModifier_;
    float globalReactionTime_;
    int maxAIEntities_;
    
    float nearLODDistance_;
    float farLODDistance_;
    bool occlusionEnabled_;
    bool debugVisualization_;
    
    AIVector3 lastKnownPlayerPosition_;
    float lastPlayerPositionTime_;
    
    void UpdateLOD();
    void ProcessGlobalEvents();
    void OptimizePerformance();
};

} // namespace Nexus
