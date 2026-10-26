#pragma once
#include <string>
#include <3ds.h>

namespace Screenshot {
    // Initialize screenshot system
    void init();

    // Capture current top screen frame buffer and save as PNG
    bool capture();

    // Capture with custom path
    bool capture(const std::string& path);

    // Get screenshot count
    int getCount();

    // Get screenshot file path by index
    std::string getScreenshotPath(int index);

    // Delete screenshot by index
    bool deleteScreenshot(int index);

    // Get screenshot directory
    const char* getDirectory();

    // Shutdown
    void shutdown();
}