#include "AnimationSystem.h"
#include "Camera.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Nexus {

AnimationSystem::AnimationSystem()
    : device_(nullptr)
    , lodLevel_(0)
    , cullingEnabled_(true)
    , maxAnimationDistance_(100.0f)
    , debugVisualization_(false)
    , multithreadingEnabled_(false)
    , compressionEnabled_(false)
    , positionTolerance_(0.001f)
    , rotationTolerance_(0.001f)
    , scaleTolerance_(0.001f)
{
}

AnimationSystem::~AnimationSystem() {
    Shutdown();
}

bool AnimationSystem::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (!device || !context) {
        Logger::Error("AnimationSystem::Initialize - Invalid device or context");
        return false;
    }
    
    device_ = device;
    
    Logger::Info("AnimationSystem initialized successfully");
    return true;
}

void AnimationSystem::Shutdown() {
    // Clean up all animation data
    skeletons_.clear();
    animationClips_.clear();
    animationInstances_.clear();
    stateMachines_.clear();
    ikSolvers_.clear();
    facialAnimations_.clear();
    clothSimulations_.clear();
    
    device_ = nullptr;
    
    Logger::Info("AnimationSystem shut down");
}

std::shared_ptr<AnimationSystem::Skeleton> AnimationSystem::CreateSkeleton(const std::string& name) {
    auto skeleton = std::make_shared<Skeleton>();
    skeleton->name = name;
    skeletons_[name] = skeleton;
    
    Logger::Info("Created skeleton: " + name);
    return skeleton;
}

std::shared_ptr<AnimationSystem::Skeleton> AnimationSystem::GetSkeleton(const std::string& name) {
    auto it = skeletons_.find(name);
    if (it != skeletons_.end()) {
        return it->second;
    }
    return nullptr;
}

void AnimationSystem::RemoveSkeleton(const std::string& name) {
    auto it = skeletons_.find(name);
    if (it != skeletons_.end()) {
        skeletons_.erase(it);
        Logger::Info("Removed skeleton: " + name);
    }
}

std::shared_ptr<AnimationSystem::AnimationClip> AnimationSystem::LoadAnimationClip(const std::string& filePath) {
    // This would load an animation clip from file
    // For now, we'll create a placeholder
    auto clip = std::make_shared<AnimationClip>();
    
    // Extract name from file path
    size_t lastSlash = filePath.find_last_of("/\\");
    size_t lastDot = filePath.find_last_of(".");
    std::string name = filePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    
    clip->name = name;
    clip->duration = 1.0f; // Default duration
    clip->frameRate = 30.0f; // Default frame rate
    clip->isLooping = false;
    clip->hasRootMotion = false;
    
    animationClips_[name] = clip;
    
    Logger::Info("Loaded animation clip: " + name + " from " + filePath);
    return clip;
}

std::shared_ptr<AnimationSystem::AnimationClip> AnimationSystem::CreateAnimationClip(const std::string& name) {
    auto clip = std::make_shared<AnimationClip>();
    clip->name = name;
    clip->duration = 1.0f;
    clip->frameRate = 30.0f;
    clip->isLooping = false;
    clip->hasRootMotion = false;
    
    animationClips_[name] = clip;
    
    Logger::Info("Created animation clip: " + name);
    return clip;
}

void AnimationSystem::RemoveAnimationClip(const std::string& name) {
    auto it = animationClips_.find(name);
    if (it != animationClips_.end()) {
        animationClips_.erase(it);
        Logger::Info("Removed animation clip: " + name);
    }
}

std::shared_ptr<AnimationSystem::AnimationInstance> AnimationSystem::CreateAnimationInstance(
    const std::string& name, std::shared_ptr<AnimationClip> clip) {
    
    if (!clip) {
        Logger::Error("AnimationSystem::CreateAnimationInstance - Invalid clip");
        return nullptr;
    }
    
    auto instance = std::make_shared<AnimationInstance>();
    instance->name = name;
    instance->clip = clip;
    instance->currentTime = 0.0f;
    instance->playbackSpeed = 1.0f;
    instance->weight = 1.0f;
    instance->isPlaying = false;
    instance->isPaused = false;
    instance->playMode = AnimationPlayMode::Loop;
    instance->blendMode = AnimationBlendMode::Replace;
    instance->layer = 0;
    instance->priority = 0;
    
    animationInstances_[name] = instance;
    
    Logger::Info("Created animation instance: " + name);
    return instance;
}

