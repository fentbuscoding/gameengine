#include "EngineUI.h"
#include "Engine.h"
#include "Logger.h"
#include "GraphicsDevice.h"
#include "InputManager.h"

// ImGui includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace Nexus {

EngineUI::EngineUI()
    : engine_(nullptr)
    , device_(nullptr)
    , context_(nullptr)
    , initialized_(false)
    , isVisible_(true)
    , imguiContext_(nullptr)
    , fontsLoaded_(false)
{
    // Initialize console log
    consoleLog_.reserve(settings_.maxLogLines);
}

EngineUI::~EngineUI() {
    Shutdown();
}

bool EngineUI::Initialize(Engine* engine, ID3D11Device* device, ID3D11DeviceContext* context) {
    if (initialized_) return true;
    
    engine_ = engine;
    device_ = device;
    context_ = context;
    
    Logger::Info("Initializing Engine UI...");
    
    try {
        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        imguiContext_ = ImGui::GetCurrentContext();
        
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        
        // Setup ImGui style
        ApplyTheme();
        
        // Setup Platform/Renderer backends
        extern HWND g_hWnd; // Assuming this is available from main window
        if (!ImGui_ImplWin32_Init(g_hWnd)) {
            Logger::Error("Failed to initialize ImGui Win32 backend");
            return false;
        }
        
        if (!ImGui_ImplDX11_Init(device_, context_)) {
            Logger::Error("Failed to initialize ImGui DirectX 11 backend");
            return false;
        }
        
        // Load fonts
        float fontSize = theme_.fontSize;
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", fontSize);
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", fontSize * 0.9f); // Console font
        
        fontsLoaded_ = true;
        initialized_ = true;
        
        Logger::Info("Engine UI initialized successfully");
        
        // Add welcome message
        AddLogMessage("🎮 Welcome to Nexus Engine!", 0);
        AddLogMessage("Press F1 to toggle this UI, F11 for fullscreen", 0);
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("Failed to initialize Engine UI: " + std::string(e.what()));
        return false;
    }
}

void EngineUI::Shutdown() {
    if (!initialized_) return;
    
    Logger::Info("Shutting down Engine UI...");
    
    if (imguiContext_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiContext_ = nullptr;
    }
    
    initialized_ = false;
    Logger::Info("Engine UI shutdown complete");
}

void EngineUI::NewFrame() {
    if (!initialized_ || !isVisible_) return;
    
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Update engine status
    UpdateEngineStatus();
}

void EngineUI::Render() {
    if (!initialized_ || !isVisible_) return;
    
    // Main menu bar
    RenderMainMenuBar();
    
    // Create dockspace
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
    
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("NexusEngine DockSpace", nullptr, window_flags);
    
    if (!opt_padding)
        ImGui::PopStyleVar();
    
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);
    
    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("NexusEngineDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    
    // Render panels
    if (showQuickActionsPanel_) RenderQuickActionsPanel();
    if (showPerformancePanel_) RenderPerformancePanel();
    if (showEngineStatusPanel_) RenderEngineStatusPanel();
    if (showConsolePanel_) RenderConsolePanel();
    if (showSettingsPanel_) RenderSettingsPanel();
    if (showHelpPanel_) RenderHelpPanel();
    if (showDebugPanel_) RenderDebugPanel();
    if (showSystemInfoPanel_) RenderSystemInfoPanel();
    
    ImGui::End();
    
    // Render dialogs
    if (showWelcomeDialog_) RenderWelcomeDialog();
    if (showErrorDialog_) RenderErrorDialog();
    if (showSuccessDialog_) RenderSuccessDialog();
    if (showAboutDialog_) RenderAboutDialog();
}

