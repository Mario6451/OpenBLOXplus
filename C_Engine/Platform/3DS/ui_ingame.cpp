#include "ui_ingame.h"
#include "font_manager.h"
#include "chat_system.h"
#include "leaderboard.h"
#include "citro_renderer.h"
#include "camera.h"
#include "settings.h"
#include "ui_theme.h"
#include "tools.h"
#include "voice_chat.h"
#include "cursor_system.h"
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <sys/stat.h>

using namespace UInGame;

// Layout constants (320x240 bottom screen)
// Top row: pause, chat, backpack buttons (left) | username & health (right)
static const int TOP_BTN_Y = 5;
static const int TOP_BTN_H = 18;
static const int PAUSE_BTN_X = 5;
static const int PAUSE_BTN_W = 40;
static const int CHAT_BTN_X = 50;
static const int CHAT_BTN_W = 35;
static const int BACKPACK_BTN_X = 90;
static const int BACKPACK_BTN_W = 50;

static const int USERNAME_X = 160;
static const int USERNAME_Y = 5;

// Center: Camera trackpad
static const int TRACKPAD_X = 10;
static const int TRACKPAD_Y = 30;
static const int TRACKPAD_W = 200;
static const int TRACKPAD_H = 160;
static const int TRACKPAD_CENTER_X = TRACKPAD_X + TRACKPAD_W / 2;
static const int TRACKPAD_CENTER_Y = TRACKPAD_Y + TRACKPAD_H / 2;

// Right side: Hotbar for 9 gears with scrollbar
static const int HOTBAR_X = 215;
static const int HOTBAR_Y = 30;
static const int HOTBAR_W = 50;
static const int HOTBAR_H = 160;
static const int HOTBAR_SLOT_H = 16;
static const int HOTBAR_SLOT_COUNT = 9;
static const int SLIDER_X = 245;
static const int SLIDER_Y = 35;
static const int SLIDER_W = 10;
static const int SLIDER_H = 150;
static const int SLIDER_THUMB_W = 14;
static const int SLIDER_THUMB_H = 16;

// Bottom row: 9 gear slots
static const int BOTTOM_BAR_Y = 200;
static const int BOTTOM_SLOT_W = 30;
static const int BOTTOM_SLOT_H = 35;
static const int BOTTOM_SLOT_GAP = 2;
static const int BOTTOM_SLOT_COUNT = 9;
static const int BOTTOM_START_X = (320 - (BOTTOM_SLOT_COUNT * (BOTTOM_SLOT_W + BOTTOM_SLOT_GAP) - BOTTOM_SLOT_GAP)) / 2;

// Top screen overlay constants (400x240)
static const int CHAT_PANEL_X = 5;
static const int CHAT_PANEL_Y = 5;
static const int CHAT_PANEL_W = 160;
static const int CHAT_PANEL_H = 60;

static const int LEADERBOARD_PANEL_Y = 5;
static const int LEADERBOARD_PANEL_H = 80;
static const int LEADERBOARD_FIXED_RIGHT_MARGIN = 5;

// State
static PlayerInfo g_playerInfo = {"Player", 100.0f, 100.0f};
static std::vector<HotbarSlot> g_hotbar;
static int g_selectedSlot = 0;
static bool g_isPaused = false;
static bool g_backpackOpen = false;
static bool g_chatOpen = false;
static bool g_touchDown = false;
static bool g_touchHeld = false;

// Zoom scrollbar state
static bool g_sliderDragging = false;
static bool g_sliderTouched = false;

static void drawSemiTransparentRectTop(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, float alpha) {
    // Draw semi-transparent panel background on top screen
    // Uses block characters with dim color to simulate transparency
    consoleSelect(NULL);
    
    // Dim the color based on alpha
    int dimFactor = (int)(200.0f * (1.0f - alpha)); // Reduced base brightness for better transparency feel
    uint8_t dr = (uint8_t)(r * dimFactor / 255);
    uint8_t dg = (uint8_t)(g * dimFactor / 255);
    uint8_t db = (uint8_t)(b * dimFactor / 255);
    
    // Draw filled rectangle using block characters
    int rows = h / 12;
    int cols = w / 8;
    
    for (int row = 0; row < rows; row++) {
        iprintf("\x1b[%d;1H", row + 1);
        for (int col = 0; col < cols; col++) {
            iprintf("\x1b[38;2;%d;%d;%dm%c", dr, dg, db, 0xDB);
        }
    }
    
    iprintf("\x1b[0m");
}

