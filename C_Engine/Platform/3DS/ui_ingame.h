#pragma once

#include <string>
#include <vector>
#include <3ds.h>

// In-game UI for OpenBLOX 3DS
// Top screen (400x240):
//   - Chat logs: top-left corner (semi-transparent background, opaque text)
//   - Leaderboard: top-right corner (semi-transparent background, opaque text)
//
// Bottom screen (320x240):
//   - Top Left: PAUSE, chat, backpack buttons
//   - Top Right: Username & health
//   - Center: Camera trackpad (touch area)
//   - Right: Hotbar for 9 gears with scrollbar
//   - Bottom: Row of 9 gear slots

struct HotbarSlot {
    int itemId;        // Asset ID or -1 for empty
    std::string name;  // Display name
    bool equipped;
};

struct PlayerInfo {
    std::string username;
    float health;      // 0-100
    float maxHealth;   // default 100
};

struct CameraTrackpad {
    float deltaX;      // Normalized -1 to 1
    float deltaY;      // Normalized -1 to 1
    bool active;       // Currently touching trackpad
    int startX;        // Touch start X
    int startY;        // Touch start Y
};

enum class InGameUIState {
    Playing,
    Paused,
    BackpackOpen
};

namespace UInGame {
    void init();
    void setPlayerInfo(const PlayerInfo& info);
    void setHotbar(const std::vector<HotbarSlot>& slots);

    // Draw bottom screen UI
    void drawInGameUI();

    // Draw top screen overlays (chat + leaderboard)
    void drawTopScreenOverlays();

    // Process input
    void processInGameInput(CameraTrackpad& trackpad, InGameUIState& state);

    int getSelectedHotbarSlot();
    void equipHotbarSlot(int index);
    bool isPaused();

    // Backpack integration
    bool isBackpackOpen();
    void toggleBackpack();

    void shutdown();
}