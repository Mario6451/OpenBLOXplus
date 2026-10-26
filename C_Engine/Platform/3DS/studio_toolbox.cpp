#include "studio_toolbox.h"
#include "settings.h"
#include "asset_loader.h"
#include "../CoreEngine/Node/nodes.h"
#include "ui_studio.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

namespace StudioToolbox {

    // UI Layout constants (fits 320x240 bottom screen)
    static const int TOOLBOX_X = 5;
    static const int TOOLBOX_Y = 5;
    static const int TOOLBOX_W = 310;
    static const int TOOLBOX_H = 230;

    static const int TAB_H = 16;
    static const int SEARCH_H = 18;
    static const int SCROLLBAR_W = 12;

    // State
    static bool visible = false;
    static ToolboxCategory currentCategory = ToolboxCategory::Parts;
    static std::vector<ToolboxAsset> assets;
    static int selectedIdx = 0;
    static int scrollOffset = 0;
    static char searchQuery[64];
    static std::string proxyUrl;
    static PrintConsole toolboxConsole;
    static bool consoleInitialized = false;

    static const char* categoryLabels[] = {
        "Parts", "Models", "Audio", "Decals", "Scripts"
    };

    // Helper to fetch proxy URL from settings
    static void updateProxyUrl() {
        proxyUrl = Settings::get().general.toolboxProxy;
        if (proxyUrl.empty()) {
            proxyUrl = "http://127.0.0.1"; // Local fallback
        }
    }

    // Helper to generate mock assets (actual implementation would fetch from proxy via HTTP)
    static void fetchAssetsFromProxy() {
        assets.clear();

        // In production, this would make an HTTP GET request to:
        // proxyUrl + "/v1/search?q=" + searchQuery + "&type=" + categoryToString(currentCategory)
        // For now, generate placeholders that can be expanded when actual HTTP client is integrated
        const char* mockNames[] = {
            "Block", "Cylinder", "Wedge", "Corner Wedge", "Sphere",
            "Plate 4x4", "Tile 2x2", "Brick 1x2", "Glass 1x4", "Neon Tube",
            "Motor", "Hinge", "Spring", "Pulley", "Winch",
            "Platform", "Ladder", "Ramp", "Stairs", "Elevator"
        };

        int count = sizeof(mockNames) / sizeof(mockNames[0]);
        if (strlen(searchQuery) > 0) {
            // Filter by search query
            for (int i = 0; i < count; i++) {
                if (strstr(mockNames[i], searchQuery) || strlen(searchQuery) == 0) {
                    ToolboxAsset asset;
                    asset.id = "proxy_" + std::to_string(i);
                    asset.name = mockNames[i];
                    asset.category = currentCategory;
                    asset.isSelected = false;
                    assets.push_back(asset);
                }
            }
        } else {
            for (int i = 0; i < count; i++) {
                ToolboxAsset asset;
                asset.id = "proxy_" + std::to_string(i);
                asset.name = mockNames[i];
                asset.category = currentCategory;
                asset.isSelected = false;
                assets.push_back(asset);
            }
        }

        // Clamp selection
        if (selectedIdx >= (int)assets.size()) selectedIdx = std::max(0, (int)assets.size() - 1);
    }

    void init() {
        visible = false;
        currentCategory = ToolboxCategory::Parts;
        selectedIdx = 0;
        scrollOffset = 0;
        memset(searchQuery, 0, sizeof(searchQuery));
        updateProxyUrl();
        fetchAssetsFromProxy();
    }

    void drawToolbox() {
        if (!visible) return;

        if (!consoleInitialized) {
            consoleInit(GFX_BOTTOM, &toolboxConsole);
            consoleInitialized = true;
        }
        consoleSelect(&toolboxConsole);

        // Background panel
        iprintf("\x1b[1;1H\x1b[44m%s\x1b[0m", ""); // Clear line
        for (int y = 1; y <= 28; y++) {
            char buf[256];
            snprintf(buf, sizeof(buf), "\x1b[%d;1H\x1b[44m%-39s\x1b[0m", y, "");
            iprintf("%s", buf);
        }

        // Title bar
        iprintf("\x1b[%d;%dH\x1b[107m TOOLBOX (Proxy: %s)\x1b[0m", 1, 1,
                proxyUrl.substr(0, std::min(20, (int)proxyUrl.size())).c_str());

        // Category tabs row
        int tabY = 3;
        int tabX = 2;
        for (int i = 0; i < (int)ToolboxCategory::Count; i++) {
            int tabW = (int)strlen(categoryLabels[i]) + 2;
            if (i == (int)currentCategory) {
                iprintf("\x1b[%d;%dH\x1b[103m %s \x1b[0m", tabY, tabX, categoryLabels[i]);
            } else {
                iprintf("\x1b[%d;%dH\x1b[37m %s \x1b[0m", tabY, tabX, categoryLabels[i]);
            }
            tabX += tabW + 1;
        }

        // Search bar
        int searchY = 5;
        iprintf("\x1b[%d;%dH\x1b[47m Search: %.*s \x1b[0m", searchY, 2,
                (int)strlen(searchQuery), searchQuery);

        // Scrollable asset list
        int listY = 7;
        int listH = 18;
        int listX = 2;

        for (int i = scrollOffset; i < (int)assets.size() && i < scrollOffset + listH; i++) {
            const char* sel = (i == selectedIdx) ? ">>" : "  ";
            iprintf("\x1b[%d;%dH%s %s", listY + (i - scrollOffset), listX, sel, assets[i].name.c_str());
        }

        // Scrollbar
        int scrollX = TOOLBOX_W - 2;
        if ((int)assets.size() > listH) {
            int thumbH = listH * listH / (int)assets.size();
            int thumbY = listY + (scrollOffset * (listH - thumbH) / (int)assets.size());
            for (int y = listY; y < listY + listH; y++) {
                if (y >= thumbY && y < thumbY + thumbH) {
                    iprintf("\x1b[%d;%dH\x1b[43m  \x1b[0m", y, scrollX);
                }
            }
        }

        // Buttons at bottom
        iprintf("\x1b[26;%dH\x1b[102m  A:Insert  \x1b[0m", 2);
        iprintf("\x1b[26;%dH\x1b[101m  B:Close   \x1b[0m", 16);
    }