void UInGame::init() {
    g_playerInfo = {"Player", 100.0f, 100.0f};
    g_hotbar.clear();
    for (int i = 0; i < HOTBAR_SLOT_COUNT; i++) {
        g_hotbar.push_back({-1, "", false});
    }
    g_selectedSlot = 0;
    g_isPaused = false;
    g_backpackOpen = false;
    g_chatOpen = false;
    g_touchDown = false;
    g_touchHeld = false;
}

void UInGame::setPlayerInfo(const PlayerInfo& info) {
    g_playerInfo = info;
}

void UInGame::setHotbar(const std::vector<HotbarSlot>& slots) {
    g_hotbar.clear();
    for (size_t i = 0; i < slots.size() && i < HOTBAR_SLOT_COUNT; i++) {
        g_hotbar.push_back(slots[i]);
    }
    while ((int)g_hotbar.size() < HOTBAR_SLOT_COUNT) {
        g_hotbar.push_back({-1, "", false});
    }
}

void UInGame::drawInGameUI() {
    consoleSelect(NULL);

    // === TOP ROW: PAUSE | chat | backpack | USERNAME & HEALTH ===
    FontManager::drawText("pause", PAUSE_BTN_X, TOP_BTN_Y, g_isPaused ? 255 : 200, 200, 0, FontSize::Size12);
    FontManager::drawText("chat", CHAT_BTN_X, TOP_BTN_Y, g_chatOpen ? 255 : 200, 200, 200, FontSize::Size12);
    FontManager::drawText("backpack", BACKPACK_BTN_X, TOP_BTN_Y, g_backpackOpen ? 255 : 200, 200, 200, FontSize::Size12);

    // === VOICE CHAT PUSH-TO-TALK BUTTON (right of backpack) ===
    static const int VC_BTN_X = 145;
    static const int VC_BTN_W = 25;
    bool isVCActive = (VoiceChat::getState() == VoiceChat::VcState::Talking);
    FontManager::drawText("VC", VC_BTN_X, TOP_BTN_Y, isVCActive ? 255 : 180, isVCActive ? 80 : 200, isVCActive ? 80 : 200, FontSize::Size12);
    if (isVCActive) {
        FontManager::drawText("[TALKING]", VC_BTN_X, TOP_BTN_Y + 12, 255, 100, 100, FontSize::Size8);
    }

    // Username & health bar (top right)
    FontManager::drawText("username:", USERNAME_X, USERNAME_Y, 255, 255, 255, FontSize::Size12);
    FontManager::drawText(g_playerInfo.username.c_str(), USERNAME_X + 60, USERNAME_Y, 200, 200, 255, FontSize::Size12);

    int healthPct = 0;
    if (g_playerInfo.maxHealth > 0.0f) healthPct = (int)((g_playerInfo.health / g_playerInfo.maxHealth) * 100);
    uint8_t healthR = healthPct > 50 ? 0 : 255;
    uint8_t healthG = healthPct > 50 ? 255 : healthPct;
    char healthText[64];
    snprintf(healthText, sizeof(healthText), "health: %d%%", healthPct);
    FontManager::drawText(healthText, USERNAME_X, USERNAME_Y + 12, healthR, healthG, 0, FontSize::Size12);

    // === CAMERA TRACKPAD (center) ===
    FontManager::drawText("trackpad for camera", TRACKPAD_X + 30, TRACKPAD_Y + 65, 150, 150, 150, FontSize::Size12);

    // Trackpad border indicators
    FontManager::drawText("^", TRACKPAD_CENTER_X - 4, TRACKPAD_Y + 5, 120, 120, 120, FontSize::Size12);
    FontManager::drawText("v", TRACKPAD_CENTER_X - 4, TRACKPAD_Y + TRACKPAD_H - 15, 120, 120, 120, FontSize::Size12);
    FontManager::drawText("<", TRACKPAD_X + 10, TRACKPAD_CENTER_Y - 4, 120, 120, 120, FontSize::Size12);
    FontManager::drawText(">", TRACKPAD_X + TRACKPAD_W - 20, TRACKPAD_CENTER_Y - 4, 120, 120, 120, FontSize::Size12);

    // === GEAR HOTBAR (right side) ===
    // Get content year for styling
    int contentYear = 2012;
    switch(Settings::get().general.contentYear) {
        case Settings::ContentYear::Year2007: contentYear = 2007; break;
        case Settings::ContentYear::Year2011: contentYear = 2011; break;
        case Settings::ContentYear::Year2012: contentYear = 2012; break;
        case Settings::ContentYear::Year2015: contentYear = 2015; break;
        case Settings::ContentYear::Year2016: contentYear = 2016; break;
        case Settings::ContentYear::Year2019: contentYear = 2019; break;
        case Settings::ContentYear::Year2022: contentYear = 2022; break;
    }

    // Year-themed colors for hotbar
    uint8_t hotbarR, hotbarG, hotbarB;
    if (contentYear <= 2012) {
        hotbarR = 255; hotbarG = 255; hotbarB = 0;
    } else if (contentYear <= 2016) {
        hotbarR = 80; hotbarG = 160; hotbarB = 255;
    } else if (contentYear <= 2019) {
        hotbarR = 200; hotbarG = 120; hotbarB = 255;
    } else {
        hotbarR = 100; hotbarG = 220; hotbarB = 180;
    }

    // Draw hotbar slots
    for (int i = 0; i < HOTBAR_SLOT_COUNT; i++) {
        int slotY = HOTBAR_Y + i * HOTBAR_SLOT_H;
        const HotbarSlot& slot = g_hotbar[i];

        char numText[8];
        snprintf(numText, sizeof(numText), "%d", i + 1);
        uint8_t slotR = (i == g_selectedSlot) ? hotbarR : 100;
        uint8_t slotG = (i == g_selectedSlot) ? hotbarG : 100;
        uint8_t slotB = (i == g_selectedSlot) ? hotbarB : 100;
        FontManager::drawText(numText, HOTBAR_X + 5, slotY + 2, slotR, slotG, slotB, FontSize::Size12);

        if (!slot.name.empty()) {
            char shortName[16];
            strncpy(shortName, slot.name.c_str(), 15);
            shortName[15] = '\0';
            FontManager::drawText(shortName, HOTBAR_X + 15, slotY + 2, 255, 255, 255, FontSize::Size12);
        }
    }

    // Scrollbar for hotbar
    float scrollNorm = 0.0f;
    if (HOTBAR_SLOT_COUNT > 1) scrollNorm = (float)g_selectedSlot / (HOTBAR_SLOT_COUNT - 1);
    int thumbY = SLIDER_Y + (int)(scrollNorm * (SLIDER_H - SLIDER_THUMB_H));
    for (int row = 0; row < SLIDER_THUMB_H / 12; row++) {
        int y = thumbY + row * 12;
        FontManager::drawText("▓", SLIDER_X, y, hotbarR, hotbarG, hotbarB, FontSize::Size12);
    }

    // === BOTTOM GEAR SLOTS ===
    for (int i = 0; i < BOTTOM_SLOT_COUNT; i++) {
        int slotX = BOTTOM_START_X + i * (BOTTOM_SLOT_W + BOTTOM_SLOT_GAP);
        int slotY = BOTTOM_BAR_Y;
        const HotbarSlot& slot = g_hotbar[i];

        char numText[8];
        snprintf(numText, sizeof(numText), "%d", i + 1);
        uint8_t slotR = (i == g_selectedSlot) ? 255 : 100;
        uint8_t slotG = (i == g_selectedSlot) ? 255 : 100;
        uint8_t slotB = (i == g_selectedSlot) ? 0 : 100;
        FontManager::drawText(numText, slotX + 2, slotY + 2, slotR, slotG, slotB, FontSize::Size12);

        if (!slot.name.empty()) {
            char shortName[16];
            strncpy(shortName, slot.name.c_str(), 15);
            shortName[15] = '\0';
            FontManager::drawText(shortName, slotX + 2, slotY + 16, 255, 255, 255, FontSize::Size12);
        }

        if (slot.equipped) {
            FontManager::drawText("*", slotX + 18, slotY + 2, 255, 255, 0, FontSize::Size12);
        }
    }

    // === PAUSE OVERLAY ===
    if (g_isPaused) {
        consoleClear();
        FontManager::drawText("=== GAME PAUSED ===", 100, 80, 255, 255, 0, FontSize::Size24);
        FontManager::drawText("START: Resume", 100, 110, 200, 200, 200, FontSize::Size16);
        FontManager::drawText("X: Settings", 100, 130, 200, 200, 200, FontSize::Size16);
        FontManager::drawText("B: Leave Game", 100, 150, 200, 200, 200, FontSize::Size16);
    }
}