void AnimationSystem::RemoveAnimationInstance(const std::string& name) {
    auto it = animationInstances_.find(name);
    if (it != animationInstances_.end()) {
        animationInstances_.erase(it);
        Logger::Info("Removed animation instance: " + name);
    }
}

void AnimationSystem::PlayAnimation(const std::string& instanceName) {
    auto it = animationInstances_.find(instanceName);
    if (it != animationInstances_.end()) {
        it->second->isPlaying = true;
        it->second->isPaused = false;
        Logger::Info("Playing animation: " + instanceName);
    }
}

void AnimationSystem::PauseAnimation(const std::string& instanceName) {
    auto it = animationInstances_.find(instanceName);
    if (it != animationInstances_.end()) {
        it->second->isPaused = true;
        Logger::Info("Paused animation: " + instanceName);
    }
}

void AnimationSystem::StopAnimation(const std::string& instanceName) {
    auto it = animationInstances_.find(instanceName);
    if (it != animationInstances_.end()) {
        it->second->isPlaying = false;
        it->second->isPaused = false;
        it->second->currentTime = 0.0f;
        Logger::Info("Stopped animation: " + instanceName);
    }
}

void AnimationSystem::SetAnimationTime(const std::string& instanceName, float time) {
    auto it = animationInstances_.find(instanceName);
    if (it != animationInstances_.end()) {
        it->second->currentTime = time;
    }
}

void AnimationSystem::SetAnimationSpeed(const std::string& instanceName, float speed) {
    auto it = animationInstances_.find(instanceName);
    if (it != animationInstances_.end()) {
        it->second->playbackSpeed = speed;
    }
}

void AnimationSystem::SetAnimationWeight(const std::string& instanceName, float weight) {
    auto it = animationInstances_.find(instanceName);
    if (it != animationInstances_.end()) {
        it->second->weight = weight;
    }
}

void AnimationSystem::BlendAnimations(const std::vector<std::string>& instanceNames, 
                                     const std::vector<float>& weights) {
    if (instanceNames.size() != weights.size()) {
        Logger::Error("AnimationSystem::BlendAnimations - Instance names and weights size mismatch");
        return;
    }
    
    for (size_t i = 0; i < instanceNames.size(); ++i) {
        SetAnimationWeight(instanceNames[i], weights[i]);
    }
}

void AnimationSystem::SetBlendMode(const std::string& instanceName, AnimationBlendMode mode) {
    auto it = animationInstances_.find(instanceName);
    if (it != animationInstances_.end()) {
        it->second->blendMode = mode;
    }
}

std::shared_ptr<AnimationSystem::AnimationStateMachine> AnimationSystem::CreateStateMachine(const std::string& name) {
    auto stateMachine = std::make_shared<AnimationStateMachine>();
    stateMachine->isTransitioning = false;
    stateMachine->transitionTime = 0.0f;
    stateMachine->transitionDuration = 0.0f;
    
    stateMachines_[name] = stateMachine;
    
    Logger::Info("Created animation state machine: " + name);
    return stateMachine;
}

void AnimationSystem::UpdateStateMachine(const std::string& name, float deltaTime) {
    auto it = stateMachines_.find(name);
    if (it != stateMachines_.end()) {
        auto& sm = it->second;
        sm->Update(deltaTime, *skeletons_.begin()->second);
        
        // Handle transitions
        if (sm->isTransitioning) {
            sm->transitionTime += deltaTime;
            if (sm->transitionTime >= sm->transitionDuration) {
                sm->isTransitioning = false;
                sm->currentState = sm->targetState;
                sm->transitionTime = 0.0f;
            }
        }
        
        // Check for new transitions
        auto& currentState = sm->states[sm->currentState];
        for (const auto& transition : sm->transitions) {
            if (transition.fromState == sm->currentState && 
                transition.condition && transition.condition()) {
                
                sm->targetState = transition.toState;
                sm->transitionDuration = transition.duration;
                sm->isTransitioning = true;
                sm->transitionTime = 0.0f;
                break;
            }
        }
    }
}

