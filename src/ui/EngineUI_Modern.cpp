#include "EngineUI.h"
#include "ModernTheme.h"
#include "Engine.h"
#include "Logger.h"
#include <cstdio>
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

// Beautiful animated background for splash screen
void DrawAnimatedBackground() {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    static float time = 0.0f;
    time += 0.016f;

    ImVec2 screen_size = ImGui::GetMainViewport()->Size;

    // Gradient background
    draw_list->AddRectFilledMultiColor(
        ImVec2(0, 0), screen_size,
        IM_COL32(15, 20, 30, 255),
        IM_COL32(20, 25, 40, 255),
        IM_COL32(10, 15, 25, 255),
        IM_COL32(15, 20, 30, 255)
    );

    // Animated particles/stars
    for (int i = 0; i < 100; i++) {
        float x = (i * 123.456f) * screen_size.x / 100.0f;
        float y = fmodf((i * 789.012f + time * 20.0f), screen_size.y);
        float alpha = sinf(time + i * 0.1f) * 0.5f + 0.5f;
        draw_list->AddCircleFilled(
            ImVec2(x, y), 2.0f,
            IM_COL32(100, 150, 255, (int)(alpha * 100))
        );
    }
}

// Modern card-style panel
void BeginCard(const char* title, const ImVec4& accent_color = ImVec4(0.28f, 0.56f, 1.0f, 1.0f)) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.18f, 0.22f, 1.0f));
    ImGui::BeginChild(title, ImVec2(0, 0), true);

    // Card header with accent color
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 4);
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(accent_color));

    ImGui::Dummy(ImVec2(0, 8));
    ImGui::PushFont(ImGui::GetFont()); // Use bold font if available
    ImGui::Text("%s", title);
    ImGui::PopFont();
    ImGui::Separator();
}

void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

// Smooth progress bar with glow effect
void ModernProgressBar(float progress, const char* label, const ImVec4& color = ImVec4(0.28f, 0.56f, 1.0f, 1.0f)) {
    ImGui::Text("%s", label);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 20);

    // Background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(30, 35, 40, 255), 6.0f);

    // Progress fill with gradient
    if (progress > 0.0f) {
        float fill_width = size.x * progress;
        ImVec4 gradient_end = ImVec4(color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, color.w);
        draw_list->AddRectFilledMultiColor(
            pos, ImVec2(pos.x + fill_width, pos.y + size.y),
            ImGui::GetColorU32(color),
            ImGui::GetColorU32(gradient_end),
            ImGui::GetColorU32(gradient_end),
            ImGui::GetColorU32(color)
        );

        // Glow effect
        draw_list->AddRectFilled(
            ImVec2(pos.x, pos.y),
            ImVec2(pos.x + fill_width, pos.y + size.y),
            IM_COL32(
                (int)(color.x * 255),
                (int)(color.y * 255),
                (int)(color.z * 255),
                60
            ), 8.0f
        );
    }

    // Progress text
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.0f%%", progress * 100.0f);
    ImVec2 text_size = ImGui::CalcTextSize(buf);
    draw_list->AddText(
        ImVec2(pos.x + (size.x - text_size.x) * 0.5f, pos.y + (size.y - text_size.y) * 0.5f),
        IM_COL32(255, 255, 255, 255), buf
    );

    ImGui::Dummy(size);
}