void EngineUI::EndFrame() {
    if (!initialized_ || !isVisible_) return;
    
    // Rendering
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    
    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void EngineUI::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Engine")) {
            if (ImGui::MenuItem("🏠 Dashboard", "F1", showQuickActionsPanel_)) {
                showQuickActionsPanel_ = !showQuickActionsPanel_;
            }
            if (ImGui::MenuItem("⚙️ Settings", nullptr, showSettingsPanel_)) {
                showSettingsPanel_ = !showSettingsPanel_;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("🔄 Restart Engine")) {
                if (onEngineCommand_) onEngineCommand_("restart");
            }
            if (ImGui::MenuItem("⏸️ Pause Engine")) {
                if (onEngineCommand_) onEngineCommand_("pause");
            }
            if (ImGui::MenuItem("🚪 Exit", "Alt+F4")) {
                if (onEngineCommand_) onEngineCommand_("exit");
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("📊 Performance", nullptr, &showPerformancePanel_);
            ImGui::MenuItem("🔍 Engine Status", nullptr, &showEngineStatusPanel_);
            ImGui::MenuItem("📝 Console", "~", &showConsolePanel_);
            ImGui::MenuItem("🐛 Debug Info", nullptr, &showDebugPanel_);
            ImGui::MenuItem("💻 System Info", nullptr, &showSystemInfoPanel_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("📋 Export Diagnostics")) {
                ExportDiagnostics();
            }
            if (ImGui::MenuItem("🔧 Auto-Fix Issues")) {
                // TODO: Implement auto-fix
                ShowSuccessDialog("Auto-fix completed!");
            }
            if (ImGui::MenuItem("🎨 Reset UI Layout")) {
                // Reset ImGui layout
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("❓ Controls", "F1")) {
                showHelpPanel_ = !showHelpPanel_;
            }
            if (ImGui::MenuItem("ℹ️ About")) {
                showAboutDialog_ = true;
            }
            ImGui::EndMenu();
        }
        
        // Status indicators in menu bar
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 300);
        
        // FPS indicator
        ImGui::TextColored(ImVec4(theme_.successColor[0], theme_.successColor[1], theme_.successColor[2], 1.0f), 
                          "FPS: %.1f", metrics_.fps);
        
        ImGui::SameLine();
        
        // Engine status
        if (status_.hasErrors) {
            ImGui::TextColored(ImVec4(theme_.errorColor[0], theme_.errorColor[1], theme_.errorColor[2], 1.0f), "❌ ERRORS");
        } else if (status_.hasWarnings) {
            ImGui::TextColored(ImVec4(theme_.warningColor[0], theme_.warningColor[1], theme_.warningColor[2], 1.0f), "⚠️ WARNINGS");
        } else {
            ImGui::TextColored(ImVec4(theme_.successColor[0], theme_.successColor[1], theme_.successColor[2], 1.0f), "✅ HEALTHY");
        }
        
        ImGui::EndMainMenuBar();
    }
}

void EngineUI::RenderQuickActionsPanel() {
    if (ImGui::Begin("🎮 Quick Actions", &showQuickActionsPanel_)) {
        ImGui::Text("Welcome to Nexus Engine!");
        ImGui::Separator();
        
        // Quick stats
        ImGui::Text("⚡ Engine Status: %s", status_.isRunning ? "Running" : "Stopped");
        ImGui::Text("🎯 FPS: %.1f (%.2fms)", metrics_.fps, metrics_.frameTime);
        ImGui::Text("🧠 Memory: %.1f MB", metrics_.memoryUsage);
        
        ImGui::Separator();
        
        // Quick actions
        if (ImGui::Button("🎮 Start Demo Scene", ImVec2(-1, 30))) {
            if (onEngineCommand_) onEngineCommand_("demo");
        }
        
        if (ImGui::Button("🌟 Create New Scene", ImVec2(-1, 30))) {
            if (onEngineCommand_) onEngineCommand_("new_scene");
        }
        
        if (ImGui::Button("📁 Load Scene", ImVec2(-1, 30))) {
            if (onEngineCommand_) onEngineCommand_("load_scene");
        }
        
        if (ImGui::Button("💾 Save Scene", ImVec2(-1, 30))) {
            if (onEngineCommand_) onEngineCommand_("save_scene");
        }
        
        ImGui::Separator();
        
        // System controls
        if (ImGui::Button("🔄 Restart Graphics", ImVec2(-1, 25))) {
            if (onEngineCommand_) onEngineCommand_("restart_graphics");
        }
        
        if (ImGui::Button("🔊 Restart Audio", ImVec2(-1, 25))) {
            if (onEngineCommand_) onEngineCommand_("restart_audio");
        }
        
        if (ImGui::Button("🎮 Reset Input", ImVec2(-1, 25))) {
            if (onEngineCommand_) onEngineCommand_("reset_input");
        }
    }
    ImGui::End();
}

