#include "Platform.h"
#include "Logger.h"

namespace Nexus {

bool Platform::Initialize() {
    Logger::Info("Platform layer initialized for Windows");
    return true;
}

void Platform::Shutdown() {
    Logger::Info("Platform layer shutdown");
}

std::string Platform::GetPlatformName() {
    return "Windows";
}

bool Platform::IsConsoleSupported() {
    // Check if running on a console platform
    return false; // For now, desktop only
}

void Platform::SetConsoleMode(bool enabled) {
    // Console-specific optimizations would go here
}

} // namespace Nexus