void AnimationSystem::ProcessAnimationInstance(std::shared_ptr<AnimationInstance> instance, float deltaTime) {
    if (!instance || !instance->isPlaying || instance->isPaused) return;
    
    instance->currentTime += deltaTime * instance->playbackSpeed;
    
    if (instance->clip && instance->currentTime >= instance->clip->duration) {
        switch (instance->playMode) {
            case AnimationPlayMode::Once:
                instance->isPlaying = false;
                if (instance->onAnimationComplete) {
                    instance->onAnimationComplete();
                }
                break;
            case AnimationPlayMode::Loop:
                instance->currentTime = 0.0f;
                break;
            case AnimationPlayMode::PingPong:
                instance->playbackSpeed = -instance->playbackSpeed;
                instance->currentTime = instance->clip->duration;
                break;
        }
    }
}

void AnimationSystem::BlendPoses(Skeleton& skeleton, const std::vector<std::shared_ptr<AnimationInstance>>& instances) {
    if (instances.empty()) return;
    
    // Reset bone transforms
    for (auto& bone : skeleton.bones) {
        bone.localTransform = bone.bindPose;
    }
    
    // Blend animations
    float totalWeight = 0.0f;
    for (const auto& instance : instances) {
        totalWeight += instance->weight;
    }
    
    if (totalWeight > 0.0f) {
        for (const auto& instance : instances) {
            float normalizedWeight = instance->weight / totalWeight;
            ApplyAnimationToSkeleton(skeleton, instance, normalizedWeight);
        }
    }
    
    // Update world matrices
    UpdateSkeletonMatrices(std::make_shared<Skeleton>(skeleton));
}

void AnimationSystem::ApplyAnimationToSkeleton(Skeleton& skeleton, std::shared_ptr<AnimationInstance> instance, float weight) {
    if (!instance || !instance->clip) return;
    
    for (const auto& track : instance->clip->tracks) {
        if (track.boneIndex >= 0 && track.boneIndex < skeleton.bones.size()) {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT4 rotation;
            DirectX::XMFLOAT3 scale;
            
            InterpolateKeyframes(track, instance->currentTime, position, rotation, scale);
            
            // Blend with current bone transform
            auto& bone = skeleton.bones[track.boneIndex];
            
            // Linear interpolation for position and scale
            XMFLOAT4X4 transformMatrix;
            // bone.localTransform is already XMFLOAT4X4, so we need to load it as XMMATRIX first
            XMMATRIX boneMatrix = XMLoadFloat4x4(&bone.localTransform);
            XMStoreFloat4x4(&transformMatrix, boneMatrix);
            float lerpedX = Lerp(transformMatrix._41, position.x, weight);
            float lerpedY = Lerp(transformMatrix._42, position.y, weight);
            float lerpedZ = Lerp(transformMatrix._43, position.z, weight);

            // Spherical interpolation for rotation
            DirectX::XMFLOAT4 identityQuaternion(0, 0, 0, 1);
            DirectX::XMVECTOR currentRot = DirectX::XMLoadFloat4(&identityQuaternion);
            DirectX::XMVECTOR targetRot = DirectX::XMLoadFloat4(&rotation);
            DirectX::XMVECTOR blendedRot = DirectX::XMQuaternionSlerp(currentRot, targetRot, weight);

            // Apply rotation to transform matrix
            DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixRotationQuaternion(blendedRot);
            DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
            DirectX::XMMATRIX posMatrix = DirectX::XMMatrixTranslation(lerpedX, lerpedY, lerpedZ);

            DirectX::XMMATRIX finalTransform = scaleMatrix * rotMatrix * posMatrix;
            DirectX::XMStoreFloat4x4(&bone.localTransform, finalTransform);
        }
    }
}

void AnimationSystem::InterpolateKeyframes(const AnimationTrack& track, float time, 
                                          DirectX::XMFLOAT3& position, DirectX::XMFLOAT4& rotation, 
                                          DirectX::XMFLOAT3& scale) {
    
    // Find keyframes to interpolate between
    int keyIndex = 0;
    for (int i = 0; i < track.keyframes.size() - 1; ++i) {
        if (time >= track.keyframes[i].time && time < track.keyframes[i + 1].time) {
            keyIndex = i;
            break;
        }
    }
    
    if (keyIndex >= track.keyframes.size() - 1) {
        // Use last keyframe
        const auto& keyframe = track.keyframes.back();
        position = keyframe.position;
        rotation = keyframe.rotation;
        scale = keyframe.scale;
        return;
    }
    
    const auto& keyframe1 = track.keyframes[keyIndex];
    const auto& keyframe2 = track.keyframes[keyIndex + 1];
    
    float t = (time - keyframe1.time) / (keyframe2.time - keyframe1.time);
    t = std::clamp(t, 0.0f, 1.0f);
    
    // Interpolate position
    position.x = Lerp(keyframe1.position.x, keyframe2.position.x, t);
    position.y = Lerp(keyframe1.position.y, keyframe2.position.y, t);
    position.z = Lerp(keyframe1.position.z, keyframe2.position.z, t);
    
    // Interpolate rotation (slerp)
    DirectX::XMVECTOR q1 = DirectX::XMLoadFloat4(&keyframe1.rotation);
    DirectX::XMVECTOR q2 = DirectX::XMLoadFloat4(&keyframe2.rotation);
    DirectX::XMVECTOR result = DirectX::XMQuaternionSlerp(q1, q2, t);
    DirectX::XMStoreFloat4(&rotation, result);
    
    // Interpolate scale
    scale.x = Lerp(keyframe1.scale.x, keyframe2.scale.x, t);
    scale.y = Lerp(keyframe1.scale.y, keyframe2.scale.y, t);
    scale.z = Lerp(keyframe1.scale.z, keyframe2.scale.z, t);
}