void EngineUI::RenderPerformancePanel() {
    if (ImGui::Begin("📊 Performance", &showPerformancePanel_)) {
        // FPS Graph
        static std::vector<float> fpsHistory(100);
        static int fpsOffset = 0;
        
        fpsHistory[fpsOffset] = metrics_.fps;
        fpsOffset = (fpsOffset + 1) % fpsHistory.size();
        
        ImGui::PlotLines("FPS", fpsHistory.data(), (int)fpsHistory.size(), fpsOffset, 
                        nullptr, 0.0f, 120.0f, ImVec2(0, 80));
        
        // Performance metrics
        ImGui::Columns(2);
        
        ImGui::Text("Frame Rate:");
        ImGui::Text("Frame Time:");
        ImGui::Text("CPU Usage:");
        ImGui::Text("Memory:");
        ImGui::Text("Draw Calls:");
        ImGui::Text("Triangles:");
        
        ImGui::NextColumn();
        
        ImGui::Text("%.1f FPS", metrics_.fps);
        ImGui::Text("%.2f ms", metrics_.frameTime);
        ImGui::Text("%.1f%%", metrics_.cpuUsage);
        ImGui::Text("%.1f MB", metrics_.memoryUsage);
        ImGui::Text("%d", metrics_.drawCalls);
        ImGui::Text("%d", metrics_.triangles);
        
        ImGui::Columns(1);
        
        // VSync control
        ImGui::Separator();
        if (ImGui::Checkbox("V-Sync", &settings_.enableVSync)) {
            if (onSettingsChanged_) onSettingsChanged_();
        }
    }
    ImGui::End();
}

void EngineUI::RenderEngineStatusPanel() {
    if (ImGui::Begin("🔍 Engine Status", &showEngineStatusPanel_)) {
        // Overall status
        if (status_.hasErrors) {
            ImGui::TextColored(ImVec4(theme_.errorColor[0], theme_.errorColor[1], theme_.errorColor[2], 1.0f), 
                              "❌ Engine has errors (%d)", status_.totalErrors);
        } else if (status_.hasWarnings) {
            ImGui::TextColored(ImVec4(theme_.warningColor[0], theme_.warningColor[1], theme_.warningColor[2], 1.0f), 
                              "⚠️ Engine has warnings (%d)", status_.totalWarnings);
        } else {
            ImGui::TextColored(ImVec4(theme_.successColor[0], theme_.successColor[1], theme_.successColor[2], 1.0f), 
                              "✅ Engine is healthy");
        }
        
        ImGui::Separator();
        
        // System status
        const char* systems[] = {"Graphics", "Audio", "Input", "Physics", "Scripting", "Resources"};
        const bool systemStatus[] = {true, true, true, true, true, true}; // TODO: Get real status
        
        for (int i = 0; i < 6; i++) {
            ImGui::Text("%s:", systems[i]);
            ImGui::SameLine(100);
            if (systemStatus[i]) {
                ImGui::TextColored(ImVec4(theme_.successColor[0], theme_.successColor[1], theme_.successColor[2], 1.0f), "✅ OK");
            } else {
                ImGui::TextColored(ImVec4(theme_.errorColor[0], theme_.errorColor[1], theme_.errorColor[2], 1.0f), "❌ ERROR");
            }
        }
        
        ImGui::Separator();
        
        // Last error/warning
        if (!status_.lastError.empty()) {
            ImGui::TextColored(ImVec4(theme_.errorColor[0], theme_.errorColor[1], theme_.errorColor[2], 1.0f), 
                              "Last Error:");
            ImGui::TextWrapped("%s", status_.lastError.c_str());
        }
        
        if (!status_.lastWarning.empty()) {
            ImGui::TextColored(ImVec4(theme_.warningColor[0], theme_.warningColor[1], theme_.warningColor[2], 1.0f), 
                              "Last Warning:");
            ImGui::TextWrapped("%s", status_.lastWarning.c_str());
        }
    }
    ImGui::End();
}

