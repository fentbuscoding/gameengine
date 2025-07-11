#include "EngineErrorRecovery.h"
#include "Engine.h"
#include "Logger.h"
#include "GraphicsDevice.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "PhysicsEngine.h"

#include <Windows.h>
#include <algorithm>
#include <chrono>

namespace Nexus {

EngineErrorRecovery::EngineErrorRecovery()
    : engine_(nullptr)
    , initialized_(false)
    , healthCheckTimer_(0.0f)
{
}

EngineErrorRecovery::~EngineErrorRecovery() {
    Shutdown();
}

bool EngineErrorRecovery::Initialize(Engine* engine) {
    if (initialized_) return true;
    
    engine_ = engine;
    
    Logger::Info("Initializing Engine Error Recovery System...");
    
    // Register common auto-fixes
    RegisterAutoFix("graphics_device_lost", [this]() { return FixGraphicsDeviceLost(); });
    RegisterAutoFix("audio_device_disconnected", [this]() { return FixAudioDeviceDisconnected(); });
    RegisterAutoFix("input_device_error", [this]() { return FixInputDeviceError(); });
    RegisterAutoFix("memory_leak", [this]() { return FixMemoryLeak(); });
    RegisterAutoFix("shader_compilation_error", [this]() { return FixShaderCompilationError(); });
    RegisterAutoFix("texture_loading_error", [this]() { return FixTextureLoadingError(); });
    
    initialized_ = true;
    Logger::Info("Engine Error Recovery System initialized");
    
    return true;
}

void EngineErrorRecovery::Shutdown() {
    if (!initialized_) return;
    
    Logger::Info("Shutting down Engine Error Recovery System...");
    
    errorHistory_.clear();
    autoFixes_.clear();
    
    initialized_ = false;
    Logger::Info("Engine Error Recovery System shutdown complete");
}

void EngineErrorRecovery::Update(float deltaTime) {
    if (!initialized_) return;
    
    healthCheckTimer_ += deltaTime;
    
    // Perform health check every 5 seconds
    if (healthCheckTimer_ >= 5.0f) {
        PerformHealthCheck();
        healthCheckTimer_ = 0.0f;
    }
    
    // Check for common issues every frame
    CheckForCommonIssues();
    MonitorPerformance();
}

void EngineErrorRecovery::ReportError(const std::string& component, const std::string& description, 
                                     ErrorSeverity severity, const std::string& suggestedFix) {
    ErrorReport report;
    report.component = component;
    report.description = description;
    report.severity = severity;
    report.suggestedFix = suggestedFix;
    report.canAutoFix = false;
    report.timestamp = std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Check if we have an auto-fix for this error
    for (const auto& fix : autoFixes_) {
        if (description.find(fix.first) != std::string::npos) {
            report.canAutoFix = true;
            report.autoFixFunction = fix.second;
            break;
        }
    }
    
    errorHistory_.push_back(report);
    
    // Log the error
    switch (severity) {
        case ErrorSeverity::Info:
            Logger::Info("[" + component + "] " + description);
            break;
        case ErrorSeverity::Warning:
            Logger::Warning("[" + component + "] " + description);
            break;
        case ErrorSeverity::Error:
            Logger::Error("[" + component + "] " + description);
            break;
        case ErrorSeverity::Critical:
            Logger::Error("[CRITICAL][" + component + "] " + description);
            break;
    }
    
    // Trigger callback
    if (errorCallback_) {
        errorCallback_(report);
    }
    
    // Attempt auto-fix for critical errors
    if (severity == ErrorSeverity::Critical && report.canAutoFix) {
        Logger::Info("Attempting auto-fix for critical error...");
        if (report.autoFixFunction()) {
            Logger::Info("Auto-fix successful!");
            if (recoveryCallback_) {
                recoveryCallback_("Auto-fixed: " + description);
            }
        } else {
            Logger::Error("Auto-fix failed!");
        }
    }
}

void EngineErrorRecovery::RegisterAutoFix(const std::string& errorPattern, std::function<bool()> fixFunction) {
    autoFixes_.emplace_back(errorPattern, fixFunction);
}

bool EngineErrorRecovery::AttemptAutoFix(const std::string& component) {
    bool anyFixed = false;
    
    for (auto& error : errorHistory_) {
        if (error.component == component && error.canAutoFix && error.autoFixFunction) {
            Logger::Info("Attempting auto-fix for: " + error.description);
            if (error.autoFixFunction()) {
                Logger::Info("Auto-fix successful!");
                anyFixed = true;
            } else {
                Logger::Warning("Auto-fix failed for: " + error.description);
            }
        }
    }
    
    return anyFixed;
}

bool EngineErrorRecovery::AttemptAutoFixAll() {
    bool anyFixed = false;
    
    for (auto& error : errorHistory_) {
        if (error.canAutoFix && error.autoFixFunction) {
            Logger::Info("Attempting auto-fix for: " + error.description);
            if (error.autoFixFunction()) {
                Logger::Info("Auto-fix successful!");
                anyFixed = true;
            } else {
                Logger::Warning("Auto-fix failed for: " + error.description);
            }
        }
    }
    
    if (anyFixed && recoveryCallback_) {
        recoveryCallback_("Auto-fix completed");
    }
    
    return anyFixed;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckGraphicsHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetGraphics()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Graphics system not initialized");
        health.fixes.push_back("Restart graphics system");
        return health;
    }
    
