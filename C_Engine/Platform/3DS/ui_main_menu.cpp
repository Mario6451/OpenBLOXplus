// ui_main_menu.cpp - OpenBLOX 3DS Main Menu
// Layout: Left column (buttons), Center (game list), Right (scrollbar)
// Resolution: 320x240 (bottom screen)

#include "ui_main_menu.h"
#include "citro_renderer.h"
#include "CoreEngine/icon_loader.h"
#include "voice_chat.h"
#include "font_manager.h"
#include "studio_client.h"
#include "ui_multiplayer.h"
#include "audio_manager.h"
#include <3ds.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <algorithm>
#include <cstring>
#include <cctype>

// Static state for the main menu
static const int LEFT_COL_WIDTH = 60;
static const int RIGHT_COL_WIDTH = 12;
static const int GAME_LIST_TOP = 28;
static const int GAME_LIST_BOTTOM = 236;
static const int LINE_HEIGHT = 16;

static UIMainMenu::MenuState currentState = UIMainMenu::MenuState::Main;
static std::vector<GameEntry> gameEntries;
static std::vector<GameEntry> filteredEntries;
static int selectedIndex = 0;
static int scrollOffset = 0;
static std::string currentPath = "sdmc:/OpenBLOX/games/";
static std::vector<std::string> pathHistory;
static std::string searchText;
static bool isSearchActive = false;
// On-screen keyboard
static const char s_keyboard[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .-_";
static int s_kbPage = 0;

using namespace UIMainMenu;

void UIMainMenu::init() {
    currentState = UIMainMenu::MenuState::Main;
    selectedIndex = 0;
    scrollOffset = 0;
    currentPath = "sdmc:/OpenBLOX/games/";
    pathHistory.clear();
    searchText.clear();
    isSearchActive = false;
    s_kbPage = 0;
    gameEntries = scanDirectory(currentPath);
    filteredEntries.clear();
    AudioManager::initAudio();
}

std::vector<GameEntry> UIMainMenu::scanDirectory(const std::string& path) {
    std::vector<GameEntry> entries;
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        printf("Warning: Could not open directory %s\n", path.c_str());
        return entries;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        GameEntry gameEntry;
        gameEntry.displayName = entry->d_name;
        gameEntry.path = path + "/" + entry->d_name;
        gameEntry.isFolder = (entry->d_type == DT_DIR);
        entries.push_back(gameEntry);
    }
    closedir(dir);
    std::sort(entries.begin(), entries.end(), [](const GameEntry& a, const GameEntry& b) {
        if (a.isFolder != b.isFolder) return a.isFolder;
        return a.displayName < b.displayName;
    });
    return entries;
}

std::vector<GameEntry> UIMainMenu::scanDirectoryRecursive(const std::string& path) {
    std::vector<GameEntry> entries;
    DIR* dir = opendir(path.c_str());
    if (!dir) return entries;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        
        GameEntry gameEntry;
        gameEntry.displayName = entry->d_name;
        gameEntry.path = path + "/" + entry->d_name;
        gameEntry.isFolder = (entry->d_type == DT_DIR);
        
        if (gameEntry.isFolder) {
            // Recursively scan subdirectories
            auto subEntries = scanDirectoryRecursive(gameEntry.path);
            entries.push_back(gameEntry);
            entries.insert(entries.end(), subEntries.begin(), subEntries.end());
        } else {
            // Only include .rbxl, .rbxlx, .rbxm, .rbxmx files
            if (gameEntry.displayName.find(".rbxl") != std::string::npos ||
                gameEntry.displayName.find(".rbxm") != std::string::npos) {
                entries.push_back(gameEntry);
            }
        }
    }
    closedir(dir);
    return entries;
}