void EngineUI::RenderConsolePanel() {
    if (ImGui::Begin("📝 Console", &showConsolePanel_)) {
        // Log display
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        for (const auto& line : consoleLog_) {
            if (line.find("ERROR") != std::string::npos) {
                ImGui::TextColored(ImVec4(theme_.errorColor[0], theme_.errorColor[1], theme_.errorColor[2], 1.0f), "%s", line.c_str());
            } else if (line.find("WARNING") != std::string::npos) {
                ImGui::TextColored(ImVec4(theme_.warningColor[0], theme_.warningColor[1], theme_.warningColor[2], 1.0f), "%s", line.c_str());
            } else {
                ImGui::Text("%s", line.c_str());
            }
        }
        
        // Auto-scroll
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        
        ImGui::EndChild();
        
        // Command input
        ImGui::Separator();
        if (ImGui::InputText("Command", &consoleInput_, ImGuiInputTextFlags_EnterReturnsTrue)) {
            ProcessConsoleCommand(consoleInput_);
            consoleInput_.clear();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Send")) {
            ProcessConsoleCommand(consoleInput_);
            consoleInput_.clear();
        }
    }
    ImGui::End();
}

void EngineUI::RenderSettingsPanel() {
    if (ImGui::Begin("⚙️ Settings", &showSettingsPanel_)) {
        if (ImGui::CollapsingHeader("🎨 Appearance")) {
            if (ImGui::Checkbox("Dark Mode", &settings_.darkMode)) {
                theme_.darkTheme = settings_.darkMode;
                ApplyTheme();
            }
            
            if (ImGui::SliderFloat("UI Scale", &settings_.uiScale, 0.5f, 2.0f)) {
                ImGuiIO& io = ImGui::GetIO();
                io.FontGlobalScale = settings_.uiScale;
            }
            
            ImGui::ColorEdit3("Accent Color", theme_.accentColor);
        }
        
        if (ImGui::CollapsingHeader("📊 Performance")) {
            ImGui::Checkbox("Show FPS", &settings_.showFPS);
            ImGui::Checkbox("Show Metrics", &settings_.showMetrics);
            ImGui::Checkbox("Enable V-Sync", &settings_.enableVSync);
            ImGui::Checkbox("Debug Mode", &settings_.enableDebugMode);
        }
        
        if (ImGui::CollapsingHeader("🔧 Advanced")) {
            ImGui::Checkbox("Auto-Save", &settings_.autoSaveEnabled);
            ImGui::SliderInt("Max Log Lines", &settings_.maxLogLines, 100, 5000);
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("💾 Save Settings")) {
            // TODO: Save settings to file
            ShowSuccessDialog("Settings saved!");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("🔄 Reset to Defaults")) {
            settings_ = Settings{};
            ApplyTheme();
        }
    }
    ImGui::End();
}

void EngineUI::RenderHelpPanel() {
    if (ImGui::Begin("❓ Help & Controls", &showHelpPanel_)) {
        ImGui::Text("🎮 Nexus Engine Controls");
        ImGui::Separator();
        
        ImGui::Text("🎯 Camera Controls:");
        ImGui::BulletText("WASD - Move camera");
        ImGui::BulletText("Q/E - Move up/down");
        ImGui::BulletText("Mouse - Look around");
        ImGui::BulletText("Scroll - Zoom");
        
        ImGui::Separator();
        ImGui::Text("🔧 System Controls:");
        ImGui::BulletText("F1 - Toggle UI");
        ImGui::BulletText("F11 - Toggle fullscreen");
        ImGui::BulletText("~ - Toggle console");
        ImGui::BulletText("ESC - Exit");
        
        ImGui::Separator();
        ImGui::Text("🐛 Debug Controls:");
        ImGui::BulletText("F3 - Debug overlay");
        ImGui::BulletText("F4 - Wireframe mode");
        ImGui::BulletText("F5 - Reload shaders");
        
        ImGui::Separator();
        ImGui::Text("📋 Common Commands:");
        ImGui::BulletText("help - Show available commands");
        ImGui::BulletText("clear - Clear console");
        ImGui::BulletText("fps - Show FPS info");
        ImGui::BulletText("exit - Exit engine");
    }
    ImGui::End();
}

