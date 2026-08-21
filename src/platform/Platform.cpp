#include "Platform.h"
#include "Logger.h"
#include "Window.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <unistd.h>
#else
    #include <unistd.h>
#endif

namespace Nexus {

// Most of this class was previously declared but never defined, so any call to
// Platform::Sleep, Platform::GetTime, Platform::FileExists or
// Platform::GetExecutablePath was an unresolved symbol at link time. Engine code
// worked around that by calling Win32 directly, which is what tied the core to
// Windows in the first place. These implementations give that code somewhere
// portable to go.

namespace {

std::vector<std::unique_ptr<Window>>& OwnedWindows() {
    static std::vector<std::unique_ptr<Window>> windows;
    return windows;
}

std::vector<std::unique_ptr<Window>>::iterator FindOwnedWindow(const Window* target) {
    auto& windows = OwnedWindows();
    return std::find_if(windows.begin(), windows.end(),
                        [target](const std::unique_ptr<Window>& window) {
                            return window.get() == target;
                        });
}

} // namespace

bool Platform::isInitialized_ = false;

bool Platform::Initialize() {
    if (isInitialized_) {
        return true;
    }

    isInitialized_ = true;
    Logger::Info("Platform layer initialized for " + GetPlatformName());
    return true;
}

void Platform::Shutdown() {
    if (!isInitialized_) {
        return;
    }

    // Close anything CreateGameWindow handed out. Leaving windows open past
    // shutdown leaks the SDL video subsystem, and on macOS an unclosed window
    // keeps the process alive after main returns.
    OwnedWindows().clear();

    isInitialized_ = false;
    Logger::Info("Platform layer shutdown");
}

std::string Platform::GetPlatformName() {
    // Previously hard-coded to "Windows", which made every log line and any
    // platform-conditional logic wrong everywhere else.
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#elif defined(__FreeBSD__)
    return "FreeBSD";
#else
    return "Unknown";
#endif
}

bool Platform::IsConsoleSupported() {
    // Console backends are selected at build time via ENABLE_CONSOLE_PLATFORMS
    // and none are wired up yet.
    return false;
}

void Platform::SetConsoleMode(bool enabled) {
    (void)enabled;
}

// --- Timing -----------------------------------------------------------------

double Platform::GetTime() {
    // steady_clock rather than system_clock: this feeds frame pacing, which must
    // not jump if the wall clock is adjusted.
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point origin = Clock::now();

    return std::chrono::duration<double>(Clock::now() - origin).count();
}

void Platform::Sleep(int milliseconds) {
    if (milliseconds <= 0) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

int Platform::GetSystemMemoryUsagePercent() {
#if defined(_WIN32)
    MEMORYSTATUSEX memStatus = {};
    memStatus.dwLength = sizeof(memStatus);
    if (!GlobalMemoryStatusEx(&memStatus)) {
        return -1;
    }
    return static_cast<int>(memStatus.dwMemoryLoad);

#elif defined(__linux__)
    // MemAvailable is the right field rather than MemFree: the kernel counts
    // reclaimable page cache and slab as available, so MemFree alone reports
    // near-exhaustion on any machine that has been up for a while.
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo) {
        return -1;
    }

    long long totalKb = 0;
    long long availableKb = -1;
    std::string key;
    long long value = 0;
    std::string unit;

    while (meminfo >> key >> value >> unit) {
        if (key == "MemTotal:") {
            totalKb = value;
        } else if (key == "MemAvailable:") {
            availableKb = value;
            break;      // MemAvailable follows MemTotal; nothing else is needed.
        }
    }

    if (totalKb <= 0 || availableKb < 0) {
        return -1;
    }

    const long long usedKb = totalKb - availableKb;
    return static_cast<int>((usedKb * 100) / totalKb);

#else
    // No portable equivalent on this platform yet.
    return -1;
#endif
}

// --- File system ------------------------------------------------------------

bool Platform::FileExists(const std::string& path) {
    std::error_code ec;
    // The non-throwing overload matters here: callers use this to probe for
    // optional assets, where a permission error should read as "absent" rather
    // than terminate the process.
    return std::filesystem::is_regular_file(path, ec) && !ec;
}

std::string Platform::GetExecutablePath() {
#if defined(_WIN32)
    char buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        return {};
    }
    return std::string(buffer, length);

#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);   // First call reports the size needed.

    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    buffer.resize(std::char_traits<char>::length(buffer.c_str()));

    std::error_code ec;
    const std::filesystem::path resolved = std::filesystem::canonical(buffer, ec);
    return ec ? buffer : resolved.string();

#else
    std::error_code ec;
    const std::filesystem::path resolved = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (ec) {
        Logger::Warning("Could not resolve executable path: " + ec.message());
        return {};
    }
    return resolved.string();
#endif
}

// --- Window management ------------------------------------------------------
//
// These forward to Nexus::Window, the SDL2-backed window layer, so the engine
// gets one implementation on Windows, Linux and macOS instead of the Win32-only
// code that used to live inside Engine.cpp. Platform's API is handle-based and
// Platform owns the windows it creates so that ProcessMessages can report when
// they have all been closed; pass Window::GetNativeHandle() to the RHI as
// SwapChainDesc::windowHandle.


Window* Platform::CreateGameWindow(const std::string& title, int width, int height) {
    Window::Desc desc;
    desc.title = title;
    desc.width = width;
    desc.height = height;
    // Vulkan is the engine's cross-platform backend, and the surface capability
    // has to be requested when the window is created - SDL cannot add it later.
    desc.backend = Window::Backend::Vulkan;

    std::unique_ptr<Window> window = Window::Create(desc);
    if (!window) {
        return nullptr;
    }

    Window* raw = window.get();
    OwnedWindows().push_back(std::move(window));
    return raw;
}

void Platform::DestroyGameWindow(Window* window) {
    if (!window) {
        return;
    }

    auto& windows = OwnedWindows();
    auto it = FindOwnedWindow(window);
    if (it != windows.end()) {
        windows.erase(it);
    }
}

bool Platform::ProcessMessages() {
    // Drains whatever the host windowing system has queued and reports whether
    // the application should keep running. Owning the pump here is what lets
    // Engine::Run be the same code on every platform.
    auto& windows = OwnedWindows();

    if (!windows.empty()) {
        // One PumpEvents call drains the whole process queue and updates every
        // window, so the loop below only reads the resulting state.
        windows.front()->PumpEvents();

        for (const std::unique_ptr<Window>& window : windows) {
            if (window->IsCloseRequested()) {
                return false;
            }
        }
        return true;
    }

#if defined(_WIN32)
    // No Nexus::Window open, but the legacy Win32 runtime layer creates its own
    // HWND directly, so its messages still need pumping.
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
#else
    // No window means no message queue to drain; report "keep running" so a
    // headless host loop is not terminated by the absence of a window.
    return true;
#endif
}

} // namespace Nexus
