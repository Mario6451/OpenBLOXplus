#pragma once

#include <string>
#include <vector>
#include <3ds.h>
#include "render_pipeline.h"
#include "font_manager.h"

// Backpack UI for OpenBLOX 3DS
// Bottom screen layout (320x240):
//   - Top-left: "back" button to close backpack
//   - Center: scrollable list of gear (backpack contents)
//   - Right: vertical scrollbar
//   - Bottom: hotbar with 10 slots for quick-access gear

struct BackpackGear {
    int assetId;
    std::string name;
    bool isTool;
    bool isEquipped;
    bool isHopperBin;
    bool isActive;
    std::string textureId;
    std::string toolTip;
};

static PrintConsole consoleBackpack;

namespace UIBackpack {
    void init();

    // Draw backpack screen on bottom display
    void drawBackpackUI(int scrollOffset, int selectedSlot);

    // Process backpack input, returns false when user presses BACK
    // Updates scrollOffset and selectedSlot
    bool processBackpackInput(int& scrollOffset, int& selectedSlot);

    // Equip gear from backpack
    void equipGear(int index);

    // Load gear list from inventory
    void loadGearList();

    // Get gear at index
    BackpackGear* getGear(int index);

    // Get total gear count
    int getGearCount();

    void shutdown();
}