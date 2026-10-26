/*
    OpenBLOX Debug Logger
    Writes timestamped log entries to sdmc:/OpenBLOX/logs/debug-YYYY-MM-DD_HH-MM-SS.txt
*/

#ifndef _DEBUG_LOGGER_H_
#define _DEBUG_LOGGER_H_

#include <string>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

namespace DebugLogger {

    // Initialize logger - creates logs directory and opens log file
    // If enableLogging is false, logging is disabled (no-op mode)
    void init(bool enableLogging);

    // Shutdown logger and close file
    void shutdown();

    // Log a message (writes to both console and file if enabled)
    void log(LogLevel level, const char* file, int line, const char* fmt, ...);

    // Check if logging is enabled
    bool isEnabled();

    // Get current log file path (for display in UI)
    const char* getLogFilePath();

    // Convenience macros
    #define LOG_DEBUG(fmt, ...)   DebugLogger::log(LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)    DebugLogger::log(LogLevel::Info, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_WARNING(fmt, ...) DebugLogger::log(LogLevel::Warning, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...)   DebugLogger::log(LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

} // namespace DebugLogger

#endif // _DEBUG_LOGGER_H_
