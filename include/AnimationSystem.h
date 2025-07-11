#pragma once

#include "Platform.h"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <thread>
#include <DirectXMath.h>

namespace Nexus {

/**
 * Advanced animation system with skeletal animation, blending, and IK
 */
class AnimationSystem {
public:
    enum class AnimationBlendMode {
        Replace,
        Additive,
        Multiply,
        Overlay
    };

    enum class AnimationPlayMode {
        Once,
        Loop,
        PingPong,
        ClampForever
    };

    enum class InterpolationType {
        Linear,
        Cubic,
        Hermite,
        Bezier
    };

    struct Bone {
        std::string name;
        int parentIndex;
        DirectX::XMFLOAT4X4 bindPose;
        DirectX::XMFLOAT4X4 inverseBindPose;
        DirectX::XMFLOAT4X4 currentTransform;
        DirectX::XMFLOAT4X4 localTransform;
        DirectX::XMFLOAT4X4 worldTransform;
        DirectX::XMFLOAT4X4 finalTransform;
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 rotation;
        DirectX::XMFLOAT3 scale;
        std::vector<int> childIndices;
    };

    struct Keyframe {
        float time;
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 rotation;
        DirectX::XMFLOAT3 scale;
        
        // Tangent information for cubic interpolation
        DirectX::XMFLOAT3 inTangentPos;
        DirectX::XMFLOAT3 outTangentPos;
        DirectX::XMFLOAT4 inTangentRot;
        DirectX::XMFLOAT4 outTangentRot;
    };

    struct AnimationTrack {
        int boneIndex;
        std::vector<Keyframe> keyframes;
        InterpolationType interpolationType;
        
        // Find keyframes for interpolation
        void FindKeyframes(float time, int& prevIndex, int& nextIndex, float& t) const;
        
        // Interpolate between keyframes
        DirectX::XMFLOAT3 InterpolatePosition(float time) const;
        DirectX::XMFLOAT4 InterpolateRotation(float time) const;
        DirectX::XMFLOAT3 InterpolateScale(float time) const;
    };

    struct AnimationClip {
        std::string name;
        float duration;
        float frameRate;
        bool isLooping;
        std::vector<AnimationTrack> tracks;
        std::map<std::string, float> events; // Animation events at specific times
        
        // Root motion support
        bool hasRootMotion;
        DirectX::XMFLOAT3 rootMotionDelta;
        DirectX::XMFLOAT4 rootRotationDelta;
    };

    struct Skeleton {
        std::string name;
        std::vector<Bone> bones;
        std::map<std::string, int> boneNameToIndex;
        DirectX::XMFLOAT4X4 rootTransform;
        
        // Find bone by name
        int FindBoneIndex(const std::string& name) const;
        
        // Build bone hierarchy
        void BuildHierarchy();
        
        // Update bone transforms
        void UpdateBoneTransforms();
        
        // Get final bone matrices for rendering
        void GetBoneMatrices(std::vector<DirectX::XMMATRIX>& matrices) const;
    };

    struct AnimationInstance {
        std::string name;
        std::shared_ptr<AnimationClip> clip;
        float currentTime;
        float playbackSpeed;
        float weight;
        bool isPlaying;
        bool isPaused;
        AnimationPlayMode playMode;
        AnimationBlendMode blendMode;
        
        // Layered animation support
        int layer;
        int priority;
        
        // Callbacks
        std::function<void()> onAnimationComplete;
        std::function<void(const std::string&)> onAnimationEvent;
        
        // Update animation state
        void Update(float deltaTime);
        
        // Control playback
        void Play();
        void Pause();
        void Stop();
        void SetTime(float time);
        void SetSpeed(float speed);
        void SetWeight(float weight);
    };

    // Animation blending node for complex state machines
    struct BlendNode {
        enum class NodeType {
            Clip,
            Blend2D,
            Blend1D,
            StateMachine,
            LayerBlend
        };
        
        NodeType type;
        std::string name;
        std::vector<std::shared_ptr<BlendNode>> children;
        std::map<std::string, float> parameters;
        
        virtual void Evaluate(float deltaTime, Skeleton& skeleton) = 0;
        virtual float GetWeight() const = 0;
        virtual void SetWeight(float weight) = 0;
    };

    // 2D Blend node (for directional movement)
    struct Blend2DNode : public BlendNode {
        struct BlendPoint {
            DirectX::XMFLOAT2 position;
            std::shared_ptr<AnimationClip> clip;
            float weight;
        };
        
        std::vector<BlendPoint> blendPoints;
        DirectX::XMFLOAT2 currentPosition;
        
        void Evaluate(float deltaTime, Skeleton& skeleton) override;
        float GetWeight() const override;
        void SetWeight(float weight) override;
        
