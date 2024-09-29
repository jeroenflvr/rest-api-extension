#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Constructor
Logger::Logger(const std::string& filename) {
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

// Destructor
Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& file, const std::string& function, int line) {
    if (logFile.is_open()) {
        logFile << getCurrentTimestamp() << " || " << getLogLevelName(level) << " || "
                << file << " || " << function << ":" << line << " || "
                << message << std::endl;
    } else {
        std::cerr << "Log file not open!" << std::endl;
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::getLogLevelName(LogLevel level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
        default:    return "UNKNOWN";
    }
}