float AnimationSystem::Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

void AnimationSystem::UpdateSkeletonMatrices(std::shared_ptr<Skeleton> skeleton) {
    if (!skeleton) return;
    
    // Calculate world matrices for each bone
    for (int i = 0; i < skeleton->bones.size(); ++i) {
        auto& bone = skeleton->bones[i];
        
        if (bone.parentIndex >= 0) {
            // Multiply by parent's world matrix
            DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&bone.localTransform);
            DirectX::XMMATRIX parent = DirectX::XMLoadFloat4x4(&skeleton->bones[bone.parentIndex].worldTransform);
            DirectX::XMMATRIX world = local * parent;
            DirectX::XMStoreFloat4x4(&bone.worldTransform, world);
        } else {
            // Root bone
            bone.worldTransform = bone.localTransform;
        }
        
        // Calculate final transform (world * inverse bind pose)
        DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&bone.worldTransform);
        DirectX::XMMATRIX invBindPose = DirectX::XMLoadFloat4x4(&bone.inverseBindPose);
        DirectX::XMMATRIX final = invBindPose * world;
        DirectX::XMStoreFloat4x4(&bone.finalTransform, final);
    }
}

void AnimationSystem::SolveIK(const std::string& solverName, std::shared_ptr<Skeleton> skeleton) {
    auto it = ikSolvers_.find(solverName);
    if (it == ikSolvers_.end() || !skeleton) return;
    
    auto& solver = it->second;
    
    // Simple IK solver implementation
    if (solver->type == IKSolver::SolverType::TwoBone && solver->boneChain.size() >= 2) {
        // Two-bone IK (like arm or leg)
        int bone1Index = solver->boneChain[0];
        int bone2Index = solver->boneChain[1];
        
        if (bone1Index < skeleton->bones.size() && bone2Index < skeleton->bones.size()) {
            auto& bone1 = skeleton->bones[bone1Index];
            auto& bone2 = skeleton->bones[bone2Index];
            
            // Calculate bone lengths
            DirectX::XMFLOAT4X4 bone1TransformFloat, bone2TransformFloat;
            DirectX::XMStoreFloat4x4(&bone1TransformFloat, DirectX::XMLoadFloat4x4(&bone1.worldTransform));
            DirectX::XMStoreFloat4x4(&bone2TransformFloat, DirectX::XMLoadFloat4x4(&bone2.worldTransform));
            
            DirectX::XMFLOAT3 bone1Pos(bone1TransformFloat._41, bone1TransformFloat._42, bone1TransformFloat._43);
            DirectX::XMFLOAT3 bone2Pos(bone2TransformFloat._41, bone2TransformFloat._42, bone2TransformFloat._43);
            
            // Use solver->target instead of missing targetPosition
            DirectX::XMFLOAT3 targetPos = solver->target;
            DirectX::XMFLOAT3 direction;
            direction.x = targetPos.x - bone1Pos.x;
            direction.y = targetPos.y - bone1Pos.y;
            direction.z = targetPos.z - bone1Pos.z;
            
            float distanceToTarget = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
            
            if (distanceToTarget > 0.001f) {
                direction.x /= distanceToTarget;
                direction.y /= distanceToTarget;
                direction.z /= distanceToTarget;
                
                // Calculate joint angle using law of cosines
                float bone1Length = 1.0f; // This would be calculated from bone data
                float bone2Length = 1.0f;
                float totalLength = bone1Length + bone2Length;
                if (distanceToTarget < totalLength) {
                    float cosAngle = (bone1Length * bone1Length + distanceToTarget * distanceToTarget - bone2Length * bone2Length) / 
                                    (2.0f * bone1Length * distanceToTarget);
                    cosAngle = std::clamp(cosAngle, -1.0f, 1.0f);
                    
                    float angle = std::acos(cosAngle);
                    
                    // Apply rotation to bones
                    DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0, 1, 0, 0), angle);
                    DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&bone1.localTransform);
                    DirectX::XMStoreFloat4x4(&bone1.localTransform, local * rotation);
                }
            }
        }
    }
}