        void AddBlendPoint(const DirectX::XMFLOAT2& pos, std::shared_ptr<AnimationClip> clip);
        void SetBlendPosition(const DirectX::XMFLOAT2& pos);
    };

    // Animation state machine
    struct AnimationStateMachine {
        struct State {
            std::string name;
            std::shared_ptr<AnimationClip> clip;
            std::shared_ptr<BlendNode> blendTree;
            float speed;
            bool isLooping;
            
            // State events
            std::function<void()> onEnter;
            std::function<void()> onExit;
            std::function<void(float)> onUpdate;
        };
        
        struct Transition {
            std::string fromState;
            std::string toState;
            float duration;
            std::function<bool()> condition;
            bool hasExitTime;
            float exitTime;
            bool canInterrupt;
        };
        
        std::map<std::string, State> states;
        std::vector<Transition> transitions;
        std::string currentState;
        std::string targetState;
        float transitionTime;
        float transitionDuration;
        bool isTransitioning;
        
        void AddState(const std::string& name, const State& state);
        void AddTransition(const Transition& transition);
        void SetState(const std::string& stateName);
        void Update(float deltaTime, Skeleton& skeleton);
        bool CanTransition(const std::string& fromState, const std::string& toState) const;
    };

    // IK solver - FIXED VERSION with all required members
    struct IKSolver {
        enum class SolverType {
            CCD,
            FABRIK,
            TwoBone,
            Jacobian
        };

        std::vector<int> boneChain;
        DirectX::XMFLOAT3 target;
        DirectX::XMFLOAT3 targetPosition;  // Add this for compatibility
        float tolerance;
        int maxIterations;
        SolverType type;  // Add this missing member
        
        IKSolver() : tolerance(0.01f), maxIterations(10), type(SolverType::CCD) {}
        
        // Add missing solver methods
        void Solve(Skeleton& skeleton);
        void SolveCCD(Skeleton& skeleton);
        void SolveFABRIK(Skeleton& skeleton);
        void SolveJacobian(Skeleton& skeleton);
    };

    // Facial animation support
    struct FacialAnimation {
        struct BlendShape {
            std::string name;
            std::vector<DirectX::XMFLOAT3> deltaVertices;
            std::vector<DirectX::XMFLOAT3> deltaNormals;
            float weight;
        };
        
        std::vector<BlendShape> blendShapes;
        std::map<std::string, float> expressionWeights;
        
        void SetExpressionWeight(const std::string& expression, float weight);
        void BlendShapes(std::vector<DirectX::XMFLOAT3>& vertices, std::vector<DirectX::XMFLOAT3>& normals);
    };

    // Cloth simulation for capes, hair, etc.
    struct ClothSimulation {
        struct ClothParticle {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 oldPosition;
            DirectX::XMFLOAT3 velocity;
            float mass;
            bool isPinned;
        };
        
        struct ClothConstraint {
            int particle1, particle2;
            float restLength;
            float stiffness;
        };
        
        std::vector<ClothParticle> particles;
        std::vector<ClothConstraint> constraints;
        DirectX::XMFLOAT3 gravity;
        float damping;
        float windForce;
        DirectX::XMFLOAT3 windDirection;
        
        void Update(float deltaTime);
        void ApplyConstraints();
        void HandleCollisions(const std::vector<DirectX::XMFLOAT3>& colliders);
    };

public:
    AnimationSystem();
    ~AnimationSystem();

    // Initialization
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Skeleton management
    std::shared_ptr<Skeleton> CreateSkeleton(const std::string& name);
    std::shared_ptr<Skeleton> GetSkeleton(const std::string& name);
    void RemoveSkeleton(const std::string& name);

    // Animation clip management
    std::shared_ptr<AnimationClip> LoadAnimationClip(const std::string& filePath);
    std::shared_ptr<AnimationClip> CreateAnimationClip(const std::string& name);
    std::shared_ptr<AnimationClip> GetAnimationClip(const std::string& name);
    void RemoveAnimationClip(const std::string& name);

    // Animation instance management
    std::shared_ptr<AnimationInstance> CreateAnimationInstance(const std::string& name, 
                                                              std::shared_ptr<AnimationClip> clip);
    void RemoveAnimationInstance(const std::string& name);

    // Playback control
    void PlayAnimation(const std::string& instanceName);
    void PauseAnimation(const std::string& instanceName);
    void StopAnimation(const std::string& instanceName);
    void SetAnimationTime(const std::string& instanceName, float time);
    void SetAnimationSpeed(const std::string& instanceName, float speed);
    void SetAnimationWeight(const std::string& instanceName, float weight);

