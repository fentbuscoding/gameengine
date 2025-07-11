#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

using namespace DirectX;

namespace Nexus {

/**
 * Advanced Animation System with inverse kinematics, motion matching, and facial animation
 */
class AdvancedAnimationSystem {
public:
    enum class AnimationType {
        Skeletal,
        Morph,
        Procedural,
        Physics,
        Facial,
        Hair,
        Cloth
    };

    enum class BlendMode {
        Override,
        Additive,
        Multiply,
        Screen,
        Overlay
    };

    struct Bone {
        std::string name;
        int parentIndex = -1;
        XMFLOAT4X4 bindPose;
        XMFLOAT4X4 inverseBindPose;
        XMFLOAT4X4 currentTransform;
        std::vector<int> children;
    };

    struct Skeleton {
        std::vector<Bone> bones;
        std::unordered_map<std::string, int> boneNameToIndex;
        XMFLOAT4X4 rootTransform;
    };

    struct Keyframe {
        float time;
        XMFLOAT3 position;
        XMFLOAT4 rotation;
        XMFLOAT3 scale;
    };

    struct AnimationClip {
        std::string name;
        float duration;
        bool looping = true;
        std::vector<std::vector<Keyframe>> boneKeyframes;
        std::vector<std::vector<float>> morphKeyframes;
        float fps = 30.0f;
    };

    struct AnimationLayer {
        std::string name;
        float weight = 1.0f;
        BlendMode blendMode = BlendMode::Override;
        bool enabled = true;
        std::vector<uint32_t> activeClips;
    };

    struct IKChain {
        std::string name;
        std::vector<int> bones;
        XMFLOAT3 targetPosition;
        XMFLOAT4 targetRotation;
        float weight = 1.0f;
        int iterations = 10;
        float tolerance = 0.01f;
        bool enableRotationLimits = true;
        std::vector<XMFLOAT2> rotationLimits; // Min/Max for each bone
    };

    struct MotionMatchingData {
        std::vector<AnimationClip> database;
        std::vector<std::vector<XMFLOAT3>> trajectoryData;
        std::vector<std::vector<XMFLOAT3>> poseData;
        float trajectoryWeight = 1.0f;
        float poseWeight = 1.0f;
        float velocityWeight = 0.5f;
    };

    struct FacialRig {
        std::vector<std::string> blendShapeNames;
        std::vector<float> blendShapeWeights;
        std::unordered_map<std::string, int> expressionMapping;
        bool enableEyeTracking = false;
        bool enableLipSync = false;
    };

public:
    AdvancedAnimationSystem();
    ~AdvancedAnimationSystem();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Skeleton Management
    uint32_t CreateSkeleton(const Skeleton& skeleton);
    void DestroySkeleton(uint32_t skeletonId);
    Skeleton* GetSkeleton(uint32_t skeletonId);

    // Animation Clips
    uint32_t LoadAnimationClip(const std::string& filePath);
    uint32_t CreateAnimationClip(const AnimationClip& clip);
    void DestroyAnimationClip(uint32_t clipId);
    AnimationClip* GetAnimationClip(uint32_t clipId);

    // Animation Playback
    uint32_t CreateAnimationState(uint32_t skeletonId);
    void DestroyAnimationState(uint32_t stateId);
    void PlayAnimation(uint32_t stateId, uint32_t clipId, float weight = 1.0f, 
                      float fadeInTime = 0.2f, int layer = 0);
    void StopAnimation(uint32_t stateId, uint32_t clipId, float fadeOutTime = 0.2f);
    void SetAnimationSpeed(uint32_t stateId, uint32_t clipId, float speed);
    void SetAnimationTime(uint32_t stateId, uint32_t clipId, float time);

    // Animation Layers
    void CreateAnimationLayer(uint32_t stateId, const std::string& layerName, 
                             BlendMode blendMode = BlendMode::Override);
    void SetLayerWeight(uint32_t stateId, const std::string& layerName, float weight);
    void SetLayerBlendMode(uint32_t stateId, const std::string& layerName, BlendMode mode);

    // Inverse Kinematics
    uint32_t CreateIKChain(uint32_t stateId, const IKChain& chain);
    void UpdateIKTarget(uint32_t stateId, uint32_t chainId, const XMFLOAT3& position, 
                       const XMFLOAT4& rotation = XMFLOAT4(0,0,0,1));
    void SetIKWeight(uint32_t stateId, uint32_t chainId, float weight);
    void SolveIK(uint32_t stateId, uint32_t chainId);

