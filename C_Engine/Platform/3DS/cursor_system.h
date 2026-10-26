#pragma once

#include <string>
#include <map>
#include "settings.h"

// Top-screen cursor system with year-adaptive cursor textures
// Provides a virtual mouse cursor on the top screen controlled by touch trackpad
// Cursor textures loaded from assets/cursor{YEAR}/ArrowCursor.png

namespace CursorSystem {

    enum class CursorState {
        Idle,
        Moving,
        LeftClick,
        RightClick
    };

    struct CursorTexture {
        std::string name;
        uint8_t* pixels = nullptr;  // Raw RGBA pixel data (stbi_load output)
        int width = 0, height = 0;
    };

    // Initialize cursor system
    void init();

    // Shutdown and free all cursor textures
    void shutdown();

    // Enable/disable cursor visibility
    void setEnabled(bool enabled);
    bool isEnabled();

    // Get cursor position
    int getCursorX();
    int getCursorY();

    // Set cursor position directly
    void setCursorPosition(int x, int y);

    // Trackpad update - called with touch delta from bottom screen trackpad
    // dx, dy are relative touch movements
    void trackpadUpdate(int dx, int dy);

    // Click state updates
    void setLeftClick(bool pressed);
    void setRightClick(bool pressed);

    // Check if click just happened (edge detection)
    bool isLeftClick();
    bool isRightClick();

    // Render cursor on top screen at current position
    void render();

    // Reload cursor texture for current content year
    void reloadCursorForYear();

    // Get cursor folder name for current year (e.g., "cursor2015")
    std::string getCursorFolder();

    // Get full path to cursor texture for a given year
    std::string getCursorPath(Settings::ContentYear year, const std::string& cursorName = "ArrowCursor.png");

    // Clamp cursor to top screen bounds
    void clampCursor();
};