    // Blending
    void BlendAnimations(const std::vector<std::string>& instanceNames, 
                        const std::vector<float>& weights);
    void SetBlendMode(const std::string& instanceName, AnimationBlendMode mode);

    // State machine
    std::shared_ptr<AnimationStateMachine> CreateStateMachine(const std::string& name);
    void UpdateStateMachine(const std::string& name, float deltaTime);
    void SetStateMachineState(const std::string& stateMachineName, 
                             const std::string& stateName);

    // Inverse Kinematics
    std::shared_ptr<IKSolver> CreateIKSolver(IKSolver::SolverType type, 
                                           const std::vector<int>& boneChain);
    void SetIKTarget(const std::string& solverName, const DirectX::XMFLOAT3& target);
    void SolveIK(const std::string& solverName, std::shared_ptr<Skeleton> skeleton);

    // Facial animation
    std::shared_ptr<FacialAnimation> CreateFacialAnimation(const std::string& name);
    void SetFacialExpression(const std::string& animName, 
                           const std::string& expression, float weight);

    // Cloth simulation
    std::shared_ptr<ClothSimulation> CreateClothSimulation(const std::string& name);
    void UpdateClothSimulation(const std::string& name, float deltaTime);

    // Root motion
    void EnableRootMotion(const std::string& instanceName, bool enable);
    DirectX::XMFLOAT3 GetRootMotionDelta(const std::string& instanceName);
    DirectX::XMFLOAT4 GetRootRotationDelta(const std::string& instanceName);

    // Animation events
    void RegisterAnimationEvent(const std::string& instanceName, 
                               float time, const std::string& eventName);
    void SetAnimationEventCallback(const std::string& instanceName,
                                  std::function<void(const std::string&)> callback);

    // Update system
    void Update(float deltaTime);

    // Rendering support
    void UpdateSkeletonMatrices(std::shared_ptr<Skeleton> skeleton);
    void GetBoneMatrices(std::shared_ptr<Skeleton> skeleton, 
                        std::vector<DirectX::XMMATRIX>& matrices);

    // Performance optimization
    void SetLOD(int level); // Level of detail for distant objects
    void EnableCulling(bool enable);
    void SetMaxAnimationDistance(float distance);

    // Debug visualization
    void EnableDebugVisualization(bool enable);
    void DrawSkeleton(std::shared_ptr<Skeleton> skeleton);
    void DrawIKChain(std::shared_ptr<IKSolver> solver);

    // Add missing methods that the implementation expects
    void ApplyAnimationToSkeleton(Skeleton& skeleton, std::shared_ptr<AnimationInstance> instance, float weight);
    DirectX::XMMATRIX Lerp(const DirectX::XMMATRIX& a, const DirectX::XMMATRIX& b, float t);
    float Lerp(float a, float b, float t);  // Add this missing declaration

private:
    // Animation processing
    void ProcessAnimationInstance(std::shared_ptr<AnimationInstance> instance, 
                                 float deltaTime);
    void BlendPoses(Skeleton& skeleton, 
                   const std::vector<std::shared_ptr<AnimationInstance>>& instances);
    void InterpolateKeyframes(const AnimationTrack& track, float time,
                            DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotation, 
                            DirectX::XMFLOAT3& scale);

    // Utilities
    void DecomposeMatrix(const DirectX::XMMATRIX& matrix, 
                        DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotation, 
                        DirectX::XMFLOAT3& scale);
    DirectX::XMMATRIX ComposeMatrix(const DirectX::XMFLOAT3& position, 
                           const DirectX::XMFLOAT4& rotation, 
                           const DirectX::XMFLOAT3& scale);

private:
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    
    // Animation data
    std::map<std::string, std::shared_ptr<Skeleton>> skeletons_;
    std::map<std::string, std::shared_ptr<AnimationClip>> animationClips_;
    std::map<std::string, std::shared_ptr<AnimationInstance>> animationInstances_;
    std::map<std::string, std::shared_ptr<AnimationStateMachine>> stateMachines_;
    std::map<std::string, std::shared_ptr<IKSolver>> ikSolvers_;
    std::map<std::string, std::shared_ptr<FacialAnimation>> facialAnimations_;
    std::map<std::string, std::shared_ptr<ClothSimulation>> clothSimulations_;
    
    // Performance settings
    int lodLevel_;
    bool cullingEnabled_;
    float maxAnimationDistance_;
    
    // Debug settings
    bool debugVisualization_;
    
    // Threading support
    bool multithreadingEnabled_;
    std::vector<std::thread> workerThreads_;
    
    // Animation compression
    bool compressionEnabled_;
    float positionTolerance_;
    float rotationTolerance_;
    float scaleTolerance_;
};

} // namespace Nexus