std::vector<GameEntry> UIMainMenu::filterBySearch(const std::vector<GameEntry>& entries, const std::string& search) {
    if (search.empty()) return entries;
    
    std::vector<GameEntry> filtered;
    std::string searchLower = search;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
    
    for (const auto& entry : entries) {
        std::string nameLower = entry.displayName;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        if (nameLower.find(searchLower) != std::string::npos) {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

void UIMainMenu::drawMainMenu() {
    CitroRenderer::startFrame();
    CitroRenderer::setColor(0.0f, 0.0f, 0.0f);
    CitroRenderer::beginTriangles();
    CitroRenderer::vertex3f(-0.9f, -0.9f, 0.0f);
    CitroRenderer::vertex3f(0.9f, -0.9f, 0.0f);
    CitroRenderer::vertex3f(0.9f, 0.9f, 0.0f);
    CitroRenderer::vertex3f(-0.9f, -0.9f, 0.0f);
    CitroRenderer::vertex3f(0.9f, 0.9f, 0.0f);
    CitroRenderer::vertex3f(-0.9f, 0.9f, 0.0f);
    CitroRenderer::endPrimitives();
    FontManager::drawText("INSERT GAME HERE", 80, 120, 255, 255, 255, FontSize::Size20);
    CitroRenderer::endFrame();
}

void UIMainMenu::drawGameList(const std::vector<GameEntry>& entries) {
    consoleClear();
    consoleSelect(NULL);
    int visibleCount = (GAME_LIST_BOTTOM - GAME_LIST_TOP) / LINE_HEIGHT;
    int total = (int)entries.size();

    // Background (batched rects instead of 76,800 DrawPixel calls)
    DrawRect(0, 0, 320, 240, 20, 20, 25);
    DrawRect(0, 0, LEFT_COL_WIDTH, 240, 15, 15, 20);
    DrawRect(320 - RIGHT_COL_WIDTH, 0, RIGHT_COL_WIDTH, 240, 25, 25, 30);
    DrawRect(LEFT_COL_WIDTH, 0, 320 - LEFT_COL_WIDTH - RIGHT_COL_WIDTH, 24, isSearchActive ? 40 : 25, 40, 50);
    FontManager::drawText("rbxl search in folder:", LEFT_COL_WIDTH + 4, 8,
                          isSearchActive ? 100 : 150, 150, 255, FontSize::Size8);
    if (!searchText.empty())
        FontManager::drawText(searchText.c_str(), LEFT_COL_WIDTH + 100, 8, 255, 255, 255, FontSize::Size10);

    // Scrollbar thumb
    if (total > visibleCount) {
        int thumbH = std::max(20, (visibleCount * visibleCount) / total);
        int thumbY = GAME_LIST_TOP + ((scrollOffset * (GAME_LIST_BOTTOM - GAME_LIST_TOP - thumbH))) / std::max(1, total - visibleCount);
        DrawRect(320 - RIGHT_COL_WIDTH + 2, thumbY, RIGHT_COL_WIDTH - 4, thumbH, 100, 150, 255);
    }

    // Settings button with icon
    IconLoader::drawIconScaled(IconLoader::getIcon(IconLoader::ICON_SETTINGS), 8, 4, 28, 28, 150);
    FontManager::drawText("Settings", 12, 34, 200, 200, 200, FontSize::Size8);

    // Multiplayer button with icon
    IconLoader::drawIconScaled(IconLoader::getIcon(IconLoader::ICON_MULTIPLAYER), 8, 40, 28, 28, 100);
    FontManager::drawText("Multiplayer", 8, 70, 200, 200, 200, FontSize::Size8);

    // Avatar Editor button
    IconLoader::drawIconScaled(IconLoader::getIcon(IconLoader::ICON_AVATAR_EDITOR), 8, 76, 28, 28, 200);
    FontManager::drawText("Avatar", 16, 106, 200, 200, 200, FontSize::Size8);

    // Studio button
    IconLoader::drawIconScaled(IconLoader::getIcon(IconLoader::ICON_STUDIO), 8, 112, 28, 28, 100);
    FontManager::drawText("Studio", 16, 142, 200, 200, 200, FontSize::Size8);

    // Quit button (bottom)
    DrawRect(4, 200, LEFT_COL_WIDTH - 8, 36, 180, 50, 50);
    FontManager::drawText("Quit", 20, 214, 255, 255, 255, FontSize::Size12);

    // Game list
    for (int i = 0; i < visibleCount && (scrollOffset + i) < total; i++) {
        int idx = scrollOffset + i;
        const GameEntry& e = entries[idx];
        int y = GAME_LIST_TOP + i * LINE_HEIGHT;
        bool sel = (idx == selectedIndex);
        DrawRect(LEFT_COL_WIDTH, y, 320 - LEFT_COL_WIDTH - RIGHT_COL_WIDTH, LINE_HEIGHT, sel ? 60 : 25, sel ? 80 : 25, sel ? 120 : 35);
        const char* pf = e.isFolder ? "[Folder] " : "";
        FontManager::drawText(pf, LEFT_COL_WIDTH + 4, y + 4, sel ? 255 : 200, sel ? 255 : 200, sel ? 255 : 200, FontSize::Size10);
        FontManager::drawText(e.displayName.c_str(), LEFT_COL_WIDTH + 4 + (int)(strlen(pf) * 5), y + 4, sel ? 255 : 200, sel ? 255 : 200, sel ? 255 : 200, FontSize::Size10);
    }

    // On-screen keyboard
    if (isSearchActive) {
        int kbY = GAME_LIST_BOTTOM;
        DrawRect(0, kbY, 320, 40, 30, 30, 40);
        // OK button
        DrawRect(4, kbY, 28, 10, 60, 180, 60);
        FontManager::drawText("OK", 8, kbY + 1, 255, 255, 255, FontSize::Size8);
        // CLR button
        DrawRect(34, kbY, 28, 10, 180, 60, 60);
        FontManager::drawText("CLR", 38, kbY + 1, 255, 255, 255, FontSize::Size8);
        // Chars
        int len = (int)strlen(s_keyboard);
        int cps = 10; // chars per row
        int start = s_kbPage * (cps * 3);
        int end = std::min(start + cps * 3, len);
        for (int i = start; i < end; i++) {
            int r = (i - start) / cps;
            int c = (i - start) % cps;
            FontManager::drawText(std::string(1, s_keyboard[i]).c_str(),
                                  4 + c * 28, kbY + 14 + r * 10, 255, 255, 255, FontSize::Size8);
        }
        // Page nav
        if (s_kbPage > 0) FontManager::drawText("<", 4, kbY + 46, 150, 150, 255, FontSize::Size10);
        if (end < len) FontManager::drawText(">", 310, kbY + 46, 150, 150, 255, FontSize::Size10);
    }
}

void UIMainMenu::drawMainUI() {
    drawGameList(gameEntries);
}

std::string UIMainMenu::runMenu() {
    init();
    bool running = true;
    while (running) {
        drawMainMenu();
        drawGameList(gameEntries);
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        int total = (int)gameEntries.size();
        int visibleCount = (GAME_LIST_BOTTOM - GAME_LIST_TOP) / LINE_HEIGHT;

        touchPosition touch;
        hidTouchRead(&touch);

        if (isSearchActive) {
            // Handle keyboard input
            if (kDown & KEY_TOUCH) {
                int kbY = GAME_LIST_BOTTOM;
                if (touch.py >= kbY && touch.py < kbY + 10) {
                    if (touch.px >= 4 && touch.px < 32) {
                        // OK
                        isSearchActive = false; s_kbPage = 0;
                        filteredEntries = filterBySearch(scanDirectoryRecursive(currentPath), searchText);
                        gameEntries = filteredEntries;
                        selectedIndex = 0; scrollOffset = 0;
                    } else if (touch.px >= 34 && touch.px < 62) {
                        // CLR
                        searchText.clear();
                    }
                } else if (touch.py >= kbY + 14 && touch.py < kbY + 46) {
                    int len = (int)strlen(s_keyboard);
                    int cps = 10;
                    int start = s_kbPage * (cps * 3);
                    int end = std::min(start + cps * 3, len);
                    int c = (touch.px - 4) / 28;
                    int r = (touch.py - kbY - 14) / 10;
                    int idx2 = start + r * cps + c;
                    if (idx2 >= start && idx2 < end && searchText.length() < 30)
                        searchText += s_keyboard[idx2];
                }
                if (touch.py >= kbY + 46) {
                    int len2 = (int)strlen(s_keyboard);
                    if (touch.px < 20 && s_kbPage > 0) s_kbPage--;
                    if (touch.px > 300 && s_kbPage < len2 / 30) s_kbPage++;
                }
            }
            if (kDown & KEY_B && !searchText.empty()) searchText.pop_back();
            if (kDown & KEY_A) {
                isSearchActive = false; s_kbPage = 0;
                filteredEntries = filterBySearch(scanDirectoryRecursive(currentPath), searchText);
                gameEntries = filteredEntries;
                selectedIndex = 0; scrollOffset = 0;
            }
        } else if (total > 0) {
            if (kDown & KEY_UP) { selectedIndex--; if (selectedIndex < 0) selectedIndex = total - 1; if (selectedIndex < scrollOffset) scrollOffset = selectedIndex; }
            if (kDown & KEY_DOWN) { selectedIndex++; if (selectedIndex >= total) selectedIndex = 0; if (selectedIndex >= scrollOffset + visibleCount) scrollOffset = selectedIndex - visibleCount + 1; }
            if (kDown & KEY_A) {
                const GameEntry& e = gameEntries[selectedIndex];
                if (e.isFolder) {
                    pathHistory.push_back(currentPath);
                    currentPath = e.path;
                    gameEntries = scanDirectory(currentPath);
                    selectedIndex = 0; scrollOffset = 0;
                } else {
                    const char* n = e.displayName.c_str();
                    if (strstr(n, ".rbxl") || strstr(n, ".rbxlx")) return e.path;
                }
            }
            if (kDown & KEY_B && !pathHistory.empty()) {
                currentPath = pathHistory.back(); pathHistory.pop_back();
                gameEntries = scanDirectory(currentPath);
                selectedIndex = 0; scrollOffset = 0;
            }
            // Touch search bar
            if (kDown & KEY_TOUCH && touch.py >= 0 && touch.py < 24 &&
                touch.px >= LEFT_COL_WIDTH && touch.px < 320 - RIGHT_COL_WIDTH) {
                isSearchActive = true; s_kbPage = 0;
            }
        }
        if (kDown & KEY_X) return "__SETTINGS__";
        if (kDown & KEY_Y) return "__MULTIPLAYER__";
        if (kDown & KEY_START) return "";
        if (!isSearchActive) {
            if (kHeld & KEY_TOUCH) {
                touchPosition tp; hidTouchRead(&tp);
                if (tp.px >= LEFT_COL_WIDTH && tp.px < 320 - RIGHT_COL_WIDTH) {
                    int line = (tp.py - GAME_LIST_TOP) / LINE_HEIGHT;
                    if (line >= 0 && line < visibleCount && (scrollOffset + line) < total)
                        selectedIndex = scrollOffset + line;
                }
            }
            if (kDown & KEY_TOUCH) {
                touchPosition tp; hidTouchRead(&tp);
                if (tp.px < LEFT_COL_WIDTH) {
                    if (tp.py >= 4 && tp.py < 44) return "__SETTINGS__";
                    if (tp.py >= 40 && tp.py < 80) return "__MULTIPLAYER__";
                    if (tp.py >= 76 && tp.py < 112) return "__AVATAR__";
                    if (tp.py >= 112 && tp.py < 148) return "__STUDIO__";
                    if (tp.py >= 200 && tp.py < 236) return "";
                }
            }
        }
        if (kHeld & KEY_L) scrollOffset = std::max(0, scrollOffset - 1);
        if (kHeld & KEY_R) scrollOffset = std::min(std::max(0, total - visibleCount), scrollOffset + 1);
        gspWaitForVBlank();
    }
    return "";
}

void UIMainMenu::runStudio(const std::string& placePath) {
    StudioClient::init();
    StudioClient::runStudio();
    StudioClient::shutdown();
}

void UIMainMenu::runMultiplayerMenu() {
    // TODO: Multiplayer menu removed - implement later
    // UIMultiplayer multiplayer;
    // multiplayer.run();
}