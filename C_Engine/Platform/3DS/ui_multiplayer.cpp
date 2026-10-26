#include "ui_multiplayer.h"
#include "ui_main_menu.h"  // Complete GameEntry type for std::vector<GameEntry>
#include "citro_renderer.h"
#include "font_manager.h"
#include "multiplayer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <unistd.h>
#include <3ds.h>

UIMultiplayer g_uiMultiplayer;

static const char* kbRows[4] = {
    "1234567890-=",
    "qwertyuiop[]",
    "asdfghjkl;'",
    "zxcvbnm,./"
};

void UIMultiplayer::init() {
    ipAddress = "";
    portStr = "27016";
    mapName = "";
    userId = "";
    selectedServer = "";
    selectedServerIndex = 0;
    serverScrollOffset = 0;
    activeInput = 0;
    keyboardShow = false;
    serverList.clear();
}

void UIMultiplayer::drawMenu() {
    consoleClear();
    consoleSelect(NULL);

    int leftPanelW = 130;

    // Background
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            DrawPixel(x, y, 10, 10, 15);
        }
    }

    // Divider line
    for (int y = 0; y < 240; y++) {
        DrawPixel(leftPanelW, y, 60, 60, 70);
    }

    // Draw input fields on left panel
    drawTextInput("ip address:", 8, 8, leftPanelW - 16);
    drawTextInput("port:", 8, 40, leftPanelW - 16);
    drawTextInput("map name:", 8, 72, leftPanelW - 16);
    drawTextInput("user id:", 8, 104, leftPanelW - 16);

    // Host button
    int btnX = 16, btnY = 160, btnW = leftPanelW - 32, btnH = 28;
    for (int y = btnY; y < btnY + btnH; y++) {
        for (int x = btnX; x < btnX + btnW; x++) {
            DrawPixel(x, y, 40, 100, 180);
        }
    }
    // Button border
    for (int x = btnX; x < btnX + btnW; x++) {
        DrawPixel(x, btnY, 255, 255, 255);
        DrawPixel(x, btnY + btnH - 1, 255, 255, 255);
    }
    for (int y = btnY; y < btnY + btnH; y++) {
        DrawPixel(btnX, y, 255, 255, 255);
        DrawPixel(btnX + btnW - 1, y, 255, 255, 255);
    }
    FontManager::drawText("Host!", btnX + 8, btnY + 8, 255, 255, 255, FontSize::Size12);

    // Draw server list on the right
    drawServerList();

    // Draw scrollbar on far right
    drawScrollbar();
}

void UIMultiplayer::drawTextInput(const char* label, int x, int y, int w) {
    // Label
    FontManager::drawText(label, x, y, 180, 180, 180, FontSize::Size10);

    // Input box
    int boxY = y + 12;
    for (int dy = 0; dy < 16; dy++) {
        for (int dx = 0; dx < w; dx++) {
            DrawPixel(x + dx, boxY + dy, 30, 30, 40);
        }
    }

    // Value
    const char* val = "";
    if (activeInput == 0) val = ipAddress.c_str();
    else if (activeInput == 1) val = portStr.c_str();
    else if (activeInput == 2) val = mapName.c_str();
    else if (activeInput == 3) val = userId.c_str();

    FontManager::drawText(val, x + 2, boxY + 2, 255, 255, 255, FontSize::Size10);
}

void UIMultiplayer::drawServerList() {
    int listX = 132;
    int listY = 10;
    int listW = 176;
    int listH = 220;
    int lineHeight = 18;
    int visible = listH / lineHeight;

    // Draw list background
    for (int y = listY; y < 230; y++) {
        for (int x = listX; x < listX + listW; x++) {
            DrawPixel(x, y, 15, 15, 20);
        }
    }

    for (int i = 0; i < visible && (serverScrollOffset + i) < (int)serverList.size(); i++) {
        int idx = serverScrollOffset + i;
        int y = listY + i * lineHeight;
        const ServerEntry& s = serverList[idx];

        // Background highlight for selected
        if (idx == selectedServerIndex) {
            for (int yy = y; yy < y + lineHeight; yy++) {
                for (int xx = listX; xx < listX + listW; xx++) {
                    DrawPixel(xx, yy, 50, 100, 180);
                }
            }
        }

        // Draw server name
        char buf[128];
        snprintf(buf, sizeof(buf), "%s (%d/%d)", s.name.c_str(), s.players, s.maxPlayers);
        FontManager::drawText(buf, listX + 2, y + 4, 255, 255, 255, FontSize::Size10);
    }
}

void UIMultiplayer::drawScrollbar() {
    int total = (int)serverList.size();
    int visible = 220 / 18;
    if (total <= visible) return;

    int barX = 308;
    int barY = 10;
    int barH = 220;
    int thumbH = std::max(16, (visible * barH) / total);
    int thumbY = barY + ((serverScrollOffset * (barH - thumbH)) / std::max(1, total - visible));

    // Track
    for (int y = barY; y < barY + barH; y++) {
        for (int x = barX; x < 320; x++) {
            DrawPixel(x, y, 25, 25, 35);
        }
    }

    // Thumb
    for (int y = thumbY; y < thumbY + thumbH; y++) {
        for (int x = barX; x < 320; x++) {
            DrawPixel(x, y, 80, 130, 200);
        }
    }
}

void UIMultiplayer::processInput() {
    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    if (kDown & KEY_B) {
        // Exit
        return;
    }

    if (kDown & KEY_LEFT) {
        if (activeInput == 4) activeInput = 3;
        else if (activeInput > 0) activeInput--;
    }

    if (kDown & KEY_RIGHT) {
        if (activeInput < 4) activeInput++;
    }

    if (activeInput < 4) {
        // Text input mode - use keyboard
        if (kDown & KEY_A || kDown & KEY_X) {
            keyboardShow = !keyboardShow;
        }
    } else {
        // Server list navigation
        int total = (int)serverList.size();
        if (total > 0) {
            if (kDown & KEY_UP) {
                selectedServerIndex--;
                if (selectedServerIndex < 0) selectedServerIndex = total - 1;
                if (selectedServerIndex < serverScrollOffset) serverScrollOffset = selectedServerIndex;
            }
            if (kDown & KEY_DOWN) {
                selectedServerIndex++;
                if (selectedServerIndex >= total) selectedServerIndex = 0;
                int visible = 220 / 18;
                if (selectedServerIndex >= serverScrollOffset + visible)
                    serverScrollOffset = selectedServerIndex - visible + 1;
            }
            if (kDown & KEY_A) {
                selectedServer = serverList[selectedServerIndex].ip;
                return; // Join selected server
            }
        }
    }

    // Host button activation
    if (kDown & KEY_START) {
        // Validate User ID
        if (userId.empty()) {
            FontManager::drawText("User ID required!", 10, 195, 255, 100, 100, FontSize::Size12);
            return;
        }

        // Check for duplicate User ID
        if (!Multiplayer::isValidUserId(userId.c_str())) {
            FontManager::drawText("User ID already in use!", 10, 195, 255, 100, 100, FontSize::Size12);
            return;
        }

        // Host the game
        Multiplayer::startHosting(mapName, userId);
        return;
    }
}

std::string UIMultiplayer::run() {
    init();
    bool running = true;
    std::string result = "";

    while (running) {
        drawMenu();
        processInput();
        usleep(16666);  // ~60fps frame rate
    }

    return result;
}