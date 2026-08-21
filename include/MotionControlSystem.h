#pragma once

#include "Platform.h"
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

// See AudioSystem.h: resolves to the SDK on Windows, to compat/win32 elsewhere.
#include <xinput.h>
#include <hidusage.h>
#include <hidsdi.h>

#include <DirectXMath.h>

namespace Nexus {

// Forward declarations
class Camera;
class InputManager;

// Kinect-specific types and enums
enum class JointType {
    SpineBase = 0,
    SpineMid,
    SpineTop,
    Neck,
    Head,
    ShoulderLeft,
    ElbowLeft,
    WristLeft,
    HandLeft,
    ShoulderRight,
    ElbowRight,
    WristRight,
    HandRight,
    HipLeft,
    KneeLeft,
    AnkleLeft,
    FootLeft,
    HipRight,
    KneeRight,
    AnkleRight,
    FootRight,
    SpineShoulder,
    HandTipLeft,
    ThumbLeft,
    HandTipRight,
    ThumbRight,
    Count
};

enum class GestureType {
    Point,
    Swipe,
    Circle,
    Shake,
    Tilt,
    Roll,
    Pitch,
    Yaw,
    Custom
};

enum class MotionEventType {
    Punch,
    Grab,
    Wave,
    Point,
    Swipe,
    Circle,
    Movement,
    Custom
};

struct RecognizedGesture {
    std::string name;
    GestureType type;
    float confidence;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 direction;
    float duration;
    std::vector<JointType> jointSequence;
};

struct GesturePattern {
    std::string name;
    GestureType type;
    std::vector<JointType> jointSequence;
    std::vector<DirectX::XMFLOAT3> keyPositions;
    float minDuration;
    float maxDuration;
    float threshold;
    float motionThreshold;
    float timeWindow;
};

struct MotionEvent {
    MotionEventType type;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 direction;
    float intensity;
    float confidence;
    float timestamp;
};

struct MotionAimingData {
    DirectX::XMFLOAT2 screenPosition;
    DirectX::XMFLOAT3 worldPosition;
    DirectX::XMFLOAT3 direction;
    float confidence;
    bool isValid;
    bool isActive;
};

// Kinect-specific gesture recognizer
class GestureRecognizer {
public:
    void AddPattern(const GesturePattern& pattern);
    std::vector<RecognizedGesture> GetRecognizedGestures() const;
    void ProcessJoints(const std::vector<DirectX::XMFLOAT3>& joints);
    void Update(const std::vector<DirectX::XMFLOAT3>& joints, float deltaTime);
    void Reset();

private:
    std::vector<GesturePattern> patterns_;
    std::vector<RecognizedGesture> recognizedGestures_;
    std::vector<DirectX::XMFLOAT3> currentJoints_;
    float gestureStartTime_;
    bool isRecognizing_;
};

class MotionControlSystem {
public:
    enum class MotionDeviceType {
        Mouse,
        Gamepad,
        Gyroscope,
        Accelerometer,
        WiiRemote,
        PlayStation_Move,
        Kinect,
        VR_Controller,
        Custom
    };

    enum class AimingMode {
        Traditional,
        MotionAssisted,
        MotionPrimary,
        Hybrid,
        Gesture
    };

    struct MotionDevice {
        std::string name;
        MotionDeviceType type;
        bool isConnected;
        bool isCalibrated;

        bool hasGyroscope;
        bool hasAccelerometer;
        bool hasMagnetometer;
        bool hasPositionTracking;
        bool hasOrientationTracking;
        bool hasButtonInput;

        DirectX::XMFLOAT3 acceleration;
        DirectX::XMFLOAT3 gyroscope;
        DirectX::XMFLOAT3 magnetometer;
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 orientation;

        DirectX::XMFLOAT3 velocity;
        DirectX::XMFLOAT3 angularVelocity;
        DirectX::XMFLOAT3 worldPosition;
        DirectX::XMFLOAT4 worldOrientation;

        union {
            struct {
                int deltaX, deltaY;
                int wheel;
                bool buttons[8];
            } mouse;

            struct {
                XINPUT_STATE state;
                XINPUT_VIBRATION vibration;
            } gamepad;

            struct {
                DirectX::XMFLOAT3 pointer;
                bool trigger;
                bool buttons[16];
            } wiiRemote;

            struct {
                DirectX::XMFLOAT3 position;
                DirectX::XMFLOAT4 orientation;
                bool trigger;
                float trigger_value;
            } vrController;
        };

