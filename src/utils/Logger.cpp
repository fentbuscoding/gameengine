#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Nexus {

std::unique_ptr<std::ofstream> Logger::logFile_ = nullptr;
LogLevel Logger::logLevel_ = LogLevel::Info;
bool Logger::consoleOutput_ = true;
bool Logger::initialized_ = false;

void Logger::Initialize(const std::string& filename) {
    if (initialized_) return;
    
    logFile_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    if (logFile_->is_open()) {
        initialized_ = true;
        Info("Logger initialized - " + filename);
    }
}

void Logger::Shutdown() {
    if (initialized_) {
        Info("Logger shutting down");
        if (logFile_ && logFile_->is_open()) {
            logFile_->close();
        }
        logFile_.reset();
        initialized_ = false;
    }
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::Debug, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::Info, message);
}

void Logger::Warning(const std::string& message) {
    Log(LogLevel::Warning, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::Error, message);
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < logLevel_) return;
    
    std::string timestamp = GetTimestamp();
    std::string levelStr = LogLevelToString(level);
    std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // Console output
    if (consoleOutput_) {
        if (level == LogLevel::Error) {
            std::cerr << logMessage << std::endl;
        } else {
            std::cout << logMessage << std::endl;
        }
    }
    
    // File output
    if (initialized_ && logFile_ && logFile_->is_open()) {
        *logFile_ << logMessage << std::endl;
        logFile_->flush();
    }
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

} // namespace Nexus
