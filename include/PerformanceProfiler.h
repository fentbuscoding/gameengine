#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cinttypes>   // PRIu64
#include <cstdint>
#include <algorithm>
#include "imgui.h"

namespace Nexus {

struct ProfilerEntry {
    std::string name;
    double timeMs;
    double percentage;
    int callCount;
    double avgTimeMs;
    double minTimeMs;
    double maxTimeMs;
    ImVec4 color;
};

class PerformanceProfiler {
public:
    static PerformanceProfiler& Get() {
        static PerformanceProfiler instance;
        return instance;
    }

    void BeginFrame() {
        frameStartTime_ = std::chrono::high_resolution_clock::now();
        currentFrameEntries_.clear();
    }

    void EndFrame() {
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        double frameTimeMs = std::chrono::duration<double, std::milli>(frameEndTime - frameStartTime_).count();

        frameTimeHistory_.push_back(frameTimeMs);
        if (frameTimeHistory_.size() > historySize_) {
            frameTimeHistory_.erase(frameTimeHistory_.begin());
        }

        // Calculate FPS
        if (frameTimeMs > 0.0) {
            double fps = 1000.0 / frameTimeMs;
            fpsHistory_.push_back(static_cast<float>(fps));
            if (fpsHistory_.size() > historySize_) {
                fpsHistory_.erase(fpsHistory_.begin());
            }
        }

        // Update entries
        for (auto& entry : currentFrameEntries_) {
            auto& stats = entryStats_[entry.name];
            stats.totalTime += entry.timeMs;
            stats.callCount++;
            stats.minTime = std::min(stats.minTime, entry.timeMs);
            stats.maxTime = std::max(stats.maxTime, entry.timeMs);

            entry.avgTimeMs = stats.totalTime / stats.callCount;
            entry.minTimeMs = stats.minTime;
            entry.maxTimeMs = stats.maxTime;
            entry.callCount = stats.callCount;
            entry.percentage = (entry.timeMs / frameTimeMs) * 100.0;
        }

        frameCount_++;
    }

    void BeginSection(const std::string& name) {
        SectionTimer timer;
        timer.name = name;
        timer.startTime = std::chrono::high_resolution_clock::now();
        activeSections_.push_back(timer);
    }

    void EndSection() {
        if (activeSections_.empty()) return;

        auto timer = activeSections_.back();
        activeSections_.pop_back();

        auto endTime = std::chrono::high_resolution_clock::now();
        double timeMs = std::chrono::duration<double, std::milli>(endTime - timer.startTime).count();

        ProfilerEntry entry;
        entry.name = timer.name;
        entry.timeMs = timeMs;
        entry.color = GetColorForSection(timer.name);

        currentFrameEntries_.push_back(entry);
    }

    void RenderUI() {
        if (!showProfiler_) return;

        ImGui::SetNextWindowSize(ImVec2(600, 800), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("⚡ Performance Profiler", &showProfiler_)) {

            // FPS Graph
            ImGui::Text("FPS: %.1f (%.2f ms)", GetCurrentFPS(), GetCurrentFrameTime());
            ImGui::PlotLines("##FPS", fpsHistory_.data(), (int)fpsHistory_.size(), 0, nullptr, 0.0f, 200.0f, ImVec2(0, 80));

            ImGui::Separator();

            // Frame Time Graph
            ImGui::Text("Frame Time");
            std::vector<float> frameTimesFloat(frameTimeHistory_.begin(), frameTimeHistory_.end());
            ImGui::PlotLines("##FrameTime", frameTimesFloat.data(), (int)frameTimesFloat.size(), 0, nullptr, 0.0f, 33.3f, ImVec2(0, 80));

            ImGui::Separator();

            // Stats
            // PRIu64 rather than %llu: uint64_t is `unsigned long` on LP64
            // targets, so %llu is a type mismatch and therefore undefined
            // behaviour in a varargs call.
            ImGui::Text("Total Frames: %" PRIu64, frameCount_);
            ImGui::Text("Avg FPS: %.1f", GetAverageFPS());
            ImGui::Text("Min FPS: %.1f", GetMinFPS());
            ImGui::Text("Max FPS: %.1f", GetMaxFPS());

            ImGui::Separator();

            // Section breakdown
            ImGui::Text("Section Breakdown:");

            if (ImGui::BeginTable("ProfilerTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Section");
                ImGui::TableSetupColumn("Time (ms)");
                ImGui::TableSetupColumn("Percent");
                ImGui::TableSetupColumn("Calls");
                ImGui::TableSetupColumn("Avg (ms)");
                ImGui::TableHeadersRow();

                // Sort by time descending
                std::vector<ProfilerEntry> sortedEntries = currentFrameEntries_;
                std::sort(sortedEntries.begin(), sortedEntries.end(),
                    [](const ProfilerEntry& a, const ProfilerEntry& b) {
                        return a.timeMs > b.timeMs;
                    });

                for (const auto& entry : sortedEntries) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::TextColored(entry.color, "%s", entry.name.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", entry.timeMs);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.1f%%", entry.percentage);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", entry.callCount);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", entry.avgTimeMs);
                }

                ImGui::EndTable();
            }