        DirectX::XMFLOAT3 calibrationOffset;
        DirectX::XMFLOAT4 calibrationRotation;
        float calibrationScale;

        float lowPassFilterAlpha;
        float highPassFilterAlpha;
        bool useKalmanFilter;

        float sensitivity;
        float deadZone;
        bool invertX;
        bool invertY;

        float updateFrequency;
        float lastUpdateTime;
        int sampleCount;
    };

    struct AimingProfile {
        std::string name;
        AimingMode mode;

        float mouseSensitivity;
        float gamepadSensitivity;
        float motionSensitivity;
        DirectX::XMFLOAT3 motionScale;

        float smoothingFactor;
        float accelerationCurve;
        float decelerationCurve;
        bool enableSmoothing;
        bool enableAcceleration;
        bool enablePrediction;

        float innerDeadZone;
        float outerDeadZone;
        float motionDeadZone;

        bool enableAimAssist;
        bool enableAutoAim;
        bool enableStabilization;
        bool enableGestureRecognition;

        float aimAssistStrength;
        float aimAssistRadius;
        float aimAssistSlowdownFactor;
        float autoAimSnapStrength;
        float autoAimFollowStrength;

        float stabilizationStrength;
        float stabilizationThreshold;
        float stabilizationDecay;

        float motionToPixelRatio;
        float motionFilterCutoff;
        bool useAbsoluteMotion;
        bool useRelativeMotion;
        DirectX::XMFLOAT3 motionAxisMapping;

        float gestureThreshold;
        float gestureTimeout;
        int gestureMinSamples;

        bool enableComfortMode;
        float comfortTurnSpeed;
        float comfortSnapTurnAngle;

        bool enableOneHandedMode;
        bool enableReducedMotion;
        float tremorCompensation;
    };

    struct MotionFilter {
        enum class FilterType {
            None,
            LowPass,
            HighPass,
            Bandpass,
            Kalman,
            Complementary,
            Mahony,
            Madgwick
        };

        FilterType type;
        float cutoffFrequency;
        float sampleRate;

        float alpha;
        float beta;
        float gamma;

        std::vector<DirectX::XMFLOAT3> history;
        int historySize;

        DirectX::XMFLOAT4X4 stateMatrix;
        DirectX::XMFLOAT4X4 covarianceMatrix;
        DirectX::XMFLOAT4X4 processNoise;
        DirectX::XMFLOAT4X4 measurementNoise;

        DirectX::XMFLOAT3 Filter(const DirectX::XMFLOAT3& input);
        DirectX::XMFLOAT4 FilterOrientation(const DirectX::XMFLOAT4& input);
        void Reset();
    };

    struct AimTarget {
        DirectX::XMFLOAT3 worldPosition;
        DirectX::XMFLOAT2 screenPosition;
        DirectX::XMFLOAT3 velocity;
        float distance;
        float priority;
        float radius;
        bool isActive;
        bool isLocked;

        DirectX::XMFLOAT3 predictedPosition;
        float predictionTime;

        float highlightStrength;
        float lockOnStrength;

        bool IsInRange(const DirectX::XMFLOAT2& aimPosition, float radius) const;
        DirectX::XMFLOAT3 GetPredictedPosition(float time) const;
    };

    struct AimAssistSystem {
        bool isEnabled;
        std::vector<AimTarget> targets;
        AimTarget* currentTarget;

        float magnetismStrength;
        float magnetismRadius;
        float slowdownFactor;
        float slowdownRadius;
        float snapStrength;
        float followStrength;

        bool enablePrediction;
        float predictionTime;
        float predictionAccuracy;

        enum class TargetSelectionMode {
            Closest,
            MostCentral,
            HighestPriority,
            PlayerGaze,
            Hybrid
        };

        TargetSelectionMode selectionMode;

        void UpdateTargets(const std::vector<AimTarget>& newTargets);
        AimTarget* SelectBestTarget(const DirectX::XMFLOAT2& aimPosition,
                                   const DirectX::XMFLOAT2& lookDirection);
        DirectX::XMFLOAT2 ApplyMagnetism(const DirectX::XMFLOAT2& input, const DirectX::XMFLOAT2& aimPosition);
        DirectX::XMFLOAT2 ApplySlowdown(const DirectX::XMFLOAT2& input, const DirectX::XMFLOAT2& aimPosition);
        DirectX::XMFLOAT2 ApplySnap(const DirectX::XMFLOAT2& aimPosition);
        DirectX::XMFLOAT2 ApplyFollow(const DirectX::XMFLOAT2& aimPosition, float deltaTime);
    };