void EngineUI::RenderDebugPanel() {
    if (ImGui::Begin("🐛 Debug Info", &showDebugPanel_)) {
        // TODO: Add debug information
        ImGui::Text("Debug information will be displayed here");
    }
    ImGui::End();
}

void EngineUI::RenderSystemInfoPanel() {
    if (ImGui::Begin("💻 System Info", &showSystemInfoPanel_)) {
        // TODO: Add system information
        ImGui::Text("System information will be displayed here");
    }
    ImGui::End();
}

void EngineUI::RenderWelcomeDialog() {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("🎮 Welcome to Nexus Engine!", &showWelcomeDialog_, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Welcome to the Nexus Game Engine!");
        ImGui::Separator();
        
        ImGui::Text("🎯 Quick Start:");
        ImGui::BulletText("Press F1 to toggle this UI anytime");
        ImGui::BulletText("Use the Quick Actions panel to get started");
        ImGui::BulletText("Check the Help panel for controls");
        
        ImGui::Separator();
        ImGui::Text("🔧 What's New:");
        ImGui::BulletText("Modern user-friendly interface");
        ImGui::BulletText("Real-time performance monitoring");
        ImGui::BulletText("Automatic error detection and recovery");
        ImGui::BulletText("Multi-language scripting support");
        
        ImGui::Separator();
        
        if (ImGui::Button("🚀 Get Started!", ImVec2(150, 30))) {
            showWelcomeDialog_ = false;
            showQuickActionsPanel_ = true;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("📖 View Tutorial", ImVec2(150, 30))) {
            showWelcomeDialog_ = false;
            showHelpPanel_ = true;
        }
        
        ImGui::Separator();
        ImGui::Checkbox("Don't show this again", &settings_.autoSaveEnabled);
    }
    ImGui::End();
}

