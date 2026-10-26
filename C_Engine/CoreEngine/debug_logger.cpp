/*
    OpenBLOX Debug Logger Implementation
*/

#include "debug_logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef BUILD_3DS
#include <3ds.h>
#endif

static bool s_enabled = false;
static FILE* s_logFile = nullptr;
static char s_logFilePath[256] = "";

namespace DebugLogger {

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error:   return "ERROR";
            default:                return "UNKNOWN";
        }
    }

    static std::string getBaseName(const char* filePath) {
        const char* base = strrchr(filePath, '/');
        if (base) return std::string(base + 1);
        return std::string(filePath);
    }

    void init(bool enableLogging) {
        if (!enableLogging) {
            s_enabled = false;
            s_logFile = nullptr;
            s_logFilePath[0] = '\0';
            return;
        }

        const char* logsDir = "sdmc:/OpenBLOX/logs/";

#ifdef BUILD_3DS
        // Create directories using mkdir
        mkdir("sdmc:/OpenBLOX", 0755);
        mkdir(logsDir, 0755);
#endif

        // Generate timestamped filename
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char filename[128];
        snprintf(filename, sizeof(filename), "debug-%04d-%02d-%02d_%02d-%02d-%02d.txt",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

        std::string fullPath = std::string(logsDir) + filename;
        strncpy(s_logFilePath, fullPath.c_str(), sizeof(s_logFilePath) - 1);
        s_logFilePath[sizeof(s_logFilePath) - 1] = '\0';

        s_logFile = fopen(fullPath.c_str(), "w");
        if (s_logFile) {
            fprintf(s_logFile, "=== OpenBLOX Debug Log ===\n");
            fprintf(s_logFile, "Started: %s", ctime(&now));
            fprintf(s_logFile, "Log path: %s\n", fullPath.c_str());
            fprintf(s_logFile, "========================\n\n");
            fflush(s_logFile);
            s_enabled = true;
            printf("[DebugLogger] Log file opened: %s\n", fullPath.c_str());
        } else {
            s_enabled = false;
            s_logFilePath[0] = '\0';
            printf("[DebugLogger] FAILED to open log file: %s\n", fullPath.c_str());
        }
    }

    void shutdown() {
        if (s_logFile) {
            time_t now = time(nullptr);
            fprintf(s_logFile, "\n=== Log ended at %s ===\n", ctime(&now));
            fclose(s_logFile);
            s_logFile = nullptr;
        }
        s_enabled = false;
        s_logFilePath[0] = '\0';
        printf("[DebugLogger] Log closed.\n");
    }

    void log(LogLevel level, const char* file, int line, const char* fmt, ...) {
        // Always print to console
        va_list args;
        va_start(args, fmt);
        const char* lvl = levelToString(level);

        // Format source location
        std::string src = getBaseName(file);
        char header[256];
        snprintf(header, sizeof(header), "[OpenBLOX][%s][%s:%d] ", lvl, src.c_str(), line);

        // Console output
        printf("%s", header);
        vprintf(fmt, args);
        printf("\n");
        va_end(args);

        // File output (if enabled)
        if (s_enabled && s_logFile) {
            time_t now = time(nullptr);
            struct tm* t = localtime(&now);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

            fprintf(s_logFile, "[%s][%s][%s:%d] ", timestamp, lvl, src.c_str(), line);
            va_start(args, fmt);
            vfprintf(s_logFile, fmt, args);
            va_end(args);
            fprintf(s_logFile, "\n");
            fflush(s_logFile);
        }
    }

    bool isEnabled() {
        return s_enabled;
    }

    const char* getLogFilePath() {
        return s_logFilePath;
    }

} // namespace DebugLogger