    struct MotionCalibration {
        bool isCalibrated;
        DirectX::XMFLOAT3 neutralAcceleration;
        DirectX::XMFLOAT3 neutralGyroscope;
        DirectX::XMFLOAT4 neutralOrientation;

        enum class CalibrationStep {
            PlaceFlat,
            HoldStill,
            SlowRotation,
            Complete
        };

        CalibrationStep currentStep;
        float calibrationProgress;
        std::vector<DirectX::XMFLOAT3> calibrationSamples;

        void StartCalibration();
        void UpdateCalibration(const MotionDevice& device);
        void CompleteCalibration();
        void ApplyCalibration(MotionDevice& device);
    };

public:
    MotionControlSystem();
    ~MotionControlSystem();

    bool Initialize();
    bool InitializeAdvanced(const std::string& configFile = "");
    void Shutdown();
    bool Reset();

    bool DetectDevices();
    std::vector<MotionDevice> GetAvailableDevices() const;
    bool ConnectDevice(const std::string& deviceName);
    void DisconnectDevice(const std::string& deviceName);
    MotionDevice* GetDevice(const std::string& deviceName);

    void CreateAimingProfile(const std::string& name, const AimingProfile& profile);
    void SetActiveProfile(const std::string& profileName);
    AimingProfile* GetActiveProfile();
    void SaveProfile(const std::string& profileName, const std::string& filePath);
    void LoadProfile(const std::string& profileName, const std::string& filePath);

    DirectX::XMFLOAT2 GetAimInput();
    DirectX::XMFLOAT2 GetRawMotionInput();
    DirectX::XMFLOAT2 GetProcessedMotionInput();
    void SetAimingMode(AimingMode mode);
    void SetSensitivity(float mouseSens, float gamepadSens, float motionSens);

    void RegisterGesture(const GestureRecognizer& gesture);
    void UnregisterGesture(const std::string& gestureName);
    void EnableGestureRecognition(bool enable);
    void StartGestureRecording(const std::string& gestureName);
    void StopGestureRecording();
    std::vector<GestureRecognizer> GetRecognizedGestures() const;

    void EnableAimAssist(bool enable);
    void SetAimAssistParameters(float strength, float radius, float slowdown);
    void UpdateAimTargets(const std::vector<AimTarget>& targets);
    AimTarget* GetCurrentTarget() const;
    DirectX::XMFLOAT2 GetAimAssistInput(const DirectX::XMFLOAT2& rawInput);

    void SetMotionFilter(const std::string& deviceName, const MotionFilter& filter);
    void EnableMotionSmoothing(bool enable);
    void SetSmoothingFactor(float factor);

    void StartCalibration();
    void UpdateCalibration();
    bool IsCalibrationComplete(const std::string& deviceName) const;
    void SaveCalibration(const std::string& deviceName, const std::string& filePath);
    void LoadCalibration(const std::string& deviceName, const std::string& filePath);

    void SetCamera(Camera* camera);
    void ApplyMotionToCamera(float deltaTime);
    void EnableCameraStabilization(bool enable);

    void SetHapticFeedback(const std::string& deviceName, float intensity, float duration);
    void PlayHapticPattern(const std::string& deviceName, const std::string& patternName);

    void StartRecording(const std::string& recordingName);
    void StopRecording();
    void PlaybackRecording(const std::string& recordingName);
    void SaveRecording(const std::string& recordingName, const std::string& filePath);
    void LoadRecording(const std::string& recordingName, const std::string& filePath);

    void GetMotionStatistics(const std::string& deviceName,
                           float& avgAcceleration, float& avgRotation,
                           float& jitter, float& stability);
    void EnablePerformanceLogging(bool enable);
    void ExportMotionData(const std::string& filePath);

    void EnableTremorCompensation(bool enable);
    void SetTremorCompensationStrength(float strength);
    void EnableOneHandedMode(bool enable);
    void EnableReducedMotion(bool enable);

    void Update(float deltaTime);
    void ProcessMotionInput();
    void UpdateAimAssist();

    void EnableDebugVisualization(bool enable);
    void DrawMotionData();
    void DrawGestureRecognition();
    void DrawAimAssist();

