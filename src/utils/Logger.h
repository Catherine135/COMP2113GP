#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <cstdio>
#include <vector>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
public:
    /**
     * @Michael-wzl
     * @brief Get the singleton instance of the Logger.
     * @return Reference to the Logger instance.
     */
    static Logger& getInstance();

    /**
     * @Michael-wzl
     * @brief Set the log file name.
     * @param filename Name of the log file.
     */
    void setLogFile(const std::string& filename);

    /**
     * @Michael-wzl
     * @brief Log a message with the specified log level.
     * @param level Log level of the message.
     * @param message Message to log.
     */
    void log(LogLevel level, const std::string& message);

    /**
     * @Michael-wzl
     * @brief Log an info message.
     * @param message Info message to log.
     */
    void info(const std::string& message);

    /**
     * @Michael-wzl
     * @brief Log a warning message.
     * @param message Warning message to log.
     */
    void warning(const std::string& message);

    /**
     * @Michael-wzl
     * @brief Log an error message.
     * @param message Error message to log.
     */
    void error(const std::string& message);

    /**
     * @Michael-wzl
     * @brief Log a debug message.
     * @param message Debug message to log.
     */
    void debug(const std::string& message);

    // Formatted logging with printf-style
    template<typename... Args>

    /**
     * @Michael-wzl
     * @brief Log a formatted message with the specified log level.
     * @param level Log level of the message.
     * @param format Format string (printf-style).
     * @param args Arguments for the format string.
     */
    void logf(LogLevel level, const char* format, Args&&... args) {
    // First determine required buffer size
    // Use snprintf with nullptr to compute needed size
    int required = 0;
    // suppress -Wformat-security for non-literal format strings
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
    required = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    if (required < 0) {
        log(level, std::string("[Logger] formatting error"));
        return;
    }
    std::vector<char> buffer(static_cast<size_t>(required) + 1);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
    std::snprintf(buffer.data(), buffer.size(), format, std::forward<Args>(args)...);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    log(level, std::string(buffer.data()));
    }

    template<typename... Args>
    void infof(const char* format, Args&&... args) {
        logf(LogLevel::INFO, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warningf(const char* format, Args&&... args) {
        logf(LogLevel::WARNING, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void errorf(const char* format, Args&&... args) {
        logf(LogLevel::ERROR, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debugf(const char* format, Args&&... args) {
        logf(LogLevel::DEBUG, format, std::forward<Args>(args)...);
    }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getCurrentTime() const;
    std::string levelToString(LogLevel level) const;

    std::ofstream logFile_;
    std::mutex mutex_;
    std::string filename_;
};

// Convenience logging macros that automatically include call-site file and function.
#define LOG_INFO(msg) Logger::getInstance().info(std::string(__FILE__) + ":" + __func__ + " - " + (msg))
#define LOG_INFOF(fmt, ...) Logger::getInstance().infof((std::string("%s:%s: ") + fmt).c_str(), __FILE__, __func__, ##__VA_ARGS__)
#define LOG_WARNING(msg) Logger::getInstance().warning(std::string(__FILE__) + ":" + __func__ + " - " + (msg))
#define LOG_WARNINGF(fmt, ...) Logger::getInstance().warningf((std::string("%s:%s: ") + fmt).c_str(), __FILE__, __func__, ##__VA_ARGS__)
#define LOG_ERROR(msg) Logger::getInstance().error(std::string(__FILE__) + ":" + __func__ + " - " + (msg))
#define LOG_ERRORF(fmt, ...) Logger::getInstance().errorf((std::string("%s:%s: ") + fmt).c_str(), __FILE__, __func__, ##__VA_ARGS__)
#define LOG_DEBUG(msg) Logger::getInstance().debug(std::string(__FILE__) + ":" + __func__ + " - " + (msg))
#define LOG_DEBUGF(fmt, ...) Logger::getInstance().debugf((std::string("%s:%s: ") + fmt).c_str(), __FILE__, __func__, ##__VA_ARGS__)