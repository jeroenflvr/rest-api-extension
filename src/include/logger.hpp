#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>

// Logger class definition
class Logger {
public:
    // Log levels enumeration
    enum LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    // Constructor and destructor
    Logger(const std::string& filename);
    ~Logger();

    // Log function with contextual information
    void log(LogLevel level, const std::string& message, const std::string& file, const std::string& function, int line);

    // Macros for easier logging
    #define LOG_DEBUG(message) log(Logger::DEBUG, message, __FILE__, __FUNCTION__, __LINE__)
    #define LOG_INFO(message)  log(Logger::INFO, message, __FILE__, __FUNCTION__, __LINE__)
    #define LOG_WARN(message)  log(Logger::WARN, message, __FILE__, __FUNCTION__, __LINE__)
    #define LOG_ERROR(message) log(Logger::ERROR, message, __FILE__, __FUNCTION__, __LINE__)

private:
    std::ofstream logFile;

    // Helper functions to get timestamp and log level names
    std::string getCurrentTimestamp();
    std::string getLogLevelName(LogLevel level);
};

#endif // LOGGER_H