// Glowing status badge
void StatusBadge(const char* text, const ImVec4& color, const char* icon = nullptr) {
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, color.w));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));

    if (icon) {
        char label[256];
        std::snprintf(label, sizeof(label), "%s %s", icon, text);
        ImGui::Button(label);
    } else {
        ImGui::Button(text);
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

// Modern stat display
void StatDisplay(const char* label, const char* value, const char* icon = "📊") {
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s %s", icon, label);
    ImGui::PushFont(ImGui::GetFont()); // Use larger font if available
    ImGui::TextColored(ImVec4(0.28f, 0.56f, 1.0f, 1.0f), "%s", value);
    ImGui::PopFont();
    ImGui::EndGroup();
}

EngineUI::EngineUI()
    : engine_(nullptr)
    , device_(nullptr)
    , context_(nullptr)
    , initialized_(false)
    , isVisible_(true)
    , imguiContext_(nullptr)
    , fontsLoaded_(false)
{
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

    Logger::Info("Initializing Modern Engine UI...");

    try {
        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        imguiContext_ = ImGui::GetCurrentContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        #ifdef ImGuiConfigFlags_DockingEnable
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        #endif
        #ifdef ImGuiConfigFlags_ViewportsEnable
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        #endif

        // Apply modern theme
        ModernTheme::ApplyTheme(ModernTheme::ColorScheme::Dark);

        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(GetActiveWindow())) {
            Logger::Error("Failed to initialize ImGui Win32");
            return false;
        }

        if (!ImGui_ImplDX11_Init(device, context)) {
            Logger::Error("Failed to initialize ImGui DX11");
            return false;
        }

        initialized_ = true;
        Logger::Info("✨ Modern Engine UI initialized successfully!");
        return true;
    }
    catch (const std::exception& e) {
        Logger::Error("Exception initializing UI: " + std::string(e.what()));
        return false;
    }
}

void EngineUI::Shutdown() {
    if (!initialized_) return;

    Logger::Info("Shutting down UI...");

    if (imguiContext_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(static_cast<ImGuiContext*>(imguiContext_));
        imguiContext_ = nullptr;
    }

    initialized_ = false;
}

