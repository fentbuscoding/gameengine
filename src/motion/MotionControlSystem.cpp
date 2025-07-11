#include "MotionControlSystem.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace Nexus {

MotionControlSystem::MotionControlSystem()
    : kinectSensor_(nullptr)
    , coordinateMapper_(nullptr)
    , bodyFrameReader_(nullptr)
    , gestureRecognizer_(nullptr)
    , isInitialized_(false)
    , calibrationMode_(false)
    , motionSensitivity_(1.0f)
    , aimingSmoothing_(0.8f)
    , gestureThreshold_(0.7f)
{
}

MotionControlSystem::~MotionControlSystem() {
    Shutdown();
}

bool MotionControlSystem::Initialize() {
    Logger::Info("MotionControlSystem::Initialize starting...");
    
    // Initialize Kinect sensor
    if (!InitializeKinect()) {
        Logger::Warning("Kinect not available - motion control features disabled");
        return false;
    }
    Logger::Info("Kinect initialization completed");
    
    // Initialize gesture recognition
    if (!InitializeGestureRecognition()) {
        Logger::Warning("Gesture recognition initialization failed");
    }
    Logger::Info("Gesture recognition initialization completed");
    
    // Initialize motion tracking
    Logger::Info("About to initialize motion tracking...");
    InitializeMotionTracking();
    Logger::Info("Motion tracking initialization completed");
    
    isInitialized_ = true;
    Logger::Info("Motion control system initialized successfully");
    Logger::Info("MotionControlSystem::Initialize ending...");
    return true;
}

void MotionControlSystem::Shutdown() {
    // Clean up Kinect resources
    if (bodyFrameReader_) {
        static_cast<IUnknown*>(bodyFrameReader_)->Release();
        bodyFrameReader_ = nullptr;
    }
    
    if (coordinateMapper_) {
        static_cast<IUnknown*>(coordinateMapper_)->Release();
        coordinateMapper_ = nullptr;
    }
    
    if (kinectSensor_) {
        // For Kinect sensor, we need to close it first
        // static_cast<IKinectSensor*>(kinectSensor_)->Close();
        static_cast<IUnknown*>(kinectSensor_)->Release();
        kinectSensor_ = nullptr;
    }
    
    // Clean up gesture recognition
    if (gestureRecognizer_) {
        delete gestureRecognizer_;
        gestureRecognizer_ = nullptr;
    }
    
    isInitialized_ = false;
    Logger::Info("Motion control system shut down");
}

void MotionControlSystem::Update(float deltaTime) {
    if (!isInitialized_) return;
    
    // Update Kinect body tracking
    UpdateBodyTracking();
    
    // Update gesture recognition
    UpdateGestureRecognition(deltaTime);
    
    // Update motion-based aiming
    UpdateMotionAiming(deltaTime);
    
    // Update motion controls
    UpdateMotionControls(deltaTime);
}

bool MotionControlSystem::InitializeKinect() {
    // Initialize Kinect sensor (simplified for demonstration)
    // In a real implementation, this would use Kinect SDK
    
    Logger::Info("Initializing Kinect sensor...");
    
    // Simulated Kinect initialization
    // HRESULT hr = GetDefaultKinectSensor(&kinectSensor_);
    // if (FAILED(hr)) {
    //     return false;
    // }
    
    // For demonstration, we'll simulate successful initialization
    Logger::Info("Kinect sensor initialized (simulated)");
    return true;
}

bool MotionControlSystem::InitializeGestureRecognition() {
    // Temporarily disable gesture recognition to isolate crash
    Logger::Info("Gesture recognition initialized (disabled for debugging)");
    return true;
}

void MotionControlSystem::InitializeMotionTracking() {
    // Initialize motion tracking parameters
    motionFilter_.windowSize = 5;
    motionFilter_.weights = { 0.1f, 0.2f, 0.4f, 0.2f, 0.1f };
    
    // Initialize joint tracking - resize vectors first!
    int jointCount = static_cast<int>(JointType::Count);
    trackedJoints_.resize(jointCount);
    jointConfidence_.resize(jointCount);
    
    for (int i = 0; i < jointCount; i++) {
        trackedJoints_[i] = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        jointConfidence_[i] = 0.0f;
    }
    
    Logger::Info("Motion tracking initialized");
}