            ImGui::Separator();

            // GPU Stats (placeholder)
            ImGui::Text("GPU Stats:");
            ImGui::Text("  Memory Used: %.1f MB", gpuMemoryUsedMB_);
            ImGui::Text("  Draw Calls: %d", drawCalls_);
            ImGui::Text("  Triangles: %d", triangles_);

            ImGui::Separator();

            // Controls
            if (ImGui::Button("Reset Stats")) {
                ResetStats();
            }

            ImGui::SameLine();
            ImGui::Checkbox("Enable Profiling", &enabled_);
        }
        ImGui::End();
    }

    void ToggleProfiler() {
        showProfiler_ = !showProfiler_;
    }

    void SetEnabled(bool enabled) {
        enabled_ = enabled;
    }

    bool IsEnabled() const {
        return enabled_;
    }

    double GetCurrentFPS() const {
        if (fpsHistory_.empty()) return 0.0;
        return fpsHistory_.back();
    }

    double GetCurrentFrameTime() const {
        if (frameTimeHistory_.empty()) return 0.0;
        return frameTimeHistory_.back();
    }

    double GetAverageFPS() const {
        if (fpsHistory_.empty()) return 0.0;
        double sum = 0.0;
        for (float fps : fpsHistory_) {
            sum += fps;
        }
        return sum / fpsHistory_.size();
    }

    double GetMinFPS() const {
        if (fpsHistory_.empty()) return 0.0;
        return *std::min_element(fpsHistory_.begin(), fpsHistory_.end());
    }

    double GetMaxFPS() const {
        if (fpsHistory_.empty()) return 0.0;
        return *std::max_element(fpsHistory_.begin(), fpsHistory_.end());
    }

    void SetGPUStats(float memoryMB, int drawCalls, int triangles) {
        gpuMemoryUsedMB_ = memoryMB;
        drawCalls_ = drawCalls;
        triangles_ = triangles;
    }

    void ResetStats() {
        entryStats_.clear();
        frameCount_ = 0;
    }

private:
    PerformanceProfiler() = default;

    struct SectionTimer {
        std::string name;
        std::chrono::high_resolution_clock::time_point startTime;
    };

    struct EntryStats {
        double totalTime = 0.0;
        int callCount = 0;
        double minTime = 999999.0;
        double maxTime = 0.0;
    };

    ImVec4 GetColorForSection(const std::string& name) {
        // Hash the name to get a consistent color
        size_t hash = std::hash<std::string>{}(name);
        float h = (hash % 360) / 360.0f;

        // Convert HSV to RGB
        float r, g, b;
        float s = 0.7f, v = 0.9f;

        int i = (int)(h * 6);
        float f = h * 6 - i;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);

        switch (i % 6) {
            case 0: r = v, g = t, b = p; break;
            case 1: r = q, g = v, b = p; break;
            case 2: r = p, g = v, b = t; break;
            case 3: r = p, g = q, b = v; break;
            case 4: r = t, g = p, b = v; break;
            case 5: r = v, g = p, b = q; break;
            default: r = v, g = t, b = p; break;
        }

        return ImVec4(r, g, b, 1.0f);
    }

    bool enabled_ = true;
    bool showProfiler_ = true;
    size_t historySize_ = 120; // 2 seconds at 60fps

    std::chrono::high_resolution_clock::time_point frameStartTime_;
    std::vector<SectionTimer> activeSections_;
    std::vector<ProfilerEntry> currentFrameEntries_;
    std::unordered_map<std::string, EntryStats> entryStats_;

    std::vector<double> frameTimeHistory_;
    std::vector<float> fpsHistory_;
    uint64_t frameCount_ = 0;

    // GPU stats
    float gpuMemoryUsedMB_ = 0.0f;
    int drawCalls_ = 0;
    int triangles_ = 0;
};

// RAII helper for profiling sections
class ProfileScope {
public:
    ProfileScope(const std::string& name) {
        PerformanceProfiler::Get().BeginSection(name);
    }

    ~ProfileScope() {
        PerformanceProfiler::Get().EndSection();
    }
};

// Macros for easy profiling
#define PROFILE_SCOPE(name) ProfileScope __profileScope##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

} // namespace Nexus