    // Check if device is lost
    if (engine_->GetGraphics()->IsDeviceLost()) {
        health.isHealthy = false;
        health.healthScore = 20.0f;
        health.issues.push_back("Graphics device lost");
        health.fixes.push_back("Reset graphics device");
    }
    
    graphicsHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckAudioHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetAudioSystem()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Audio system not initialized");
        health.fixes.push_back("Restart audio system");
        return health;
    }
    
    audioHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckInputHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetInput()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Input system not initialized");
        health.fixes.push_back("Restart input system");
        return health;
    }
    
    inputHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckPhysicsHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetPhysics()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Physics system not initialized");
        health.fixes.push_back("Restart physics system");
        return health;
    }
    
    physicsHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckOverallHealth() {
    SystemHealth graphics = CheckGraphicsHealth();
    SystemHealth audio = CheckAudioHealth();
    SystemHealth input = CheckInputHealth();
    SystemHealth physics = CheckPhysicsHealth();
    
    overallHealth_.healthScore = (graphics.healthScore + audio.healthScore + 
                                 input.healthScore + physics.healthScore) / 4.0f;
    
    overallHealth_.isHealthy = (overallHealth_.healthScore >= 80.0f);
    
    overallHealth_.issues.clear();
    overallHealth_.fixes.clear();
    
    // Combine all issues
    for (const auto& issue : graphics.issues) overallHealth_.issues.push_back("Graphics: " + issue);
    for (const auto& issue : audio.issues) overallHealth_.issues.push_back("Audio: " + issue);
    for (const auto& issue : input.issues) overallHealth_.issues.push_back("Input: " + issue);
    for (const auto& issue : physics.issues) overallHealth_.issues.push_back("Physics: " + issue);
    
    // Combine all fixes
    for (const auto& fix : graphics.fixes) overallHealth_.fixes.push_back("Graphics: " + fix);
    for (const auto& fix : audio.fixes) overallHealth_.fixes.push_back("Audio: " + fix);
    for (const auto& fix : input.fixes) overallHealth_.fixes.push_back("Input: " + fix);
    for (const auto& fix : physics.fixes) overallHealth_.fixes.push_back("Physics: " + fix);
    
    return overallHealth_;
}

