#include "ui_backpack.h"
#include "font_manager.h"
#include "render_pipeline.h"
#include "inventory_system.h"
#include <cstdio>
#include <cstring>

namespace UIBackpack {
    static std::vector<BackpackGear> gearList;

    void init() {
        gearList.clear();
    }

    void loadGearList() {
        gearList.clear();
        for (int i = 0; i < Inventory::MAX_SLOTS; i++) {
            Inventory::InventoryItem* item = Inventory::getItemAtSlot(i);
            if (item) {
                BackpackGear gear;
                gear.assetId = i;
                gear.name = item->name;
                gear.isTool = item->isTool;
                gear.isHopperBin = item->isHopperBin;
                gear.isEquipped = item->isEquipped;
                gear.isActive = item->isActive;
                gear.textureId = item->textureId;
                gear.toolTip = item->toolTip;
                gearList.push_back(gear);
            }
        }
    }

    BackpackGear* getGear(int index) {
        if (index >= 0 && index < (int)gearList.size())
            return &gearList[index];
        return nullptr;
    }

    int getGearCount() {
        return (int)gearList.size();
    }

    void equipGear(int index) {
        if (index < 0 || index >= (int)gearList.size()) return;
        // Unequip current gear
        for (auto& g : gearList) {
            if (g.isEquipped && !g.isHopperBin) g.isEquipped = false;
        }
        gearList[index].isEquipped = true;
        // Call the inventory system to handle Tool vs HopperBin logic
        Inventory::equipGear(index);
    }

    void drawBackpackUI(int scrollOffset, int selectedSlot) {
        RenderPipeline::beginBottom();

        int x = 0, y = 0, w = 320, h = 240;

        // Background
        FontManager::drawText("                                                              ", 0, 0, 0, 0, 0, FontSize::Size12);

        // === "back" button (top-left) ===
        FontManager::drawText("back", 5, 5, 200, 200, 200, FontSize::Size12);

        // === Gear list area ===
        // Center area: (5, 20) to (295, 200)
        int listX = 5, listY = 20, listW = 290, listH = 180;

        // Draw list border
        // Draw gear items
        int visibleItems = 8;
        for (int i = 0; i < visibleItems; i++) {
            int idx = scrollOffset + i;
            if (idx >= (int)gearList.size()) break;

            const BackpackGear& gear = gearList[idx];
            int textY = listY + i * 22;

            // Selection highlight
            if (idx == selectedSlot) {
                FontManager::drawText(">", listX, textY, 255, 255, 0, FontSize::Size12);
                listX += 15;
            }

            // Gear name (truncated to 20 chars)
            char displayName[21];
            strncpy(displayName, gear.name.c_str(), 20);
            displayName[20] = '\0';

            uint8_t r, g, b;
            if (gear.isHopperBin) {
                // HopperBin: Green text when active, yellow when inactive
                if (gear.isActive) {
                    r = 0; g = 255; b = 0;
                } else {
                    r = 255; g = 255; b = 0;
                }
            } else if (idx == selectedSlot) {
                r = 255; g = 255; b = 0;
            } else {
                r = 200; g = 200; b = 200;
            }
            FontManager::drawText(displayName, listX + 5, textY, r, g, b, FontSize::Size12);

            // Gear type indicators
            int indicatorX = listX + listW - 40;
            if (gear.isHopperBin) {
                // [H] for HopperBin, [A] for active
                char hopperInd[3];
                if (gear.isActive) {
                    strncpy(hopperInd, "[A]", 3);
                    FontManager::drawText(hopperInd, indicatorX, textY, 0, 255, 0, FontSize::Size12);
                } else {
                    strncpy(hopperInd, "[H]", 3);
                    FontManager::drawText(hopperInd, indicatorX, textY, 255, 255, 0, FontSize::Size12);
                }
            } else if (gear.isTool) {
                // [T] for Tool
                FontManager::drawText("[T]", indicatorX, textY, 100, 150, 255, FontSize::Size12);
            }

            // Equipped indicator
            if (gear.isEquipped) {
                FontManager::drawText("*", listX + listW - 12, textY, 255, 255, 255, FontSize::Size12);
            }
        }

        // === Scrollbar (right side) ===
        int barX = 298, barY = 20, barH = 180;
        if ((int)gearList.size() > visibleItems) {
            int thumbH = (visibleItems * barH) / (int)gearList.size();
            if (thumbH < 10) thumbH = 10;
            int thumbY = barY + (scrollOffset * barH) / (int)gearList.size();
            FontManager::drawText("|", barX, thumbY, 150, 150, 150, FontSize::Size12);
        }

        // === Hotbar (bottom) ===
        // 10 slots at bottom
        int hotbarY = 205;
        int slotW = 28;
        int slotGap = 2;
        int totalW = 10 * slotW + 9 * slotGap;
        int hotbarX = (320 - totalW) / 2;

        for (int i = 0; i < 10; i++) {
            int sx = hotbarX + i * (slotW + slotGap);

            // Slot background
            char slotChar = ' ';
            FontManager::drawText(&slotChar, sx, hotbarY, 40, 40, 40, FontSize::Size12);

            // Slot number
            char num[4];
            snprintf(num, sizeof(num), "%d", i + 1);
            FontManager::drawText(num, sx + 2, hotbarY + 2, 100, 100, 100, FontSize::Size12);

            // If gear is in hotbar, show gear icon
            if (i < (int)gearList.size()) {
                BackpackGear& gear = gearList[i];
                if (gear.isHopperBin && gear.isActive) {
                    // Active HopperBin: Green border
                    FontManager::drawText("H", sx + 6, hotbarY + 8, 0, 255, 0, FontSize::Size12);
                } else if (gear.isTool) {
                    // Tool: Blue
                    FontManager::drawText("T", sx + 6, hotbarY + 8, 100, 150, 255, FontSize::Size12);
                } else {
                    char firstChar[2] = { gear.name[0], '\0' };
                    FontManager::drawText(firstChar, sx + 4, hotbarY + 10, 255, 255, 255, FontSize::Size12);
                }
            }
        }
    }

    bool processBackpackInput(int& scrollOffset, int& selectedSlot) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        // B button = back to game (or drop gear when combined with A)
        if (kDown & KEY_B) {
            return false; // Signal to close backpack
        }

        // Navigate gear list
        if (kDown & KEY_UP) {
            selectedSlot--;
            if (selectedSlot < 0) selectedSlot = getGearCount() - 1;
            if (selectedSlot < scrollOffset) scrollOffset = selectedSlot;
        }
        if (kDown & KEY_DOWN) {
            selectedSlot++;
            if (selectedSlot >= getGearCount()) selectedSlot = 0;
            if (selectedSlot >= scrollOffset + 8) scrollOffset = selectedSlot - 7;
        }

        // A button = equip selected gear
        if (kDown & KEY_A) {
            equipGear(selectedSlot);
        }

        // L/R buttons are used for Tool activation in-game, but here they navigate the list
        if (kDown & KEY_DLEFT) {
            selectedSlot--;
            if (selectedSlot < 0) selectedSlot = getGearCount() - 1;
        }
        if (kDown & KEY_DRIGHT) {
            selectedSlot++;
            if (selectedSlot >= getGearCount()) selectedSlot = 0;
        }

        return true; // Keep backpack open
    }

    void shutdown() {
        gearList.clear();
    }
}