    // Motion Matching
    void SetupMotionMatching(uint32_t stateId, const MotionMatchingData& data);
    uint32_t FindBestMatch(uint32_t stateId, const XMFLOAT3& desiredVelocity, 
                          const XMFLOAT3& currentPosition);
    void UpdateMotionMatching(uint32_t stateId, float deltaTime);

    // Facial Animation
    uint32_t CreateFacialRig(const FacialRig& rig);
    void SetBlendShapeWeight(uint32_t rigId, const std::string& shapeName, float weight);
    void PlayFacialExpression(uint32_t rigId, const std::string& expression, float intensity = 1.0f);
    void UpdateEyeTracking(uint32_t rigId, const XMFLOAT3& lookTarget);
    void UpdateLipSync(uint32_t rigId, const std::string& phoneme, float intensity);

    // Procedural Animation
    void ApplyWaveDeformation(uint32_t stateId, const std::vector<int>& bones, 
                             float amplitude, float frequency, float time);
    void ApplyNoiseDeformation(uint32_t stateId, const std::vector<int>& bones, 
                              float strength, float scale);
    void ApplySpringDamperTobone(uint32_t stateId, int boneIndex, 
                                const XMFLOAT3& targetPosition, float stiffness, float damping);

    // Physics Integration
    void EnableRagdoll(uint32_t stateId, bool enable);
    void SetBonePhysicsProperties(uint32_t stateId, int boneIndex, 
                                 float mass, float friction, float restitution);
    void ApplyForceTobone(uint32_t stateId, int boneIndex, const XMFLOAT3& force);

    // Update and Rendering
    void Update(float deltaTime);
    void GetFinalTransforms(uint32_t stateId, std::vector<XMFLOAT4X4>& transforms);
    void GetBoneTransform(uint32_t stateId, int boneIndex, XMFLOAT4X4& transform);

    // Animation Events
    void RegisterAnimationEvent(uint32_t clipId, float time, const std::string& eventName);
    void SetAnimationEventCallback(std::function<void(const std::string&)> callback);

    // Retargeting
    void RetargetAnimation(uint32_t sourceClipId, uint32_t targetSkeletonId, 
                          uint32_t& outClipId);
    void SetupBoneMapping(uint32_t sourceSkeletonId, uint32_t targetSkeletonId,
                         const std::unordered_map<std::string, std::string>& mapping);

    // LOD System
    void SetAnimationLOD(uint32_t stateId, int lodLevel);
    void SetLODDistances(const std::vector<float>& distances);

    // Compression
    void CompressAnimationClip(uint32_t clipId, float compressionRatio = 0.5f);
    void SetCompressionSettings(bool enablePositionCompression, bool enableRotationCompression,
                               bool enableScaleCompression);

    // Debug and Visualization
    void EnableDebugVisualization(bool enable) { debugVisualizationEnabled_ = enable; }
    std::vector<DebugLine> GetSkeletonDebugLines(uint32_t stateId);
    std::vector<DebugSphere> GetIKDebugSpheres(uint32_t stateId);

private:
    // Core animation processing
    void UpdateAnimationStates(float deltaTime);
    void BlendAnimations(uint32_t stateId);
    void ApplyIK(uint32_t stateId);
    void ApplyConstraints(uint32_t stateId);

    // IK Solvers
    void SolveCCDIK(IKChain& chain, const std::vector<Bone>& bones);
    void SolveFABRIK(IKChain& chain, const std::vector<Bone>& bones);
    void SolveTwoBoNEIK(IKChain& chain, const std::vector<Bone>& bones);

    // Motion matching algorithms
    float CalculateTrajectoryDistance(const std::vector<XMFLOAT3>& trajectory1,
                                     const std::vector<XMFLOAT3>& trajectory2);
    float CalculatePoseDistance(const std::vector<XMFLOAT3>& pose1,
                               const std::vector<XMFLOAT3>& pose2);

    // Facial animation helpers
    void UpdateFacialMuscles(uint32_t rigId);
    void BlendFacialExpressions(uint32_t rigId);

    // Data storage
    std::unordered_map<uint32_t, std::unique_ptr<Skeleton>> skeletons_;
    std::unordered_map<uint32_t, std::unique_ptr<AnimationClip>> clips_;
    std::unordered_map<uint32_t, struct AnimationState*> states_;
    std::unordered_map<uint32_t, std::unique_ptr<FacialRig>> facialRigs_;
    
    uint32_t nextId_;
    bool debugVisualizationEnabled_;
    std::function<void(const std::string&)> eventCallback_;
    
    // Performance settings
    std::vector<float> lodDistances_;
    bool compressionEnabled_;
    
    // Threading
    bool multithreadingEnabled_;
    int workerThreads_;
};

} // namespace Nexus