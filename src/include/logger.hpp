#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>


class Logger {
public:
   enum LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    Logger(const std::string& filename);
    ~Logger();

    void log(LogLevel level, const std::string& message, const std::string& file, const std::string& function, int line);

    #define LOG_DEBUG(message) log(Logger::DEBUG, message, __FILE__, __FUNCTION__, __LINE__)
    #define LOG_INFO(message)  log(Logger::INFO, message, __FILE__, __FUNCTION__, __LINE__)
    #define LOG_WARN(message)  log(Logger::WARN, message, __FILE__, __FUNCTION__, __LINE__)
    #define LOG_ERROR(message) log(Logger::ERROR, message, __FILE__, __FUNCTION__, __LINE__)

private:
    std::ofstream logFile;

    std::string getCurrentTimestamp();
    std::string getLogLevelName(LogLevel level);
};

extern Logger logger;

#endif // LOGGER_H