void UInGame::drawTopScreenOverlays() {
    // Get content year for styling
    int contentYear = 2012;
    switch(Settings::get().general.contentYear) {
        case Settings::ContentYear::Year2007: contentYear = 2007; break;
        case Settings::ContentYear::Year2011: contentYear = 2011; break;
        case Settings::ContentYear::Year2012: contentYear = 2012; break;
        case Settings::ContentYear::Year2015: contentYear = 2015; break;
        case Settings::ContentYear::Year2016: contentYear = 2016; break;
        case Settings::ContentYear::Year2019: contentYear = 2019; break;
        case Settings::ContentYear::Year2022: contentYear = 2022; break;
    }
    ThemeManager::applyTheme(contentYear);
    ThemeManager::UITheme& theme = ThemeManager::getCurrentTheme();

    // Year-based panel colors (semi-transparent background, opaque text)
    uint8_t panelR = 0, panelG = 0, panelB = 0;
    float panelAlpha = 0.6f; // Slightly transparent
    FontSize headerFontSize = FontSize::Size12;
    FontSize bodyFontSize = FontSize::Size12;
    const char* fontName = "Arial"; // Default

    if (contentYear <= 2007) {
        // 2007: Basic blocky style
        panelR = 20; panelG = 20; panelB = 20;
        headerFontSize = FontSize::Size12;
        bodyFontSize = FontSize::Size12;
        fontName = "ErasDemi";
    } else if (contentYear <= 2012) {
        // 2007-2012: Classic blocky style
        panelR = 0; panelG = 0; panelB = 0;
        headerFontSize = FontSize::Size12;
        bodyFontSize = FontSize::Size12;
        fontName = "ErasDemi";
    } else if (contentYear <= 2016) {
        // 2015-2016: Smooth style
        panelR = 20; panelG = 30; panelB = 50;
        headerFontSize = FontSize::Size16;
        bodyFontSize = FontSize::Size12;
        fontName = "SourceSans";
    } else if (contentYear <= 2019) {
        // 2019: Modern dark style
        panelR = 30; panelG = 30; panelB = 40;
        headerFontSize = FontSize::Size16;
        bodyFontSize = FontSize::Size16;
        fontName = "SourceSans";
    } else {
        // 2020-2022: Sleek modern style
        panelR = 40; panelG = 45; panelB = 55;
        headerFontSize = FontSize::Size16;
        bodyFontSize = FontSize::Size16;
        fontName = "SourceSans";
    }

    FontFace fontFace;
    if (strcmp(fontName, "SourceSans") == 0) fontFace = FontFace::SourceSansPro;
    else if (strcmp(fontName, "ComicSans") == 0) fontFace = FontFace::ComicSans;
    else if (strcmp(fontName, "ErasDemi") == 0) fontFace = FontFace::Legacy;
    else fontFace = FontFace::Arial;

    consoleSelect(NULL);

    // Draw chat logs panel (top-left) with semi-transparent background
    drawSemiTransparentRectTop(CHAT_PANEL_X, CHAT_PANEL_Y, CHAT_PANEL_W, CHAT_PANEL_H, panelR, panelG, panelB, panelAlpha);

    // Draw "CHAT LOGS" header - opaque text
    FontManager::drawTextTopWithFont("CHAT LOGS", CHAT_PANEL_X + 5, CHAT_PANEL_Y + 2, 255, 255, 255, fontFace, FontStyle::Bold, headerFontSize);

    // Draw chat messages with OPAQUE text (using theme font)
    ChatSystem::drawChatHistory(fontName);
    ChatSystem::drawChatBubbles();

    // Leaderboard: Dynamic width calculation (expands left, right edge fixed)
    auto scores = Leaderboard::getTopScores("", 5);
    int maxTextWidth = FontManager::getTextWidth("LEADERBOARD", headerFontSize) + 10;
    
    // Calculate max entry width
    char maxEntryText[64];
    for (size_t i = 0; i < scores.size(); i++) {
        snprintf(maxEntryText, sizeof(maxEntryText), "#%d %s: %llu", (int)scores[i].rank, scores[i].username.c_str(), (unsigned long long)scores[i].score);
        int len = FontManager::getTextWidth(maxEntryText, bodyFontSize) + 10;
        if (len > maxTextWidth) maxTextWidth = len;
    }
    
    int leaderboardW = maxTextWidth;
    if (leaderboardW > 200) leaderboardW = 200; // Cap max width
    if (leaderboardW < 80) leaderboardW = 80;  // Min width
    int leaderboardX = 400 - leaderboardW - 5; // Fixed right edge, expands left
    int leaderboardY = LEADERBOARD_PANEL_Y;

    // Draw leaderboard panel (top-right) with semi-transparent background
    drawSemiTransparentRectTop(leaderboardX, leaderboardY, leaderboardW, LEADERBOARD_PANEL_H, panelR, panelG, panelB, panelAlpha);

    // Draw "LEADERBOARD" header - opaque text
    FontManager::drawTextTopWithFont("LEADERBOARD", leaderboardX + 5, leaderboardY + 2, 255, 255, 255, fontFace, FontStyle::Bold, headerFontSize);

    // Draw leaderboard entries with OPAQUE text
    for (int i = 0; i < (int)scores.size() && i < 5; i++) {
        snprintf(maxEntryText, sizeof(maxEntryText), "#%d %s: %llu", (int)scores[i].rank, scores[i].username.c_str(), (unsigned long long)scores[i].score);
        FontManager::drawTextTopWithFont(maxEntryText, leaderboardX + 5, leaderboardY + 18 + i * 14, 255, 255, 255, fontFace, FontStyle::Regular, bodyFontSize);
    }
}