void MotionControlSystem::UpdateBodyTracking() {
    // Update body tracking data from Kinect
    // This would normally read from Kinect body frame
    
    // Simulated body tracking update
    static float time = 0.0f;
    time += 0.016f; // Simulate 60 FPS
    
    // Simulate hand movement for demonstration
    trackedJoints_[static_cast<int>(JointType::HandRight)] = DirectX::XMFLOAT3(
        sinf(time) * 0.5f,
        cosf(time * 0.5f) * 0.3f + 1.0f,
        2.0f
    );
    
    trackedJoints_[static_cast<int>(JointType::HandLeft)] = DirectX::XMFLOAT3(
        -sinf(time) * 0.5f,
        cosf(time * 0.5f) * 0.3f + 1.0f,
        2.0f
    );
    
    // Update confidence values
    jointConfidence_[static_cast<int>(JointType::HandRight)] = 0.9f;
    jointConfidence_[static_cast<int>(JointType::HandLeft)] = 0.9f;
}

void MotionControlSystem::UpdateGestureRecognition(float deltaTime) {
    if (!gestureRecognizer_) return;
    
    // Collect joint positions for gesture recognition
    std::vector<DirectX::XMFLOAT3> keyJoints;
    keyJoints.push_back(trackedJoints_[static_cast<int>(JointType::HandRight)]);
    keyJoints.push_back(trackedJoints_[static_cast<int>(JointType::HandLeft)]);
    keyJoints.push_back(trackedJoints_[static_cast<int>(JointType::Head)]);
    
    // Update gesture recognition
    gestureRecognizer_->Update(keyJoints, deltaTime);
    
    // Check for recognized gestures
    auto recognizedGestures = gestureRecognizer_->GetRecognizedGestures();
    for (const auto& gesture : recognizedGestures) {
        if (gesture.confidence > gestureThreshold_) {
            HandleGesture(gesture);
        }
    }
}

void MotionControlSystem::UpdateMotionAiming(float deltaTime) {
    // Get hand position for aiming
    DirectX::XMFLOAT3 rightHand = trackedJoints_[static_cast<int>(JointType::HandRight)];
    
    // Apply motion filtering for smoother aiming
    ApplyMotionFiltering(rightHand);
    
    // Convert hand position to aiming direction
    DirectX::XMFLOAT3 aimDirection = CalculateAimDirection(rightHand);
    
    // Apply smoothing
    currentAimDirection_ = LerpVector3(currentAimDirection_, aimDirection, 1.0f - aimingSmoothing_);
    
    // Update aiming data
    aimingData_.direction = currentAimDirection_;
    aimingData_.confidence = jointConfidence_[static_cast<int>(JointType::HandRight)];
    aimingData_.isActive = (aimingData_.confidence > 0.5f);
}

void MotionControlSystem::UpdateMotionControls(float deltaTime) {
    // Update motion-based movement controls
    UpdateMovementControls();
    
    // Update motion-based interaction controls
    UpdateInteractionControls();
}

void MotionControlSystem::DefineGesturePatterns() {
    if (!gestureRecognizer_) return;
    
    // Define punch gesture
    GesturePattern punchPattern;
    punchPattern.name = "Punch";
    punchPattern.jointSequence = { JointType::HandRight };
    punchPattern.motionThreshold = 2.0f;
    punchPattern.timeWindow = 0.5f;
    gestureRecognizer_->AddPattern(punchPattern);
    
    // Define grab gesture
    GesturePattern grabPattern;
    grabPattern.name = "Grab";
    grabPattern.jointSequence = { JointType::HandRight, JointType::HandLeft };
    grabPattern.motionThreshold = 0.5f;
    grabPattern.timeWindow = 1.0f;
    gestureRecognizer_->AddPattern(grabPattern);
    
    // Define wave gesture
    GesturePattern wavePattern;
    wavePattern.name = "Wave";
    wavePattern.jointSequence = { JointType::HandRight };
    wavePattern.motionThreshold = 1.0f;
    wavePattern.timeWindow = 2.0f;
    gestureRecognizer_->AddPattern(wavePattern);
}

