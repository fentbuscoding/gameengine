#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <memory>

namespace Nexus {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static void Initialize(const std::string& filename = "nexus.log");
    static void Shutdown();
    
    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    
    static void SetLogLevel(LogLevel level) { logLevel_ = level; }
    static void SetConsoleOutput(bool enable) { consoleOutput_ = enable; }

private:
    static void Log(LogLevel level, const std::string& message);
    static std::string GetTimestamp();
    static std::string LogLevelToString(LogLevel level);
    
    static std::unique_ptr<std::ofstream> logFile_;
    static LogLevel logLevel_;
    static bool consoleOutput_;
    static bool initialized_;
};

} // namespace Nexus