void EngineUI::NewFrame() {
    if (!initialized_) return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void EngineUI::Render() {
    if (!initialized_ || !isVisible_) return;

    DrawAnimatedBackground();

    // Render modern splash/welcome screen on first launch
    static bool showWelcome = true;
    if (showWelcome) {
        RenderModernWelcomeScreen();
        if (!showWelcomeDialog_) showWelcome = false;
    }

    RenderMainMenuBar();
    RenderModernDashboard();
}

void EngineUI::RenderModernDashboard() {
    ImGui::SetNextWindowPos(ImVec2(0, 25));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 25));

    ImGui::Begin("##Dashboard", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Main dashboard grid
    float width = ImGui::GetContentRegionAvail().x;
    float col_width = (width - 20) / 3.0f;

    // Left column - Stats & Quick Actions
    ImGui::BeginChild("LeftCol", ImVec2(col_width, 0), false);
    {
        BeginCard("🎮 Quick Actions", ImVec4(0.28f, 0.56f, 1.0f, 1.0f));
        {
            if (ImGui::Button("▶️ Start Engine", ImVec2(-1, 40))) {
                if (onEngineCommand_) onEngineCommand_("start");
            }
            if (ImGui::Button("⏸️ Pause", ImVec2(-1, 40))) {
                if (onEngineCommand_) onEngineCommand_("pause");
            }
            if (ImGui::Button("🔄 Restart", ImVec2(-1, 40))) {
                if (onEngineCommand_) onEngineCommand_("restart");
            }
        }
        EndCard();

        ImGui::Spacing();

        BeginCard("⚡ Performance", ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        {
            char fps_text[32];
            std::snprintf(fps_text, sizeof(fps_text), "%.1f", metrics_.fps);
            StatDisplay("FPS", fps_text, "🎯");

            ImGui::Spacing();

            char frame_time[32];
            std::snprintf(frame_time, sizeof(frame_time), "%.2f ms", metrics_.frameTime);
            StatDisplay("Frame Time", frame_time, "⏱️");

            ImGui::Spacing();
            ModernProgressBar(metrics_.fps / 144.0f, "Performance", ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        }
        EndCard();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Middle column - Engine Status
    ImGui::BeginChild("MiddleCol", ImVec2(col_width, 0), false);
    {
        BeginCard("🔍 Engine Status", ImVec4(0.8f, 0.4f, 0.8f, 1.0f));
        {
            ImGui::Spacing();

            if (status_.hasErrors) {
                StatusBadge("ERRORS DETECTED", ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "❌");
            } else if (status_.hasWarnings) {
                StatusBadge("WARNINGS", ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "⚠️");
            } else {
                StatusBadge("ALL SYSTEMS GO", ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "✅");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            StatDisplay("Graphics API", status_.graphicsAPI.c_str(), "🎨");
            ImGui::Spacing();
            StatDisplay("Physics Engine", "Jolt Physics", "⚛️");
            ImGui::Spacing();
            StatDisplay("Audio System", status_.audioSystem.c_str(), "🔊");
        }
        EndCard();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right column - Console
    ImGui::BeginChild("RightCol", ImVec2(col_width, 0), false);
    {
        BeginCard("📝 Console Log", ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        {
            ImGui::BeginChild("ConsoleScroll", ImVec2(0, -40), false);
            for (const auto& line : consoleLog_) {
                if (line.find("ERROR") != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%s", line.c_str());
                } else if (line.find("WARNING") != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "%s", line.c_str());
                } else if (line.find("INFO") != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", line.c_str());
                } else {
                    ImGui::Text("%s", line.c_str());
                }
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();

            if (ImGui::Button("Clear Console", ImVec2(-1, 30))) {
                consoleLog_.clear();
            }
        }
        EndCard();
    }
    ImGui::EndChild();

    ImGui::End();
}

void EngineUI::RenderModernWelcomeScreen() {
    if (!showWelcomeDialog_) return;

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400));

    ImGui::Begin("##Welcome", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    // Animated header
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::PushFont(ImGui::GetFont()); // Use large font if available
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("NEXUS ENGINE").x) * 0.5f);
    ImGui::TextColored(ImVec4(0.28f, 0.56f, 1.0f, 1.0f), "NEXUS ENGINE");
    ImGui::PopFont();

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Modern Game Development Platform").x) * 0.5f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Modern Game Development Platform");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("Welcome to Nexus Engine! A powerful, modern game engine with:");
    ImGui::Spacing();
    ImGui::BulletText("Vulkan, DirectX 11, and OpenGL support");
    ImGui::BulletText("Jolt Physics integration");
    ImGui::BulletText("Python & Lua scripting");
    ImGui::BulletText("Advanced rendering features");
    ImGui::BulletText("Real-time debugging & profiling");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
    if (ImGui::Button("🚀 Get Started", ImVec2(200, 50))) {
        showWelcomeDialog_ = false;
    }

    ImGui::End();
}

void EngineUI::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        ImGui::TextColored(ImVec4(0.28f, 0.56f, 1.0f, 1.0f), "⚡ NEXUS");

        if (ImGui::BeginMenu("Engine")) {
            if (ImGui::MenuItem("🏠 Dashboard", "F1")) { }
            if (ImGui::MenuItem("⚙️ Settings")) { }
            ImGui::Separator();
            if (ImGui::MenuItem("🚪 Exit", "Alt+F4")) {
                if (onEngineCommand_) onEngineCommand_("exit");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("📊 Performance");
            ImGui::MenuItem("📝 Console");
            ImGui::MenuItem("🐛 Debug");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("📋 Export Diagnostics")) { }
            if (ImGui::MenuItem("🎨 Change Theme")) { }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("ℹ️ About")) {
                showAboutDialog_ = true;
            }
            ImGui::EndMenu();
        }

        // Status in menu bar
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 250);

        char fps_text[32];
        std::snprintf(fps_text, sizeof(fps_text), "🎯 %.0f FPS", metrics_.fps);
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", fps_text);

        ImGui::SameLine();

        if (status_.hasErrors) {
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "❌ ERROR");
        } else {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "✅ OK");
        }

        ImGui::EndMainMenuBar();
    }
}

void EngineUI::EndFrame() {
    if (!initialized_ || !isVisible_) return;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    #ifdef ImGuiConfigFlags_ViewportsEnable
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    #endif
}

void EngineUI::UpdateMetrics(float fps, float frameTime) {
    metrics_.fps = fps;
    metrics_.frameTime = frameTime;
}

void EngineUI::AddLog(const std::string& message) {
    consoleLog_.push_back(message);
    if (consoleLog_.size() > settings_.maxLogLines) {
        consoleLog_.erase(consoleLog_.begin());
    }
}

} // namespace Nexus