void MotionControlSystem::HandleGesture(const RecognizedGesture& gesture) {
    Logger::Info("Gesture recognized: " + gesture.name + " (confidence: " + std::to_string(gesture.confidence) + ")");
    
    // Handle different gesture types
    if (gesture.name == "Punch") {
        HandlePunchGesture(gesture);
    } else if (gesture.name == "Grab") {
        HandleGrabGesture(gesture);
    } else if (gesture.name == "Wave") {
        HandleWaveGesture(gesture);
    }
}

void MotionControlSystem::HandlePunchGesture(const RecognizedGesture& gesture) {
    // Trigger punch action
    MotionEvent event;
    event.type = MotionEventType::Punch;
    event.position = trackedJoints_[static_cast<int>(JointType::HandRight)];
    event.confidence = gesture.confidence;
    event.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    
    motionEvents_.push_back(event);
}

void MotionControlSystem::HandleGrabGesture(const RecognizedGesture& gesture) {
    // Trigger grab action
    MotionEvent event;
    event.type = MotionEventType::Grab;
    event.position = trackedJoints_[static_cast<int>(JointType::HandRight)];
    event.confidence = gesture.confidence;
    event.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    
    motionEvents_.push_back(event);
}

void MotionControlSystem::HandleWaveGesture(const RecognizedGesture& gesture) {
    // Trigger wave action
    MotionEvent event;
    event.type = MotionEventType::Wave;
    event.position = trackedJoints_[static_cast<int>(JointType::HandRight)];
    event.confidence = gesture.confidence;
    event.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    
    motionEvents_.push_back(event);
}

