#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

// Forward declarations
namespace ImGui { }
struct ImDrawData;

namespace Nexus {

class Engine;
class Logger;

/**
 * Modern, user-friendly UI system for the Nexus Engine
 * Features: Real-time monitoring, easy controls, bug reporting, performance metrics
 */
class EngineUI {
public:
    struct UITheme {
        float accentColor[4] = { 0.2f, 0.6f, 1.0f, 1.0f };  // Blue
        float backgroundColor[4] = { 0.1f, 0.1f, 0.1f, 0.95f };
        float textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float successColor[4] = { 0.2f, 0.8f, 0.2f, 1.0f };   // Green
        float warningColor[4] = { 1.0f, 0.8f, 0.2f, 1.0f };   // Yellow
        float errorColor[4] = { 1.0f, 0.2f, 0.2f, 1.0f };     // Red
        float fontSize = 16.0f;
        bool darkTheme = true;
    };

    struct PerformanceMetrics {
        float fps = 0.0f;
        float frameTime = 0.0f;
        float cpuUsage = 0.0f;
        float memoryUsage = 0.0f;
        int drawCalls = 0;
        int triangles = 0;
        bool vsyncEnabled = true;
    };

    struct EngineStatus {
        bool isRunning = false;
        bool hasErrors = false;
        bool hasWarnings = false;
        std::string lastError;
        std::string lastWarning;
        int totalErrors = 0;
        int totalWarnings = 0;
    };

public:
    EngineUI();
    ~EngineUI();

    // Initialization
    bool Initialize(Engine* engine, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Main UI rendering
    void NewFrame();
    void Render();
    void EndFrame();

    // UI state management
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }
    void ToggleVisibility() { isVisible_ = !isVisible_; }

    // Theme management
    void SetTheme(const UITheme& theme);
    const UITheme& GetTheme() const { return theme_; }
    void LoadThemeFromFile(const std::string& filename);
    void SaveThemeToFile(const std::string& filename) const;

    // Performance monitoring
    void UpdatePerformanceMetrics(const PerformanceMetrics& metrics);
    void AddLogMessage(const std::string& message, int level = 0); // 0=Info, 1=Warning, 2=Error

    // User interaction callbacks
    void SetOnSettingsChanged(std::function<void()> callback) { onSettingsChanged_ = callback; }
    void SetOnEngineCommand(std::function<void(const std::string&)> callback) { onEngineCommand_ = callback; }

    // Quick access functions
    void ShowWelcomeDialog();
    void ShowErrorDialog(const std::string& error);
    void ShowSuccessDialog(const std::string& message);

private:
    // UI Panels
    void RenderMainMenuBar();
    void RenderPerformancePanel();
    void RenderEngineStatusPanel();
    void RenderConsolePanel();
    void RenderSettingsPanel();
    void RenderHelpPanel();
    void RenderQuickActionsPanel();
    void RenderDebugPanel();
    void RenderSystemInfoPanel();

    // Dialog windows
    void RenderWelcomeDialog();
    void RenderErrorDialog();
    void RenderSuccessDialog();
    void RenderAboutDialog();

    // Helper functions
    void ApplyTheme();
    void UpdateEngineStatus();
    void ProcessConsoleCommand(const std::string& command);
    void ExportDiagnostics();
    
    Engine* engine_;
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    
    bool initialized_;
    bool isVisible_;
    UITheme theme_;
    
    // UI state
    bool showPerformancePanel_ = true;
    bool showEngineStatusPanel_ = true;
    bool showConsolePanel_ = false;
    bool showSettingsPanel_ = false;
    bool showHelpPanel_ = false;
    bool showQuickActionsPanel_ = true;
    bool showDebugPanel_ = false;
    bool showSystemInfoPanel_ = false;
    
    // Dialog states
    bool showWelcomeDialog_ = true;
    bool showErrorDialog_ = false;
    bool showSuccessDialog_ = false;
    bool showAboutDialog_ = false;
    
    // Data
    PerformanceMetrics metrics_;
    EngineStatus status_;
    std::vector<std::string> consoleLog_;
    std::string currentError_;
    std::string currentSuccess_;
    std::string consoleInput_;
    
    // Settings
    struct Settings {
        bool autoSaveEnabled = true;
        float uiScale = 1.0f;
        bool showFPS = true;
        bool showMetrics = true;
        bool enableVSync = true;
        bool enableDebugMode = false;
        int maxLogLines = 1000;
        bool darkMode = true;
    } settings_;
    
    // Callbacks
    std::function<void()> onSettingsChanged_;
    std::function<void(const std::string&)> onEngineCommand_;
    
    // ImGui specific
    void* imguiContext_;
    bool fontsLoaded_;
};

/**
 * Engine Health Monitor - monitors engine state and provides diagnostics
 */
class EngineHealthMonitor {
public:
    struct HealthReport {
        bool isHealthy = true;
        float overallScore = 100.0f;
        std::vector<std::string> issues;
        std::vector<std::string> suggestions;
        
        struct SystemHealth {
            float graphics = 100.0f;
            float audio = 100.0f;
            float input = 100.0f;
            float physics = 100.0f;
            float memory = 100.0f;
            float performance = 100.0f;
        } systems;
    };

    EngineHealthMonitor();
    ~EngineHealthMonitor();

    bool Initialize(Engine* engine);
    void Update(float deltaTime);
    HealthReport GenerateReport() const;
    
    // Issue tracking
    void ReportIssue(const std::string& system, const std::string& issue, int severity = 1);
    void ClearIssues(const std::string& system = "");
    
    // Auto-fixing
    bool AttemptAutoFix(const std::string& issue);
    void SetAutoFixEnabled(bool enabled) { autoFixEnabled_ = enabled; }

private:
    void CheckGraphicsHealth();
    void CheckAudioHealth();
    void CheckInputHealth();
    void CheckPhysicsHealth();
    void CheckMemoryHealth();
    void CheckPerformanceHealth();
    
    Engine* engine_;
    bool initialized_;
    bool autoFixEnabled_;
    float updateTimer_;
    HealthReport lastReport_;
    
    struct Issue {
        std::string system;
        std::string description;
        int severity;
        float timestamp;
    };
    std::vector<Issue> issues_;
};

} // namespace Nexus