void UInGame::processInGameInput(CameraTrackpad& trackpad, InGameUIState& state) {
    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    // === PAUSE TOGGLE (START button) ===
    if (kDown & KEY_START) {
        g_isPaused = !g_isPaused;
        state = g_isPaused ? InGameUIState::Paused : InGameUIState::Playing;
        return;
    }

    if (g_isPaused) {
        if (kDown & KEY_X) {
            printf("Settings triggered\n");
        }
        if (kDown & KEY_B) {
            printf("Leave game triggered\n");
        }
        return;
    }

    // === VOICE CHAT PUSH-TO-TALK (Touch VC button) ===
    {
        touchPosition touch;
        hidTouchRead(&touch);
        // VC button area: (145, 5) to (170, 23)
        bool vcTouched = kHeld & KEY_TOUCH &&
            touch.px >= 145 && touch.px < 170 &&
            touch.py >= 5 && touch.py < 23;
        VoiceChat::setTalking(vcTouched);
    }

    // === CAMERA TRACKPAD (Touch) ===
    touchPosition touch;
    bool inTrackpad = false;

    // === VOICE CHAT PUSH-TO-TALK (Touch VC button) ===
    if (kDown & KEY_TOUCH) {
        hidTouchRead(&touch);
        if (touch.px >= 145 && touch.px <= 170 && touch.py >= TOP_BTN_Y && touch.py <= TOP_BTN_Y + TOP_BTN_H) {
            // Touch down on VC button - start talking
            VoiceChat::setTalking(true);
            VoiceChat::startCapture();
        }
    }
    if (!(kHeld & KEY_TOUCH)) {
        // Touch released anywhere - stop talking if was talking
        if (VoiceChat::getState() == VoiceChat::VcState::Talking) {
            VoiceChat::setTalking(false);
            VoiceChat::stopCapture();
        }
    }

    if (kHeld & KEY_TOUCH) {
        touchRead(&touch);

        if (touch.px >= TRACKPAD_X && touch.px < TRACKPAD_X + TRACKPAD_W &&
            touch.py >= TRACKPAD_Y && touch.py < TRACKPAD_Y + TRACKPAD_H) {

            if (!g_touchHeld) {
                g_touchDown = true;
                trackpad.startX = touch.px;
                trackpad.startY = touch.py;
                trackpad.active = true;
            }

            g_touchHeld = true;
            inTrackpad = true;

            int deltaX = touch.px - TRACKPAD_CENTER_X;
            int deltaY = touch.py - TRACKPAD_CENTER_Y;

            float maxX = TRACKPAD_W / 2.0f;
            float maxY = TRACKPAD_H / 2.0f;

            trackpad.deltaX = deltaX / maxX;
            trackpad.deltaY = deltaY / maxY;

            if (trackpad.deltaX > 1.0f) trackpad.deltaX = 1.0f;
            if (trackpad.deltaX < -1.0f) trackpad.deltaX = -1.0f;
            if (trackpad.deltaY > 1.0f) trackpad.deltaY = 1.0f;
            if (trackpad.deltaY < -1.0f) trackpad.deltaY = -1.0f;
        }
    }

    if (!inTrackpad) {
        g_touchDown = false;
        g_touchHeld = false;
        trackpad.active = false;
        trackpad.deltaX = 0.0f;
        trackpad.deltaY = 0.0f;
    }

    // === ZOOM (Touch slider only) ===
    // Zooming is ONLY controlled by the zoom scrollbar in the bottom screen UI.
    // No ZL/ZR, no D-pad, no pinch-to-zoom.
    if (kHeld & KEY_TOUCH) {
        touchRead(&touch);
        // Check if touching slider area (right side)
        if (touch.px >= SLIDER_X - 5 && touch.px <= SLIDER_X + SLIDER_W + 10) {
            float clickNorm = (float)(touch.py - SLIDER_Y) / SLIDER_H;
            if (clickNorm < 0) clickNorm = 0;
            if (clickNorm > 1) clickNorm = 1;
            float newDist = CameraSystem::getMaxDistance() - clickNorm * (CameraSystem::getMaxDistance() - CameraSystem::getMinDistance());
            CameraSystem::setDistance(newDist);
            g_sliderDragging = true;
        }
    }
    if (!(kHeld & KEY_TOUCH)) {
        g_sliderDragging = false;
    }

    // === CHAT TOGGLE (Y button) ===
    if (kDown & KEY_Y) {
        g_chatOpen = !g_chatOpen;
        if (g_chatOpen) ChatSystem::openChat();
        else ChatSystem::closeChat();
    }

    // === BACKPACK TOGGLE (B button) ===
    if (kDown & KEY_B) {
        toggleBackpack();
        state = g_backpackOpen ? InGameUIState::BackpackOpen : InGameUIState::Playing;
    }

    // === GEAR ACTIVATION (L = Left Click, R = Right Click) ===
    // L button triggers primary gear activation (Tool.Activated)
    if (kDown & KEY_L) {
        ClassicTools::onAction();
        // TODO: GearManager::activateTool(); // GearManager not yet implemented
    }
    // R button triggers secondary gear activation (Tool.SecondaryActivated)
    if (kDown & KEY_R) {
        ClassicTools::onActionSecondary();
        // TODO: GearManager::activateToolSecondary(); // GearManager not yet implemented
    }

    // === HOTBAR EQUIP (A button) ===
    if (kDown & KEY_A) {
        equipHotbarSlot(g_selectedSlot);
    }

    // === CURSOR TRACKPAD (Touch right side of bottom screen) ===
    // Right half of bottom screen acts as cursor trackpad
    if (kHeld & KEY_TOUCH) {
        hidTouchRead(&touch);
        if (touch.px >= 160) { // Right half of bottom screen (160-319)
            // Calculate delta from trackpad center
            int trackpadCenterX = 240; // Center of right half
            int trackpadCenterY = 80;
            int deltaX = (touch.px - trackpadCenterX) / 4; // Scale down sensitivity
            int deltaY = (touch.py - trackpadCenterY) / 4;
            CursorSystem::trackpadUpdate(deltaX, deltaY);
            
            // L = Left click, R = Right click
            CursorSystem::setLeftClick(kHeld & KEY_L);
            CursorSystem::setRightClick(kHeld & KEY_R);
        }
    }
}

int UInGame::getSelectedHotbarSlot() {
    return g_selectedSlot;
}

void UInGame::equipHotbarSlot(int index) {
    if (index < 0 || index >= HOTBAR_SLOT_COUNT) return;

    for (auto& slot : g_hotbar) {
        slot.equipped = false;
    }

    if (g_hotbar[index].itemId != -1) {
        g_hotbar[index].equipped = true;
        g_selectedSlot = index;
    }
}

bool UInGame::isPaused() {
    return g_isPaused;
}

bool UInGame::isBackpackOpen() {
    return g_backpackOpen;
}

void UInGame::toggleBackpack() {
    g_backpackOpen = !g_backpackOpen;
}

void UInGame::shutdown() {
    g_hotbar.clear();
    g_isPaused = false;
    g_backpackOpen = false;
    g_chatOpen = false;
}