void AnimationSystem::EnableDebugVisualization(bool enable) {
    debugVisualization_ = enable;
    Logger::Info("Animation debug visualization " + std::string(enable ? "enabled" : "disabled"));
}

void AnimationSystem::Update(float deltaTime) {
    // Update all animation instances
    for (auto& instancePair : animationInstances_) {
        if (instancePair.second) {
            instancePair.second->Update(deltaTime);
        }
    }
    
    // Update all skeletons
    for (auto& skeletonPair : skeletons_) {
        if (skeletonPair.second) {
            UpdateSkeletonMatrices(skeletonPair.second);
        }
    }
    
    // Update state machines
    for (auto& stateMachinePair : stateMachines_) {
        if (stateMachinePair.second) {
            UpdateStateMachine(stateMachinePair.first, deltaTime);
        }
    }
    
    // Update IK solvers
    for (auto& solverPair : ikSolvers_) {
        if (solverPair.second && !skeletons_.empty()) {
            auto firstSkeleton = skeletons_.begin()->second;
            if (firstSkeleton) {
                solverPair.second->Solve(*firstSkeleton);
            }
        }
    }
    
    // Update cloth simulations
    for (auto& clothPair : clothSimulations_) {
        if (clothPair.second) {
            clothPair.second->Update(deltaTime);
        }
    }
}

// Implementation of nested classes
int AnimationSystem::Skeleton::FindBoneIndex(const std::string& name) const {
    auto it = boneNameToIndex.find(name);
    if (it != boneNameToIndex.end()) {
        return it->second;
    }
    return -1;
}

void AnimationSystem::Skeleton::BuildHierarchy() {
    // Build bone hierarchy
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex >= 0 && bones[i].parentIndex < bones.size()) {
            bones[bones[i].parentIndex].childIndices.push_back(i);
        }
    }
}

void AnimationSystem::Skeleton::UpdateBoneTransforms() {
    // Update bone transforms in hierarchy order
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex == -1) {
            // Root bone
            bones[i].currentTransform = bones[i].bindPose;
        } else {
            // Child bone
            XMMATRIX bindPoseMatrix = XMLoadFloat4x4(&bones[i].bindPose);
            XMMATRIX parentMatrix = XMLoadFloat4x4(&bones[bones[i].parentIndex].currentTransform);
            XMMATRIX result = XMMatrixMultiply(bindPoseMatrix, parentMatrix);
            XMStoreFloat4x4(&bones[i].currentTransform, result);
        }
    }
}

void AnimationSystem::Skeleton::GetBoneMatrices(std::vector<XMMATRIX>& matrices) const {
    matrices.resize(bones.size());
    for (size_t i = 0; i < bones.size(); ++i) {
        // Load matrices for calculation
        XMMATRIX bindPoseMatrix = XMLoadFloat4x4(&bones[i].bindPose);
        XMMATRIX currentTransformMatrix = XMLoadFloat4x4(&bones[i].currentTransform);
        XMMATRIX tempResult = XMMatrixMultiply(bindPoseMatrix, currentTransformMatrix);
        
        // Calculate final matrix without modifying const member
        XMMATRIX inverseBindPoseMatrix = XMLoadFloat4x4(&bones[i].inverseBindPose);
        XMMATRIX finalMatrix = XMMatrixMultiply(inverseBindPoseMatrix, tempResult);
        
        // Store directly to matrices vector (which is std::vector<XMMATRIX>)
        matrices[i] = finalMatrix;
    }
}

void AnimationSystem::AnimationInstance::Update(float deltaTime) {
    // Update animation instance
    if (isPlaying && !isPaused) {
        currentTime += deltaTime * playbackSpeed;
    }
}

void AnimationSystem::AnimationInstance::Play() {
    isPlaying = true;
    isPaused = false;
}