    void SetOnGestureRecognized(std::function<void(const std::string&)> callback);
    void SetOnDeviceConnected(std::function<void(const std::string&)> callback);
    void SetOnDeviceDisconnected(std::function<void(const std::string&)> callback);
    void SetOnCalibrationComplete(std::function<void(const std::string&)> callback);

    DirectX::XMFLOAT3 GetJointPosition(JointType joint) const;
    float GetJointConfidence(JointType joint) const;
    MotionAimingData GetAimingData() const;
    std::vector<MotionEvent> GetMotionEvents();
    void SetMotionSensitivity(float sensitivity);
    void SetAimingSmoothing(float smoothing);
    void SetGestureThreshold(float threshold);
    void StopCalibration();
    bool IsCalibrating() const;

    enum class TrackingQuality {
        Low,
        Medium,
        High,
        Ultra
    };

    bool InitializeFallbackTracking();
    void SetTrackingQuality(TrackingQuality quality);
    void ConfigureFilteringByQuality(TrackingQuality quality);
    void UpdateCalibrationRange(const DirectX::XMFLOAT3& head,
                              const DirectX::XMFLOAT3& rightHand,
                              const DirectX::XMFLOAT3& leftHand);
    void FinalizeCalibration();
    void AttemptSystemRecovery();

private:
    void UpdateMouseInput();
    void UpdateGamepadInput();
    void UpdateGyroscopeInput();
    void UpdateAccelerometerInput();
    void UpdateWiiRemoteInput();
    void UpdateVRControllerInput();

    DirectX::XMFLOAT3 ProcessAcceleration(const DirectX::XMFLOAT3& raw);
    DirectX::XMFLOAT3 ProcessGyroscope(const DirectX::XMFLOAT3& raw);
    DirectX::XMFLOAT4 ProcessOrientation(const DirectX::XMFLOAT4& raw);
    DirectX::XMFLOAT2 ConvertMotionToAiming(const DirectX::XMFLOAT3& motion);
    void ApplyWeightedFiltering(DirectX::XMFLOAT3& position, float responsiveness);
    void ApplyPredictiveFiltering(DirectX::XMFLOAT3& position);
    void ApplyJitterReduction(DirectX::XMFLOAT3& position);

    void UpdateGestureRecording();
    void ProcessGestureRecognition();
    float CalculateGestureDistance(const std::vector<DirectX::XMFLOAT3>& points1,
                                  const std::vector<DirectX::XMFLOAT3>& points2);

    void UpdateAimAssistTargets();
    AimTarget* SelectBestTarget(const DirectX::XMFLOAT2& currentAim);
    DirectX::XMFLOAT2 ApplyAimAssist(const DirectX::XMFLOAT2& rawInput);

    DirectX::XMFLOAT2 WorldToScreen(const DirectX::XMFLOAT3& worldPos);
    DirectX::XMFLOAT3 ScreenToWorld(const DirectX::XMFLOAT2& screenPos);
    float CalculateDistance(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b);
    DirectX::XMFLOAT3 ApplyDeadZone(const DirectX::XMFLOAT3& input, float deadZone);
    DirectX::XMFLOAT2 ApplyDeadZone(const DirectX::XMFLOAT2& input, float deadZone);

#ifdef _WIN32
    void InitializeXInput();
    void InitializeRawInput();
    void InitializeHID();
    void ProcessXInputDevice(int deviceIndex);
    void ProcessRawInputDevice(HRAWINPUT hRawInput);
#endif

    bool InitializeKinect();
    bool InitializeGestureRecognition();
    bool InitializeMotionTracking();
    bool UpdateBodyTracking();
    void UpdateGestureRecognition(float deltaTime);
    void UpdateMotionAiming(float deltaTime);
    void UpdateMotionControls(float deltaTime);
    void HandleGesture(const RecognizedGesture& gesture);
    void HandlePunchGesture(const RecognizedGesture& gesture);
    void HandleGrabGesture(const RecognizedGesture& gesture);
    void HandleWaveGesture(const RecognizedGesture& gesture);
    void DefineGesturePatterns();
    void ApplyMotionFiltering(DirectX::XMFLOAT3& motion);
    void ApplySkeletalConstraints(std::vector<DirectX::XMFLOAT3>& joints);
    DirectX::XMFLOAT3 CalculateAimDirection(const DirectX::XMFLOAT3& handPosition);
    DirectX::XMFLOAT3 LerpVector3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t);
    bool AttemptTrackingRecovery();
    float CalculateJointConfidence(const DirectX::XMFLOAT3& current, const DirectX::XMFLOAT3& previous, float deltaTime);
    void UpdateMovementControls();
    void UpdateInteractionControls();
    void HandleTrackingLoss();
    bool ValidateJointPositions(const std::vector<DirectX::XMFLOAT3>& joints);
    void UpdateCalibration(float deltaTime);
    void InitializeSkeletalConstraints();
    void ApplySkeletalConstraints();

private:
    std::map<std::string, MotionDevice> devices_;
    std::vector<std::string> connectedDevices_;