void MotionControlSystem::ApplyMotionFiltering(DirectX::XMFLOAT3& position) {
    // Apply motion filter to reduce noise
    motionFilter_.positions.push_back(position);
    
    if (motionFilter_.positions.size() > motionFilter_.windowSize) {
        motionFilter_.positions.erase(motionFilter_.positions.begin());
    }
    
    // Calculate weighted average
    DirectX::XMFLOAT3 filteredPosition(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    
    for (size_t i = 0; i < motionFilter_.positions.size(); i++) {
        float weight = (i < motionFilter_.weights.size()) ? motionFilter_.weights[i] : 1.0f;
        filteredPosition.x += motionFilter_.positions[i].x * weight;
        filteredPosition.y += motionFilter_.positions[i].y * weight;
        filteredPosition.z += motionFilter_.positions[i].z * weight;
        totalWeight += weight;
    }
    
    if (totalWeight > 0.0f) {
        filteredPosition.x /= totalWeight;
        filteredPosition.y /= totalWeight;
        filteredPosition.z /= totalWeight;
        position = filteredPosition;
    }
}

DirectX::XMFLOAT3 MotionControlSystem::CalculateAimDirection(const DirectX::XMFLOAT3& handPosition) {
    // Calculate aiming direction based on hand position
    // This is a simplified calculation - in reality, you'd use both hands and body orientation
    
    DirectX::XMFLOAT3 centerPosition(0.0f, 1.0f, 0.0f); // Assume center of screen/body
    
    DirectX::XMFLOAT3 direction;
    direction.x = (handPosition.x - centerPosition.x) * motionSensitivity_;
    direction.y = (handPosition.y - centerPosition.y) * motionSensitivity_;
    direction.z = 1.0f; // Forward direction
    
    // Normalize direction
    DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
    dir = DirectX::XMVector3Normalize(dir);
    DirectX::XMStoreFloat3(&direction, dir);
    
    return direction;
}

DirectX::XMFLOAT3 MotionControlSystem::LerpVector3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t) {
    return DirectX::XMFLOAT3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

void MotionControlSystem::UpdateMovementControls() {
    // Use body lean for movement
    DirectX::XMFLOAT3 spine = trackedJoints_[static_cast<int>(JointType::SpineBase)];
    DirectX::XMFLOAT3 head = trackedJoints_[static_cast<int>(JointType::Head)];
    
    // Calculate lean direction
    DirectX::XMFLOAT3 leanDirection;
    leanDirection.x = head.x - spine.x;
    leanDirection.y = 0.0f; // Don't use vertical lean for movement
    leanDirection.z = head.z - spine.z;
    
    // Apply movement based on lean
    if (abs(leanDirection.x) > 0.1f || abs(leanDirection.z) > 0.1f) {
        MotionEvent event;
        event.type = MotionEventType::Movement;
        event.direction = leanDirection;
        event.confidence = jointConfidence_[static_cast<int>(JointType::Head)];
        event.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
        
        motionEvents_.push_back(event);
    }
}

void MotionControlSystem::UpdateInteractionControls() {
    // Check for interaction gestures
    DirectX::XMFLOAT3 leftHand = trackedJoints_[static_cast<int>(JointType::HandLeft)];
    DirectX::XMFLOAT3 rightHand = trackedJoints_[static_cast<int>(JointType::HandRight)];
    
    // Check for pointing gesture
    if (rightHand.y > 1.2f && jointConfidence_[static_cast<int>(JointType::HandRight)] > 0.8f) {
        MotionEvent event;
        event.type = MotionEventType::Point;
        event.position = rightHand;
        event.confidence = jointConfidence_[static_cast<int>(JointType::HandRight)];
        event.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
        
        motionEvents_.push_back(event);
    }
}

// Public interface methods
DirectX::XMFLOAT3 MotionControlSystem::GetJointPosition(JointType joint) const {
    int index = static_cast<int>(joint);
    if (index >= 0 && index < static_cast<int>(JointType::Count)) {
        return trackedJoints_[index];
    }
    return DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
}

float MotionControlSystem::GetJointConfidence(JointType joint) const {
    int index = static_cast<int>(joint);
    if (index >= 0 && index < static_cast<int>(JointType::Count)) {
        return jointConfidence_[index];
    }
    return 0.0f;
}

MotionAimingData MotionControlSystem::GetAimingData() const {
    return aimingData_;
}

std::vector<MotionEvent> MotionControlSystem::GetMotionEvents() {
    std::vector<MotionEvent> events = motionEvents_;
    motionEvents_.clear(); // Clear events after retrieval
    return events;
}

void MotionControlSystem::SetMotionSensitivity(float sensitivity) {
    motionSensitivity_ = std::clamp(sensitivity, 0.1f, 5.0f);
}

void MotionControlSystem::SetAimingSmoothing(float smoothing) {
    aimingSmoothing_ = std::clamp(smoothing, 0.0f, 0.95f);
}

void MotionControlSystem::SetGestureThreshold(float threshold) {
    gestureThreshold_ = std::clamp(threshold, 0.1f, 1.0f);
}

void MotionControlSystem::StartCalibration() {
    calibrationMode_ = true;
    Logger::Info("Motion control calibration started");
}

void MotionControlSystem::StopCalibration() {
    calibrationMode_ = false;
    Logger::Info("Motion control calibration completed");
}

bool MotionControlSystem::IsCalibrating() const {
    return calibrationMode_;
}

// GestureRecognizer implementations
void GestureRecognizer::AddPattern(const GesturePattern& pattern) {
    patterns_.push_back(pattern);
}

std::vector<RecognizedGesture> GestureRecognizer::GetRecognizedGestures() const {
    return recognizedGestures_;
}

void GestureRecognizer::ProcessJoints(const std::vector<DirectX::XMFLOAT3>& joints) {
    currentJoints_ = joints;
    // Basic gesture processing logic would go here
}

void GestureRecognizer::Update(const std::vector<DirectX::XMFLOAT3>& joints, float deltaTime) {
    ProcessJoints(joints);
    
    // Simple gesture recognition logic
    recognizedGestures_.clear();
    
    // For now, just a placeholder implementation
    // Real gesture recognition would analyze joint movements over time
}

void GestureRecognizer::Reset() {
    recognizedGestures_.clear();
    currentJoints_.clear();
    gestureStartTime_ = 0.0f;
    isRecognizing_ = false;
}

} // namespace Nexus