void AnimationSystem::AnimationInstance::Pause() {
    isPaused = true;
}

void AnimationSystem::AnimationInstance::Stop() {
    isPlaying = false;
    isPaused = false;
    currentTime = 0.0f;
}

void AnimationSystem::AnimationInstance::SetTime(float time) {
    currentTime = time;
}

void AnimationSystem::AnimationInstance::SetSpeed(float speed) {
    playbackSpeed = speed;
}

void AnimationSystem::AnimationInstance::SetWeight(float weight) {
    this->weight = weight;
}

void AnimationSystem::AnimationTrack::FindKeyframes(float time, int& prevIndex, int& nextIndex, float& t) const {
    // Find surrounding keyframes
    prevIndex = 0;
    nextIndex = 0;
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            prevIndex = i;
            nextIndex = i + 1;
            t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            return;
        }
    }
    
    // If not found, use last keyframe
    prevIndex = nextIndex = keyframes.size() - 1;
    t = 0.0f;
}

XMFLOAT3 AnimationSystem::AnimationTrack::InterpolatePosition(float time) const {
    if (keyframes.empty()) return XMFLOAT3(0.0f, 0.0f, 0.0f);
    if (keyframes.size() == 1) return keyframes[0].position;
    
    int prevIndex, nextIndex;
    float t;
    FindKeyframes(time, prevIndex, nextIndex, t);
    
    XMVECTOR prev = XMLoadFloat3(&keyframes[prevIndex].position);
    XMVECTOR next = XMLoadFloat3(&keyframes[nextIndex].position);
    XMVECTOR result = XMVectorLerp(prev, next, t);
    
    XMFLOAT3 resultFloat3;
    XMStoreFloat3(&resultFloat3, result);
    return resultFloat3;
}

XMFLOAT4 AnimationSystem::AnimationTrack::InterpolateRotation(float time) const {
    if (keyframes.empty()) {
        return XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); // Identity quaternion
    }
    if (keyframes.size() == 1) return keyframes[0].rotation;
    
    int prevIndex, nextIndex;
    float t;
    FindKeyframes(time, prevIndex, nextIndex, t);
    
    XMVECTOR prev = XMLoadFloat4(&keyframes[prevIndex].rotation);
    XMVECTOR next = XMLoadFloat4(&keyframes[nextIndex].rotation);
    XMVECTOR result = XMQuaternionSlerp(prev, next, t);
    
    XMFLOAT4 resultFloat4;
    XMStoreFloat4(&resultFloat4, result);
    return resultFloat4;
}

XMFLOAT3 AnimationSystem::AnimationTrack::InterpolateScale(float time) const {
    if (keyframes.empty()) return XMFLOAT3(1.0f, 1.0f, 1.0f);
    if (keyframes.size() == 1) return keyframes[0].scale;
    
    int prevIndex, nextIndex;
    float t;
    FindKeyframes(time, prevIndex, nextIndex, t);
    
    XMVECTOR prev = XMLoadFloat3(&keyframes[prevIndex].scale);
    XMVECTOR next = XMLoadFloat3(&keyframes[nextIndex].scale);
    XMVECTOR result = XMVectorLerp(prev, next, t);
    
    XMFLOAT3 resultFloat3;
    XMStoreFloat3(&resultFloat3, result);
    return resultFloat3;
}

void AnimationSystem::AnimationStateMachine::AddState(const std::string& name, const State& state) {
    states[name] = state;
}

void AnimationSystem::AnimationStateMachine::AddTransition(const Transition& transition) {
    transitions.push_back(transition);
}

void AnimationSystem::AnimationStateMachine::SetState(const std::string& stateName) {
    if (states.find(stateName) != states.end()) {
        currentState = stateName;
    }
}

void AnimationSystem::AnimationStateMachine::Update(float deltaTime, Skeleton& skeleton) {
    // Update current state
    if (!currentState.empty()) {
        auto& state = states[currentState];
        if (state.onUpdate) {
            state.onUpdate(deltaTime);
        }
    }
    
    // Check for transitions
    for (const auto& transition : transitions) {
        if (transition.fromState == currentState && transition.condition && transition.condition()) {
            // Start transition
            targetState = transition.toState;
            isTransitioning = true;
            transitionTime = 0.0f;
            transitionDuration = transition.duration;
            break;
        }
    }
    
    // Update transition
    if (isTransitioning) {
        transitionTime += deltaTime;
        if (transitionTime >= transitionDuration) {
            // Complete transition
            currentState = targetState;
            isTransitioning = false;
            transitionTime = 0.0f;
        }
    }
}