    void processInput() {
        if (!visible) return;

        hidScanInput();
        u32 kDown = hidKeysDown();
        touchPosition touch;
        touchRead(&touch);

        // Navigation
        if (kDown & KEY_UP) {
            selectedIdx = std::max(0, selectedIdx - 1);
            if (selectedIdx < scrollOffset) scrollOffset = selectedIdx;
        }
        if (kDown & KEY_DOWN) {
            selectedIdx = std::min((int)assets.size() - 1, selectedIdx + 1);
            if (selectedIdx >= scrollOffset + 18) scrollOffset = selectedIdx - 17;
        }

        // Category switching (D-pad left/right)
        if (kDown & KEY_LEFT) {
            currentCategory = (ToolboxCategory)((int)currentCategory - 1 < 0 ? (int)ToolboxCategory::Count - 1 : (int)currentCategory - 1);
            fetchAssetsFromProxy();
        }
        if (kDown & KEY_RIGHT) {
            currentCategory = (ToolboxCategory)((int)currentCategory + 1 >= (int)ToolboxCategory::Count ? 0 : (int)currentCategory + 1);
            fetchAssetsFromProxy();
        }

        // Insert asset (A button)
        if (kDown & KEY_A && selectedIdx >= 0 && selectedIdx < (int)assets.size()) {
            assets[selectedIdx].isSelected = true;
        }

        // Close (B button)
        if (kDown & KEY_B) {
            visible = false;
        }

        // Touch input for asset selection
        if (kDown & KEY_TOUCH) {
            int tx = touch.px;
            int ty = touch.py;

            // Tab touch detection
            if (ty >= TOOLBOX_Y + 20 && ty <= TOOLBOX_Y + 36) {
                int tabX = 2;
                for (int i = 0; i < (int)ToolboxCategory::Count; i++) {
                    int tabW = (int)strlen(categoryLabels[i]) + 2;
                    if (tx >= tabX * 8 && tx <= (tabX + tabW) * 8) {
                        currentCategory = (ToolboxCategory)i;
                        fetchAssetsFromProxy();
                        break;
                    }
                    tabX += tabW + 1;
                }
            }

            // List touch
            if (ty >= TOOLBOX_Y + 56 && ty <= TOOLBOX_Y + 200 && tx <= TOOLBOX_W - 20) {
                int idx = scrollOffset + ((ty - TOOLBOX_Y - 56) / 8);
                if (idx >= 0 && idx < (int)assets.size()) {
                    selectedIdx = idx;
                }
            }
        }
    }

    const ToolboxAsset* getSelectedAsset() {
        if (selectedIdx >= 0 && selectedIdx < (int)assets.size()) {
            return &assets[selectedIdx];
        }
        return nullptr;
    }

    void insertSelectedAsset(void* dataModel) {
        if (!dataModel || selectedIdx < 0 || selectedIdx >= (int)assets.size()) return;

        ToolboxAsset& asset = assets[selectedIdx];
        if (!asset.isSelected) return;

        // Create the asset in the workspace
        switch (asset.category) {
            case ToolboxCategory::Parts: {
                Part* part = StudioUI::createPart(asset.name.c_str(), 0, 5, 0);
                part->SetParent((Node*)dataModel);
                break;
            }
            case ToolboxCategory::Models:
                // Fetch model from proxy URL: proxyUrl + "/v1/model/" + asset.id
                // For now, create a placeholder folder/model node
                break;
            case ToolboxCategory::Audio:
                // Create Sound node
                break;
            case ToolboxCategory::Decals:
                // Create Decal/Texture node
                break;
            case ToolboxCategory::Scripts:
                // Create Script node
                break;
            default:
                break;
        }

        asset.isSelected = false; // Reset selection flag
    }

    bool isToolboxVisible() {
        return visible;
    }

    void toggleToolbox() {
        visible = !visible;
        if (visible) {
            updateProxyUrl();
            fetchAssetsFromProxy();
        }
    }

    void shutdown() {
        assets.clear();
        visible = false;
    }

} // namespace StudioToolbox