#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>

namespace Nexus {

class Engine;

class EngineErrorRecovery {
public:
    enum class ErrorSeverity {
        Info,
        Warning,
        Error,
        Critical
    };
    
    struct ErrorReport {
        std::string component;
        std::string description;
        ErrorSeverity severity;
        std::string suggestedFix;
        bool canAutoFix;
        std::function<bool()> autoFixFunction;
        float timestamp;
    };
    
    struct SystemHealth {
        bool isHealthy = true;
        float healthScore = 100.0f;
        std::vector<std::string> issues;
        std::vector<std::string> fixes;
    };
    
    using ErrorCallback = std::function<void(const ErrorReport&)>;
    using RecoveryCallback = std::function<void(const std::string&)>;
    
    EngineErrorRecovery();
    ~EngineErrorRecovery();
    
    bool Initialize(Engine* engine);
    void Shutdown();
    void Update(float deltaTime);
    
    void ReportError(const std::string& component, const std::string& description, 
                     ErrorSeverity severity, const std::string& suggestedFix = "");
    
    void RegisterAutoFix(const std::string& errorPattern, std::function<bool()> fixFunction);
    bool AttemptAutoFix(const std::string& component);
    bool AttemptAutoFixAll();
    
    SystemHealth CheckGraphicsHealth();
    SystemHealth CheckAudioHealth();
    SystemHealth CheckInputHealth();
    SystemHealth CheckPhysicsHealth();
    SystemHealth CheckOverallHealth();
    
    bool RestartGraphicsSystem();
    bool RestartAudioSystem();
    bool RestartInputSystem();
    bool ResetToDefaults();
    bool RecoverFromCrash();
    
    void SetErrorCallback(ErrorCallback callback) { errorCallback_ = callback; }
    void SetRecoveryCallback(RecoveryCallback callback) { recoveryCallback_ = callback; }
    
    const std::vector<ErrorReport>& GetErrorHistory() const { return errorHistory_; }
    
private:
    void PerformHealthCheck();
    void CheckForCommonIssues();
    void MonitorPerformance();
    
    // Auto-fix implementations
    bool FixGraphicsDeviceLost();
    bool FixAudioDeviceDisconnected();
    bool FixInputDeviceError();
    bool FixMemoryLeak();
    bool FixShaderCompilationError();
    bool FixTextureLoadingError();
    
    Engine* engine_;
    bool initialized_;
    float healthCheckTimer_;
    
    std::vector<ErrorReport> errorHistory_;
    std::vector<std::pair<std::string, std::function<bool()>>> autoFixes_;
    
    SystemHealth graphicsHealth_;
    SystemHealth audioHealth_;
    SystemHealth inputHealth_;
    SystemHealth physicsHealth_;
    SystemHealth overallHealth_;
    
    ErrorCallback errorCallback_;
    RecoveryCallback recoveryCallback_;
};

} // namespace Nexus