bool AnimationSystem::AnimationStateMachine::CanTransition(const std::string& fromState, 
                                                          const std::string& toState) const {
    for (const auto& transition : transitions) {
        if (transition.fromState == fromState && transition.toState == toState) {
            return true;
        }
    }
    return false;
}

void AnimationSystem::IKSolver::Solve(Skeleton& skeleton) {
    switch (type) {
        case SolverType::CCD:
            SolveCCD(skeleton);
            break;
        case SolverType::FABRIK:
            SolveFABRIK(skeleton);
            break;
        case SolverType::Jacobian:
            SolveJacobian(skeleton);
            break;
    }
}

void AnimationSystem::IKSolver::SolveCCD(Skeleton& skeleton) {
    // Cyclic Coordinate Descent IK solver
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        bool converged = true;
        
        for (int i = boneChain.size() - 2; i >= 0; --i) {
            int boneIndex = boneChain[i];
            int endEffectorIndex = boneChain.back();
            
            // Get current positions
            XMFLOAT3 bonePos = skeleton.bones[boneIndex].position;
            XMFLOAT3 endEffectorPos = skeleton.bones[endEffectorIndex].position;
            
            // Calculate vectors
            XMVECTOR bonePosVec = XMLoadFloat3(&bonePos);
            XMVECTOR endEffectorVec = XMLoadFloat3(&endEffectorPos);
            XMVECTOR targetVec = XMLoadFloat3(&targetPosition);
            
            XMVECTOR toEndEffector = XMVectorSubtract(endEffectorVec, bonePosVec);
            XMVECTOR toTarget = XMVectorSubtract(targetVec, bonePosVec);
            
            // Calculate angle between vectors
            float dot = XMVectorGetX(XMVector3Dot(toEndEffector, toTarget));
            float lenProduct = XMVectorGetX(XMVector3Length(toEndEffector)) * XMVectorGetX(XMVector3Length(toTarget));
            
            if (lenProduct > 0.001f) {
                float angle = acos(dot / lenProduct);
                
                if (angle > tolerance) {
                    // Calculate rotation axis
                    XMVECTOR axis = XMVector3Cross(toEndEffector, toTarget);
                    axis = XMVector3Normalize(axis);
                    
                    // Create rotation quaternion
                    XMVECTOR rotation = XMQuaternionRotationAxis(axis, angle);
                    
                    // Apply rotation to bone
                    XMVECTOR currentRotation = XMLoadFloat4(&skeleton.bones[boneIndex].rotation);
                    XMVECTOR newRotation = XMQuaternionMultiply(currentRotation, rotation);
                    XMStoreFloat4(&skeleton.bones[boneIndex].rotation, newRotation);
                    
                    converged = false;
                }
            }
        }
        
        if (converged) break;
    }
}

void AnimationSystem::IKSolver::SolveFABRIK(Skeleton& skeleton) {
    // Forward And Backward Reaching Inverse Kinematics
    // This is a simplified implementation
    Logger::Info("Solving FABRIK IK");
}

void AnimationSystem::IKSolver::SolveJacobian(Skeleton& skeleton) {
    // Jacobian-based IK solver
    // This is a simplified implementation
    Logger::Info("Solving Jacobian IK");
}

void AnimationSystem::FacialAnimation::SetExpressionWeight(const std::string& expression, float weight) {
    expressionWeights[expression] = weight;
}

void AnimationSystem::FacialAnimation::BlendShapes(std::vector<XMFLOAT3>& vertices, 
                                                  std::vector<XMFLOAT3>& normals) {
    // Apply blend shape deformations
    for (auto& blendShape : blendShapes) {
        if (blendShape.weight > 0.0f) {
            for (size_t i = 0; i < vertices.size() && i < blendShape.deltaVertices.size(); ++i) {
                XMVECTOR vertex = XMLoadFloat3(&vertices[i]);
                XMVECTOR delta = XMLoadFloat3(&blendShape.deltaVertices[i]);
                vertex = XMVectorAdd(vertex, XMVectorScale(delta, blendShape.weight));
                XMStoreFloat3(&vertices[i], vertex);
                
                XMVECTOR normal = XMLoadFloat3(&normals[i]);
                XMVECTOR deltaNormal = XMLoadFloat3(&blendShape.deltaNormals[i]);
                normal = XMVectorAdd(normal, XMVectorScale(deltaNormal, blendShape.weight));
                XMStoreFloat3(&normals[i], normal);
            }
        }
    }
}