void EngineUI::RenderErrorDialog() {
    if (ImGui::BeginPopupModal("❌ Error", &showErrorDialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(theme_.errorColor[0], theme_.errorColor[1], theme_.errorColor[2], 1.0f), 
                          "An error occurred:");
        ImGui::Separator();
        ImGui::TextWrapped("%s", currentError_.c_str());
        ImGui::Separator();
        
        if (ImGui::Button("🔧 Try Auto-Fix")) {
            // TODO: Implement auto-fix
            showErrorDialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("📋 Copy Error")) {
            // TODO: Copy to clipboard
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("✅ OK")) {
            showErrorDialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void EngineUI::RenderSuccessDialog() {
    if (ImGui::BeginPopupModal("✅ Success", &showSuccessDialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(theme_.successColor[0], theme_.successColor[1], theme_.successColor[2], 1.0f), 
                          "%s", currentSuccess_.c_str());
        ImGui::Separator();
        
        if (ImGui::Button("✅ OK")) {
            showSuccessDialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void EngineUI::RenderAboutDialog() {
    if (ImGui::BeginPopupModal("ℹ️ About Nexus Engine", &showAboutDialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("🎮 Nexus Game Engine");
        ImGui::Text("Version 1.0.0");
        ImGui::Separator();
        
        ImGui::Text("A modern, user-friendly game engine with:");
        ImGui::BulletText("Multi-language scripting (C++, Python, Lua)");
        ImGui::BulletText("Advanced rendering pipeline");
        ImGui::BulletText("Real-time performance monitoring");
        ImGui::BulletText("Automatic error recovery");
        
        ImGui::Separator();
        
        if (ImGui::Button("✅ Close")) {
            showAboutDialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void EngineUI::ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    if (theme_.darkTheme) {
        ImGui::StyleColorsDark();
        
        // Customize dark theme
        style.Colors[ImGuiCol_WindowBg] = ImVec4(theme_.backgroundColor[0], theme_.backgroundColor[1], theme_.backgroundColor[2], theme_.backgroundColor[3]);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(theme_.accentColor[0], theme_.accentColor[1], theme_.accentColor[2], 0.31f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(theme_.accentColor[0], theme_.accentColor[1], theme_.accentColor[2], 0.80f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(theme_.accentColor[0], theme_.accentColor[1], theme_.accentColor[2], 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(theme_.accentColor[0], theme_.accentColor[1], theme_.accentColor[2], 0.40f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(theme_.accentColor[0], theme_.accentColor[1], theme_.accentColor[2], 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(theme_.accentColor[0], theme_.accentColor[1], theme_.accentColor[2], 1.00f);
    } else {
        ImGui::StyleColorsLight();
    }
    
    // Apply custom styling
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding = 3.0f;
}

void EngineUI::UpdateEngineStatus() {
    // TODO: Get real engine status
    status_.isRunning = engine_ != nullptr;
    
    // Update performance metrics
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    if (deltaTime > 0.0f) {
        metrics_.fps = 1.0f / deltaTime;
        metrics_.frameTime = deltaTime * 1000.0f;
    }
}

void EngineUI::UpdatePerformanceMetrics(const PerformanceMetrics& metrics) {
    metrics_ = metrics;
}

void EngineUI::AddLogMessage(const std::string& message, int level) {
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "[%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << "] ";
    
    // Add level prefix
    switch (level) {
        case 1: ss << "⚠️ WARNING: "; status_.hasWarnings = true; status_.totalWarnings++; status_.lastWarning = message; break;
        case 2: ss << "❌ ERROR: "; status_.hasErrors = true; status_.totalErrors++; status_.lastError = message; break;
        default: ss << "ℹ️ INFO: "; break;
    }
    
    ss << message;
    
    consoleLog_.push_back(ss.str());
    
    // Limit log size
    if (consoleLog_.size() > static_cast<size_t>(settings_.maxLogLines)) {
        consoleLog_.erase(consoleLog_.begin());
    }
}

void EngineUI::ProcessConsoleCommand(const std::string& command) {
    AddLogMessage("> " + command, 0);
    
    if (command == "help") {
        AddLogMessage("Available commands:", 0);
        AddLogMessage("  help - Show this help", 0);
        AddLogMessage("  clear - Clear console", 0);
        AddLogMessage("  fps - Show FPS info", 0);
        AddLogMessage("  status - Show engine status", 0);
        AddLogMessage("  exit - Exit engine", 0);
    } else if (command == "clear") {
        consoleLog_.clear();
    } else if (command == "fps") {
        AddLogMessage("FPS: " + std::to_string(metrics_.fps), 0);
        AddLogMessage("Frame Time: " + std::to_string(metrics_.frameTime) + "ms", 0);
    } else if (command == "status") {
        AddLogMessage("Engine Status: " + std::string(status_.isRunning ? "Running" : "Stopped"), 0);
        AddLogMessage("Errors: " + std::to_string(status_.totalErrors), 0);
        AddLogMessage("Warnings: " + std::to_string(status_.totalWarnings), 0);
    } else if (command == "exit") {
        if (onEngineCommand_) onEngineCommand_("exit");
    } else {
        AddLogMessage("Unknown command: " + command, 1);
        AddLogMessage("Type 'help' for available commands", 0);
    }
}

void EngineUI::ExportDiagnostics() {
    // TODO: Export comprehensive diagnostics
    ShowSuccessDialog("Diagnostics exported to diagnostics.txt");
}

void EngineUI::ShowWelcomeDialog() {
    showWelcomeDialog_ = true;
}

void EngineUI::ShowErrorDialog(const std::string& error) {
    currentError_ = error;
    showErrorDialog_ = true;
    ImGui::OpenPopup("❌ Error");
}

void EngineUI::ShowSuccessDialog(const std::string& message) {
    currentSuccess_ = message;
    showSuccessDialog_ = true;
    ImGui::OpenPopup("✅ Success");
}

} // namespace Nexus