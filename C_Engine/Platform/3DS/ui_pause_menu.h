#pragma once

#include "settings.h"
#include <3ds.h>

// Pause Menu UI for OpenBLOX
// Displays settings categories on bottom screen (left sidebar) and options on right

namespace UIPauseMenu {

    enum class PauseState {
        Inactive,      // Not paused
        Paused,        // Paused overlay shown
        Settings       // Settings menu open
    };

    // Initialize pause menu
    void init();

    // Toggle pause state
    void togglePause();

    // Get current pause state
    PauseState getState();

    // Set pause state
    void setState(PauseState state);

    // Draw pause menu (call once per frame)
    void draw();

    // Process input (call once per frame)
    void processInput();

    // Check if game should be paused (return true to pause game loop)
    bool isGamePaused();

    // Shutdown
    void shutdown();
}