void AnimationSystem::ClothSimulation::Update(float deltaTime) {
    // Update cloth simulation
    for (auto& particle : particles) {
        if (!particle.isPinned) {
            // Apply gravity
            XMVECTOR vel = XMLoadFloat3(&particle.velocity);
            XMVECTOR grav = XMLoadFloat3(&gravity);
            vel = XMVectorAdd(vel, XMVectorScale(grav, deltaTime));
            
            // Apply damping
            vel = XMVectorScale(vel, damping);
            
            // Update position
            XMVECTOR pos = XMLoadFloat3(&particle.position);
            XMVECTOR newPos = XMVectorAdd(pos, XMVectorScale(vel, deltaTime));
            
            XMStoreFloat3(&particle.oldPosition, pos);
            XMStoreFloat3(&particle.position, newPos);
            XMStoreFloat3(&particle.velocity, vel);
        }
    }
    
    // Apply constraints
    ApplyConstraints();
}

void AnimationSystem::ClothSimulation::ApplyConstraints() {
    // Apply distance constraints
    for (const auto& constraint : constraints) {
        XMVECTOR pos1 = XMLoadFloat3(&particles[constraint.particle1].position);
        XMVECTOR pos2 = XMLoadFloat3(&particles[constraint.particle2].position);
        
        XMVECTOR delta = XMVectorSubtract(pos2, pos1);
        float distance = XMVectorGetX(XMVector3Length(delta));
        
        if (distance > 0.001f) {
            float difference = (constraint.restLength - distance) / distance;
            XMVECTOR translate = XMVectorScale(delta, difference * constraint.stiffness * 0.5f);
            
            if (!particles[constraint.particle1].isPinned) {
                pos1 = XMVectorSubtract(pos1, translate);
                XMStoreFloat3(&particles[constraint.particle1].position, pos1);
            }
            if (!particles[constraint.particle2].isPinned) {
                pos2 = XMVectorAdd(pos2, translate);
                XMStoreFloat3(&particles[constraint.particle2].position, pos2);
            }
        }
    }
}

void AnimationSystem::ClothSimulation::HandleCollisions(const std::vector<XMFLOAT3>& colliders) {
    // Handle collisions with external objects
    for (auto& particle : particles) {
        for (const auto& collider : colliders) {
            XMVECTOR particlePos = XMLoadFloat3(&particle.position);
            XMVECTOR colliderPos = XMLoadFloat3(&collider);
            XMVECTOR delta = XMVectorSubtract(particlePos, colliderPos);
            float distance = XMVectorGetX(XMVector3Length(delta));
            
            if (distance < 1.0f) { // Collision radius
                delta = XMVector3Normalize(delta);
                XMVECTOR newPos = XMVectorAdd(colliderPos, XMVectorScale(delta, 1.0f));
                XMStoreFloat3(&particle.position, newPos);
            }
        }
    }
}

void AnimationSystem::Blend2DNode::Evaluate(float deltaTime, Skeleton& skeleton) {
    // Evaluate 2D blend node
    // This would implement proper 2D blending based on blend points
    Logger::Info("Evaluating 2D blend node");
}

float AnimationSystem::Blend2DNode::GetWeight() const {
    return 1.0f; // Placeholder
}

void AnimationSystem::Blend2DNode::SetWeight(float weight) {
    // Set weight for 2D blend node
}

void AnimationSystem::Blend2DNode::AddBlendPoint(const XMFLOAT2& pos, std::shared_ptr<AnimationClip> clip) {
    BlendPoint point;
    point.position = pos;
    point.clip = clip;
    point.weight = 0.0f;
    blendPoints.push_back(point);
}

void AnimationSystem::Blend2DNode::SetBlendPosition(const XMFLOAT2& pos) {
    currentPosition = pos;
    
    // Calculate weights for blend points based on position
    for (auto& point : blendPoints) {
        XMVECTOR currentPosVec = XMLoadFloat2(&pos);
        XMVECTOR pointPosVec = XMLoadFloat2(&point.position);
        XMVECTOR delta = XMVectorSubtract(currentPosVec, pointPosVec);
        float distance = XMVectorGetX(XMVector2Length(delta));
        point.weight = 1.0f / (1.0f + distance); // Simple inverse distance weighting
    }
}

} // namespace Nexus