    std::map<std::string, AimingProfile> profiles_;
    std::string activeProfile_;

    std::map<std::string, GestureRecognizer> gestures_;
    std::string recordingGesture_;
    bool gestureRecognitionEnabled_;

    std::unique_ptr<AimAssistSystem> aimAssist_;

    std::map<std::string, MotionFilter> filters_;

    std::map<std::string, MotionCalibration> calibrations_;

    Camera* camera_;
    bool cameraStabilizationEnabled_;

    struct MotionRecording {
        std::string name;
        std::vector<std::pair<float, MotionDevice>> frames;
        float duration;
        bool isRecording;
        bool isPlaying;
        float playbackTime;
    };

    std::map<std::string, MotionRecording> recordings_;
    std::string activeRecording_;

    struct MotionStatistics {
        float avgAcceleration;
        float avgRotation;
        float jitter;
        float stability;
        float updateFrequency;
    };

    std::map<std::string, MotionStatistics> statistics_;

    bool tremorCompensationEnabled_;
    float tremorCompensationStrength_;
    bool oneHandedModeEnabled_;
    bool reducedMotionEnabled_;

    bool debugVisualizationEnabled_;
    bool performanceLoggingEnabled_;

    std::function<void(const std::string&)> onGestureRecognized_;
    std::function<void(const std::string&)> onDeviceConnected_;
    std::function<void(const std::string&)> onDeviceDisconnected_;
    std::function<void(const std::string&)> onCalibrationComplete_;

    std::thread motionThread_;
    std::mutex motionMutex_;
    std::atomic<bool> isRunning_;

    InputManager* inputManager_;

    float lastUpdateTime_;
    float updateFrequency_;
    int frameCount_;

    struct SkeletalConstraint {
        JointType jointA;
        JointType jointB;
        float minDistance;
        float maxDistance;
    };

#ifdef _WIN32
    HWND windowHandle_;
    std::vector<RAWINPUTDEVICE> rawInputDevices_;
    std::map<HANDLE, std::string> hidDevices_;
#endif

    void* kinectSensor_;
    void* coordinateMapper_;
    void* bodyFrameReader_;
    GestureRecognizer* gestureRecognizer_;
    bool isInitialized_;
    bool calibrationMode_;
    float motionSensitivity_;
    float aimingSmoothing_;
    float gestureThreshold_;

    std::vector<DirectX::XMFLOAT3> trackedJoints_;
    std::vector<float> jointConfidence_;

    struct MotionFilterData {
        int windowSize;
        std::vector<DirectX::XMFLOAT3> history;
        std::vector<DirectX::XMFLOAT3> positions;
        std::vector<DirectX::XMFLOAT3> velocities;
        std::vector<float> timestamps;
        std::vector<float> weights;
        DirectX::XMFLOAT3 filteredValue;
    };
    MotionFilterData motionFilter_;

    MotionAimingData aimingData_;
    DirectX::XMFLOAT3 currentAimDirection_;

    std::vector<MotionEvent> motionEvents_;

    unsigned int trackingErrorCount_;
    unsigned int consecutiveTrackingFailures_;
    float lastSuccessfulTrackingTime_;
    bool isTrackingLost_;
    std::vector<DirectX::XMFLOAT3> lastValidJoints_;
    float trackingRecoveryTimeout_;
    float trackingConfidenceThreshold_;
    bool useJointPrediction_;

    static constexpr unsigned int MAX_ERROR_THRESHOLD = 10;
    static constexpr unsigned int MAX_TRACKING_FAILURES = 5;

    std::vector<std::vector<DirectX::XMFLOAT3>> jointHistory_;
    std::vector<DirectX::XMFLOAT3> jointVelocities_;

    float lastFrameTime_;

    std::vector<SkeletalConstraint> skeletalConstraints_;
    TrackingQuality trackingQuality_;
    bool trackingEnabled_;
    unsigned int errorCount_;
};

} // namespace Nexus