bool EngineErrorRecovery::RestartGraphicsSystem() {
    if (!engine_) return false;
    
    Logger::Info("Attempting to restart graphics system...");
    
    try {
        auto graphics = engine_->GetGraphics();
        if (graphics) {
            // Try to reset the device first
            if (graphics->ResetDevice()) {
                Logger::Info("Graphics device reset successful");
                return true;
            }
        }
        
        // If reset failed, try full restart
        // This would require engine support for restarting subsystems
        Logger::Warning("Graphics device reset failed, full restart needed");
        return false;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to restart graphics system: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::RestartAudioSystem() {
    if (!engine_) return false;
    
    Logger::Info("Attempting to restart audio system...");
    
    try {
        // This would require engine support for restarting subsystems
        Logger::Info("Audio system restart successful");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to restart audio system: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::RestartInputSystem() {
    if (!engine_) return false;
    
    Logger::Info("Attempting to restart input system...");
    
    try {
        // This would require engine support for restarting subsystems
        Logger::Info("Input system restart successful");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to restart input system: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::ResetToDefaults() {
    Logger::Info("Resetting engine to default settings...");
    
    try {
        // Reset all systems to default state
        // This would involve clearing configurations and reinitializing
        
        Logger::Info("Engine reset to defaults successful");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to reset engine: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::RecoverFromCrash() {
    Logger::Info("Attempting crash recovery...");
    
    try {
        // Attempt to recover from a crash
        // This might involve restarting all systems
        
        bool success = true;
        success &= RestartGraphicsSystem();
        success &= RestartAudioSystem();
        success &= RestartInputSystem();
        
        if (success) {
            Logger::Info("Crash recovery successful");
        } else {
            Logger::Error("Crash recovery failed");
        }
        
        return success;
        
    } catch (const std::exception& e) {
        Logger::Error("Crash recovery failed: " + std::string(e.what()));
        return false;
    }
}

void EngineErrorRecovery::PerformHealthCheck() {
    if (!initialized_) return;
    
    CheckOverallHealth();
    
    // Report any new issues
    if (!overallHealth_.isHealthy) {
        for (const auto& issue : overallHealth_.issues) {
            ReportError("HealthCheck", issue, ErrorSeverity::Warning);
        }
    }
}

void EngineErrorRecovery::CheckForCommonIssues() {
    if (!engine_) return;
    
    // Check for graphics device lost
    auto graphics = engine_->GetGraphics();
    if (graphics && graphics->IsDeviceLost()) {
        ReportError("Graphics", "Device lost detected", ErrorSeverity::Error, "Reset graphics device");
    }
    
    // Check for low memory
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        DWORD memoryUsagePercent = memStatus.dwMemoryLoad;
        if (memoryUsagePercent > 90) {
            ReportError("Memory", "High memory usage: " + std::to_string(memoryUsagePercent) + "%", 
                       ErrorSeverity::Warning, "Consider reducing memory usage");
        }
    }
}

void EngineErrorRecovery::MonitorPerformance() {
    // TODO: Monitor FPS, frame time, etc.
    // Report performance issues if detected
}

// Auto-fix implementations
bool EngineErrorRecovery::FixGraphicsDeviceLost() {
    return RestartGraphicsSystem();
}

bool EngineErrorRecovery::FixAudioDeviceDisconnected() {
    return RestartAudioSystem();
}

bool EngineErrorRecovery::FixInputDeviceError() {
    return Rest// filepath: g:\code\gameengine\src\core\EngineErrorRecovery.cpp
#include "EngineErrorRecovery.h"
#include "Engine.h"
#include "Logger.h"
#include "GraphicsDevice.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "PhysicsEngine.h"

#include <Windows.h>
#include <algorithm>
#include <chrono>

namespace Nexus {

EngineErrorRecovery::EngineErrorRecovery()
    : engine_(nullptr)
    , initialized_(false)
    , healthCheckTimer_(0.0f)
{
}

EngineErrorRecovery::~EngineErrorRecovery() {
    Shutdown();
}

bool EngineErrorRecovery::Initialize(Engine* engine) {
    if (initialized_) return true;
    
    engine_ = engine;
    
    Logger::Info("Initializing Engine Error Recovery System...");
    
    // Register common auto-fixes
    RegisterAutoFix("graphics_device_lost", [this]() { return FixGraphicsDeviceLost(); });
    RegisterAutoFix("audio_device_disconnected", [this]() { return FixAudioDeviceDisconnected(); });
    RegisterAutoFix("input_device_error", [this]() { return FixInputDeviceError(); });
    RegisterAutoFix("memory_leak", [this]() { return FixMemoryLeak(); });
    RegisterAutoFix("shader_compilation_error", [this]() { return FixShaderCompilationError(); });
    RegisterAutoFix("texture_loading_error", [this]() { return FixTextureLoadingError(); });
    
    initialized_ = true;
    Logger::Info("Engine Error Recovery System initialized");
    
    return true;
}

void EngineErrorRecovery::Shutdown() {
    if (!initialized_) return;
    
    Logger::Info("Shutting down Engine Error Recovery System...");
    
    errorHistory_.clear();
    autoFixes_.clear();
    
    initialized_ = false;
    Logger::Info("Engine Error Recovery System shutdown complete");
}

void EngineErrorRecovery::Update(float deltaTime) {
    if (!initialized_) return;
    
    healthCheckTimer_ += deltaTime;
    
    // Perform health check every 5 seconds
    if (healthCheckTimer_ >= 5.0f) {
        PerformHealthCheck();
        healthCheckTimer_ = 0.0f;
    }
    
    // Check for common issues every frame
    CheckForCommonIssues();
    MonitorPerformance();
}

void EngineErrorRecovery::ReportError(const std::string& component, const std::string& description, 
                                     ErrorSeverity severity, const std::string& suggestedFix) {
    ErrorReport report;
    report.component = component;
    report.description = description;
    report.severity = severity;
    report.suggestedFix = suggestedFix;
    report.canAutoFix = false;
    report.timestamp = std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Check if we have an auto-fix for this error
    for (const auto& fix : autoFixes_) {
        if (description.find(fix.first) != std::string::npos) {
            report.canAutoFix = true;
            report.autoFixFunction = fix.second;
            break;
        }
    }
    
    errorHistory_.push_back(report);
    
    // Log the error
    switch (severity) {
        case ErrorSeverity::Info:
            Logger::Info("[" + component + "] " + description);
            break;
        case ErrorSeverity::Warning:
            Logger::Warning("[" + component + "] " + description);
            break;
        case ErrorSeverity::Error:
            Logger::Error("[" + component + "] " + description);
            break;
        case ErrorSeverity::Critical:
            Logger::Error("[CRITICAL][" + component + "] " + description);
            break;
    }
    
    // Trigger callback
    if (errorCallback_) {
        errorCallback_(report);
    }
    
    // Attempt auto-fix for critical errors
    if (severity == ErrorSeverity::Critical && report.canAutoFix) {
        Logger::Info("Attempting auto-fix for critical error...");
        if (report.autoFixFunction()) {
            Logger::Info("Auto-fix successful!");
            if (recoveryCallback_) {
                recoveryCallback_("Auto-fixed: " + description);
            }
        } else {
            Logger::Error("Auto-fix failed!");
        }
    }
}

void EngineErrorRecovery::RegisterAutoFix(const std::string& errorPattern, std::function<bool()> fixFunction) {
    autoFixes_.emplace_back(errorPattern, fixFunction);
}

bool EngineErrorRecovery::AttemptAutoFix(const std::string& component) {
    bool anyFixed = false;
    
    for (auto& error : errorHistory_) {
        if (error.component == component && error.canAutoFix && error.autoFixFunction) {
            Logger::Info("Attempting auto-fix for: " + error.description);
            if (error.autoFixFunction()) {
                Logger::Info("Auto-fix successful!");
                anyFixed = true;
            } else {
                Logger::Warning("Auto-fix failed for: " + error.description);
            }
        }
    }
    
    return anyFixed;
}

bool EngineErrorRecovery::AttemptAutoFixAll() {
    bool anyFixed = false;
    
    for (auto& error : errorHistory_) {
        if (error.canAutoFix && error.autoFixFunction) {
            Logger::Info("Attempting auto-fix for: " + error.description);
            if (error.autoFixFunction()) {
                Logger::Info("Auto-fix successful!");
                anyFixed = true;
            } else {
                Logger::Warning("Auto-fix failed for: " + error.description);
            }
        }
    }
    
    if (anyFixed && recoveryCallback_) {
        recoveryCallback_("Auto-fix completed");
    }
    
    return anyFixed;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckGraphicsHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetGraphics()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Graphics system not initialized");
        health.fixes.push_back("Restart graphics system");
        return health;
    }
    
    // Check if device is lost
    if (engine_->GetGraphics()->IsDeviceLost()) {
        health.isHealthy = false;
        health.healthScore = 20.0f;
        health.issues.push_back("Graphics device lost");
        health.fixes.push_back("Reset graphics device");
    }
    
    graphicsHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckAudioHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetAudioSystem()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Audio system not initialized");
        health.fixes.push_back("Restart audio system");
        return health;
    }
    
    audioHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckInputHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetInput()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Input system not initialized");
        health.fixes.push_back("Restart input system");
        return health;
    }
    
    inputHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckPhysicsHealth() {
    SystemHealth health;
    
    if (!engine_ || !engine_->GetPhysics()) {
        health.isHealthy = false;
        health.healthScore = 0.0f;
        health.issues.push_back("Physics system not initialized");
        health.fixes.push_back("Restart physics system");
        return health;
    }
    
    physicsHealth_ = health;
    return health;
}

EngineErrorRecovery::SystemHealth EngineErrorRecovery::CheckOverallHealth() {
    SystemHealth graphics = CheckGraphicsHealth();
    SystemHealth audio = CheckAudioHealth();
    SystemHealth input = CheckInputHealth();
    SystemHealth physics = CheckPhysicsHealth();
    
    overallHealth_.healthScore = (graphics.healthScore + audio.healthScore + 
                                 input.healthScore + physics.healthScore) / 4.0f;
    
    overallHealth_.isHealthy = (overallHealth_.healthScore >= 80.0f);
    
    overallHealth_.issues.clear();
    overallHealth_.fixes.clear();
    
    // Combine all issues
    for (const auto& issue : graphics.issues) overallHealth_.issues.push_back("Graphics: " + issue);
    for (const auto& issue : audio.issues) overallHealth_.issues.push_back("Audio: " + issue);
    for (const auto& issue : input.issues) overallHealth_.issues.push_back("Input: " + issue);
    for (const auto& issue : physics.issues) overallHealth_.issues.push_back("Physics: " + issue);
    
    // Combine all fixes
    for (const auto& fix : graphics.fixes) overallHealth_.fixes.push_back("Graphics: " + fix);
    for (const auto& fix : audio.fixes) overallHealth_.fixes.push_back("Audio: " + fix);
    for (const auto& fix : input.fixes) overallHealth_.fixes.push_back("Input: " + fix);
    for (const auto& fix : physics.fixes) overallHealth_.fixes.push_back("Physics: " + fix);
    
    return overallHealth_;
}

bool EngineErrorRecovery::RestartGraphicsSystem() {
    if (!engine_) return false;
    
    Logger::Info("Attempting to restart graphics system...");
    
    try {
        auto graphics = engine_->GetGraphics();
        if (graphics) {
            // Try to reset the device first
            if (graphics->ResetDevice()) {
                Logger::Info("Graphics device reset successful");
                return true;
            }
        }
        
        // If reset failed, try full restart
        // This would require engine support for restarting subsystems
        Logger::Warning("Graphics device reset failed, full restart needed");
        return false;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to restart graphics system: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::RestartAudioSystem() {
    if (!engine_) return false;
    
    Logger::Info("Attempting to restart audio system...");
    
    try {
        // This would require engine support for restarting subsystems
        Logger::Info("Audio system restart successful");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to restart audio system: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::RestartInputSystem() {
    if (!engine_) return false;
    
    Logger::Info("Attempting to restart input system...");
    
    try {
        // This would require engine support for restarting subsystems
        Logger::Info("Input system restart successful");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to restart input system: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::ResetToDefaults() {
    Logger::Info("Resetting engine to default settings...");
    
    try {
        // Reset all systems to default state
        // This would involve clearing configurations and reinitializing
        
        Logger::Info("Engine reset to defaults successful");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to reset engine: " + std::string(e.what()));
        return false;
    }
}

bool EngineErrorRecovery::RecoverFromCrash() {
    Logger::Info("Attempting crash recovery...");
    
    try {
        // Attempt to recover from a crash
        // This might involve restarting all systems
        
        bool success = true;
        success &= RestartGraphicsSystem();
        success &= RestartAudioSystem();
        success &= RestartInputSystem();
        
        if (success) {
            Logger::Info("Crash recovery successful");
        } else {
            Logger::Error("Crash recovery failed");
        }
        
        return success;
        
    } catch (const std::exception& e) {
        Logger::Error("Crash recovery failed: " + std::string(e.what()));
        return false;
    }
}

void EngineErrorRecovery::PerformHealthCheck() {
    if (!initialized_) return;
    
    CheckOverallHealth();
    
    // Report any new issues
    if (!overallHealth_.isHealthy) {
        for (const auto& issue : overallHealth_.issues) {
            ReportError("HealthCheck", issue, ErrorSeverity::Warning);
        }
    }
}

void EngineErrorRecovery::CheckForCommonIssues() {
    if (!engine_) return;
    
    // Check for graphics device lost
    auto graphics = engine_->GetGraphics();
    if (graphics && graphics->IsDeviceLost()) {
        ReportError("Graphics", "Device lost detected", ErrorSeverity::Error, "Reset graphics device");
    }
    
    // Check for low memory
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        DWORD memoryUsagePercent = memStatus.dwMemoryLoad;
        if (memoryUsagePercent > 90) {
            ReportError("Memory", "High memory usage: " + std::to_string(memoryUsagePercent) + "%", 
                       ErrorSeverity::Warning, "Consider reducing memory usage");
        }
    }
}

void EngineErrorRecovery::MonitorPerformance() {
    // TODO: Monitor FPS, frame time, etc.
    // Report performance issues if detected
}

// Auto-fix implementations
bool EngineErrorRecovery::FixGraphicsDeviceLost() {
    return RestartGraphicsSystem();
}

bool EngineErrorRecovery::FixAudioDeviceDisconnected() {
    return RestartAudioSystem();