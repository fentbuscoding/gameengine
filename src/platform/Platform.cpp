#include "Platform.h"
#include "Logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

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
// Window creation is genuinely platform-specific and there is no cross-platform
// windowing backend wired up yet (SDL2 is optional and not required to build).
// Rather than pretend, these report failure clearly on platforms without an
// implementation so callers fail fast instead of dereferencing a null handle.

WindowHandle Platform::CreateGameWindow(const std::string& title, int width, int height) {
    (void)title;
    (void)width;
    (void)height;

    Logger::Error("Platform::CreateGameWindow is not implemented on " + GetPlatformName() +
                  " - build with SDL2 support or use a platform-specific window backend");
    return nullptr;
}

void Platform::DestroyGameWindow(WindowHandle window) {
    (void)window;
}

bool Platform::ProcessMessages() {
    // Drains whatever the host windowing system has queued and reports whether
    // the application should keep running. Owning the pump here is what lets
    // Engine::Run be the same code on every platform.
#if defined(_WIN32)
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
