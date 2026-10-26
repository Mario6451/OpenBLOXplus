// Studio client implementation for OpenBLOX
// Bottom screen: Left Toolbar + Top-Right Explorer + Bottom-Right Properties
// Top screen: 3D workspace preview

#include "ui_studio.h"
#include "toolbox.h"
#include "studio_toolbox.h"
#include "citro_renderer.h"
#include "font_manager.h"
#include "settings.h"
#include "asset_loader.h"
#include "../CoreEngine/Node/nodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace StudioUI {

// Helper forward declarations (stubs for link-only functions)
static void collectExplorerItems(Node*, std::vector<ExplorerItem>&, int);
static void selectByExplorerTouch(Node*, touchPosition);
static void serializeNodeToXML(FILE*, Node*, int) {}
static Node* parseRBXL(FILE*) { return nullptr; }
static int osCreateFlex(int, int) { return 0; }
static int osInitializeFlex(int) { return 0; }
static int osShowFlex(int) { return 0; }
static int osGetFlexInput(char*, size_t, const char*, int) { return -1; }

// Stub OS_FLEX constants
#define OS_FLEXPOS_BOTTOM 0
#define OS_FLEXSIZE_FULL 0

static PrintConsole topConsole;
static PrintConsole bottomConsole;
static bool RemoveFromVector(std::vector<Node*>& vec, Node* node) {
    auto it = std::remove(vec.begin(), vec.end(), node);
    if (it != vec.end()) vec.erase(it, vec.end());
    return it != vec.end();
}

// UI layout constants (320x240 bottom screen)
static const int TOOLBAR_X = 5;
static const int TOOLBAR_Y = 5;
static const int TOOLBAR_W = 40;
static const int TOOLBAR_BTN_H = 26;
static const int EXPLORER_X = TOOLBAR_X + TOOLBAR_W + 5;
static const int EXPLORER_Y = TOOLBAR_Y;
static const int EXPLORER_W = 260;
static const int EXPLORER_H = 110;
static const int SCRIPT_EDITOR_Y = EXPLORER_Y;
static const int SCRIPT_EDITOR_X = EXPLORER_X;

static const int PROPERTIES_X = TOOLBAR_X + TOOLBAR_W + 5;
static const int PROPERTIES_Y = EXPLORER_Y + EXPLORER_H + 10;
static const int PROPERTIES_W = 260;
static const int PROPERTIES_H = 115;

// State
static StudioMode currentMode = StudioMode::Editing;
static Node* selectedNode = nullptr;
static Node* workspaceNode = nullptr;
static int explorerScroll = 0;
static int propertiesScroll = 0;
static bool running = true;
static std::string lastSavedPath;
static char scriptBuffer[4096]; // Simple script editor buffer

// Toolbox state
static std::vector<ToolboxAsset> toolboxAssets;
static std::string toolboxSearchQuery;
static int toolboxScroll = 0;
static int toolboxSelectedIndex = 0;
static ToolboxCategory toolboxCategory = ToolboxCategory::Models;
static char toolboxSearchInput[64];
static bool toolboxSearchPending = false;
static bool toolboxSearchResultsReady = false;
static int toolboxCursorPos = 0;
static int toolboxPage = 1;
static int toolboxGenreDropdown = 0;
static const char* toolboxGenres[] = { "All", "Buildings", "Vehicles", "Nature", "Decor", "Props", "Audio" };

void init() {
    currentMode = StudioMode::Editing;
    selectedNode = nullptr;
    explorerScroll = 0;
    propertiesScroll = 0;
    memset(scriptBuffer, 0, sizeof(scriptBuffer));
    // Initialize toolbox state
    memset(toolboxSearchInput, 0, sizeof(toolboxSearchInput));
    toolboxSearchQuery.clear();
    toolboxAssets.clear();
    toolboxScroll = 0;
    toolboxSelectedIndex = 0;
    toolboxSearchResultsReady = false;
    toolboxPage = 1;
    toolboxGenreDropdown = 0;
}

void runStudio(Node* dataModel) {
    workspaceNode = dataModel;
    init();

    while (running) {
        // Draw top screen - 3D workspace
        drawWorkspace(workspaceNode);

        // Draw bottom screen UI
        drawBottomUI(workspaceNode);

        // Process input
        processInput(workspaceNode);

        // Wait for next frame
        svcSleepThread(10000000ULL);
    }
}

void drawWorkspace(Node* dataModel) {
    if (!dataModel) return;

    // Clear top screen
    CitroRenderer::clearScreen(0.15f, 0.15f, 0.2f, 1.0f);
    CitroRenderer::startFrame();

    // Draw workspace
    renderWorld(dataModel);

    // Draw selection highlight
    if (selectedNode && selectedNode->type) {
        if (!strcmp(selectedNode->type, "Part") || !strcmp(selectedNode->type, "BasePart")) {
            Part* part = static_cast<Part*>(selectedNode);
            // Draw wireframe selection box
            CitroRenderer::setColor(1.0f, 1.0f, 0.0f);
            CitroRenderer::drawBox(part->cf.X, part->cf.Y, part->cf.Z,
                                  part->scale.x, part->scale.y, part->scale.z,
                                  1.0f, 1.0f, 0.0f);
        }
    }

    // Draw "STUDIO MODE" label
    CitroRenderer::endFrame();

    // Draw overlay text
    iprintf("\x1b[1;1H\x1b[101m  OPENBLOX STUDIO  \x1b[0m");

    if (currentMode == StudioMode::PlayTesting) {
        iprintf("\x1b[21;1H\x1b[103m  PLAY MODE  \x1b[0m");
    }
}

void drawBottomUI(Node* dataModel) {
    consoleClear();

    // Draw toolbar
    drawToolbar();

    // Draw mode-specific panels
    switch (currentMode) {
        case StudioMode::Editing:
            drawExplorer(dataModel, explorerScroll);
            drawProperties(selectedNode, propertiesScroll);
            break;
        case StudioMode::ScriptEditor:
            drawScriptEditor();
            break;
        case StudioMode::PlayTesting:
            iprintf("\x1b[10;10HPlay mode active...\x1b[0m");
            iprintf("\x1b[12;5HPress B to stop\x1b[0m");
            break;
        case StudioMode::Toolbox:
            drawToolbox();
            break;
    }
}

void drawToolbar() {
    // Draw left toolbar (Exit, Save, Load, Test, Toolbox, Import, Add, Export)

    // Draw toolbar background
    for (int y = TOOLBAR_Y; y < TOOLBAR_Y + TOOLBAR_BTN_H * 8 + 5; y += 2) {
        for (int x = TOOLBAR_X; x < TOOLBAR_X + TOOLBAR_W + 5; x += 2) {
            iprintf("\x1b[%d;%dH  ", (y / 8) + 1, (x / 8));
        }
    }

    // Highlight active button
    const char* exitColor = (currentMode == StudioMode::Editing) ? "\x1b[107m" : "\x1b[37m";
    const char* saveColor = "\x1b[104m";
    const char* loadColor = "\x1b[103m";
    const char* testColor = (currentMode == StudioMode::PlayTesting) ? "\x1b[102m" : "\x1b[32m";
    const char* toolboxColor = (currentMode == StudioMode::Toolbox) ? "\x1b[105m" : "\x1b[35m";
    const char* importColor = "\x1b[36m";
    const char* addColor = "\x1b[32m";
    const char* exportColor = "\x1b[34m";

    // 8 Buttons evenly spaced
    iprintf("\x1b[%d;%dH%sEXIT   \x1b[0m", (TOOLBAR_Y + 2) / 8 + 1, TOOLBAR_X / 8 + 1, exitColor);
    iprintf("\x1b[%d;%dH%sSAVE   \x1b[0m", (TOOLBAR_Y + TOOLBAR_BTN_H + 2) / 8 + 1, TOOLBAR_X / 8 + 1, saveColor);
    iprintf("\x1b[%d;%dH%sLOAD   \x1b[0m", (TOOLBAR_Y + TOOLBAR_BTN_H * 2 + 2) / 8 + 1, TOOLBAR_X / 8 + 1, loadColor);
    iprintf("\x1b[%d;%dH%sTEST   \x1b[0m", (TOOLBAR_Y + TOOLBAR_BTN_H * 3 + 2) / 8 + 1, TOOLBAR_X / 8 + 1, testColor);
    iprintf("\x1b[%d;%dH%sTOOLBOX\x1b[0m", (TOOLBAR_Y + TOOLBAR_BTN_H * 4 + 2) / 8 + 1, TOOLBAR_X / 8 + 1, toolboxColor);
    iprintf("\x1b[%d;%dH%sIMPORT \x1b[0m", (TOOLBAR_Y + TOOLBAR_BTN_H * 5 + 2) / 8 + 1, TOOLBAR_X / 8 + 1, importColor);
    iprintf("\x1b[%d;%dH%sADD    \x1b[0m", (TOOLBAR_Y + TOOLBAR_BTN_H * 6 + 2) / 8 + 1, TOOLBAR_X / 8 + 1, addColor);
    iprintf("\x1b[%d;%dH%sEXPORT \x1b[0m", (TOOLBAR_Y + TOOLBAR_BTN_H * 7 + 2) / 8 + 1, TOOLBAR_X / 8 + 1, exportColor);
}

void drawExplorer(Node* root, int scrollOffset) {
    if (!root) return;


    // Explorer header
    iprintf("\x1b[%d;%dH\x1b[104m EXPLORER \x1b[0m", (EXPLORER_Y + 2) / 8 + 1, EXPLORER_X / 8);

    // Draw tree
    int line = (EXPLORER_Y + 20) / 8 + 1;
    std::vector<ExplorerItem> items;

    // Collect items recursively
    collectExplorerItems(root, items, 0);

    for (int i = scrollOffset; i < (int)items.size(); i++) {
        if (line >= 28) break;

        const ExplorerItem& item = items[i];

        // Indentation
        char indent[item.depth * 3 + 1];
        memset(indent, ' ', item.depth * 3);
        indent[item.depth * 3] = '\0';

        // Checkmark for selected
        const char* check = (item.node == selectedNode) ? ">>" : "  ";

        char buf[256];
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH%s%s %.*s", line, EXPLORER_X / 8, check, indent,
                std::min(18, (int)item.name.size()), item.name.c_str());
        iprintf("%s", buf);

        line++;
    }
}

// Helper to collect explorer items recursively
static void collectExplorerItems(Node* node, std::vector<ExplorerItem>& items, int depth) {
    if (!node) return;

    ExplorerItem item;
    item.node = node;
    item.name = node->name[0] ? node->name : "Untitled";
    item.type = node->type[0] ? node->type : "Node";
    item.depth = depth;
    item.selected = (node == selectedNode);
    item.expanded = true;
    items.push_back(item);

    for (Node* child : node->children) {
        collectExplorerItems(child, items, depth + 1);
    }
}

// Helper to fetch toolbox assets from proxy
static void fetchToolboxAssets() {
    // Fallback: populate with placeholder assets for local testing
    if (toolboxAssets.empty()) {
        toolboxAssets.clear();
        for (int i = 1; i <= 10; i++) {
            ToolboxAsset asset;
            asset.name = "Asset_" + std::to_string(i);
            asset.assetId = std::to_string(i * 12345);
            asset.creator = "TestUser";
            asset.downloads = i * 100;
            asset.isFavorite = false;
            toolboxAssets.push_back(asset);
        }
    }
}

void drawProperties(Node* node, int scrollOffset) {
    if (!node) {
        consoleSelect(&bottomConsole);
        iprintf("\x1b[%d;%dH\x1b[104m PROPERTIES \x1b[0m", (PROPERTIES_Y + 2) / 8 + 1, PROPERTIES_X / 8);
        iprintf("\x1b[%d;%dH(None selected)", (PROPERTIES_Y + 20) / 8 + 1, PROPERTIES_X / 8);
        return;
    }

    consoleSelect(&bottomConsole);

    // Properties header
    iprintf("\x1b[%d;%dH\x1b[104m PROPERTIES \x1b[0m", (PROPERTIES_Y + 2) / 8 + 1, PROPERTIES_X / 8);

    int line = (PROPERTIES_Y + 20) / 8 + 1;

    // Name
    iprintf("\x1b[%d;%dHName:  %s", line++, PROPERTIES_X / 8, node->name[0] ? node->name : "Untitled");

    // Type
    iprintf("\x1b[%d;%dHType:  %s", line++, PROPERTIES_X / 8, node->type[0] ? node->type : "Node");

    // Part-specific properties
    if (!strcmp(node->type, "Part") || !strcmp(node->type, "BasePart")) {
        Part* part = static_cast<Part*>(node);

        // Position
        iprintf("\x1b[%d;%dHPosition:", line++, PROPERTIES_X / 8);
        iprintf("\x1b[%d;%dH  X: %.1f", line++, PROPERTIES_X / 8, part->cf.X);
        iprintf("\x1b[%d;%dH  Y: %.1f", line++, PROPERTIES_X / 8, part->cf.Y);
        iprintf("\x1b[%d;%dH  Z: %.1f", line++, PROPERTIES_X / 8, part->cf.Z);

        // Size
        iprintf("\x1b[%d;%dHSize:", line++, PROPERTIES_X / 8);
        iprintf("\x1b[%d;%dH  X: %.1f", line++, PROPERTIES_X / 8, part->scale.x);
        iprintf("\x1b[%d;%dH  Y: %.1f", line++, PROPERTIES_X / 8, part->scale.y);
        iprintf("\x1b[%d;%dH  Z: %.1f", line++, PROPERTIES_X / 8, part->scale.z);

        // Color
        iprintf("\x1b[%d;%dHColor: %d", line++, PROPERTIES_X / 8, part->color);

        // Transparency
        iprintf("\x1b[%d;%dHTransparency: %.1f", line++, PROPERTIES_X / 8, part->transparency);

        // Material
        iprintf("\x1b[%d;%dHMaterial: %d", line++, PROPERTIES_X / 8, part->material);

        // Anchored
        iprintf("\x1b[%d;%dHAnchored: %s", line++, PROPERTIES_X / 8, part->anchored ? "true" : "false");
    }
}

void drawScriptEditor() {
    consoleSelect(&bottomConsole);

    // Header
    iprintf("\x1b[%d;%dH\x1b[104m SCRIPT EDITOR \x1b[0m", (SCRIPT_EDITOR_Y + 2) / 8 + 1, SCRIPT_EDITOR_X / 8);

    // Script content
    char* lines = scriptBuffer;
    int lineNum = (SCRIPT_EDITOR_Y + 20) / 8 + 1;

    for (int i = 0; i < 22 && lines[i * 40]; i++) {
        char line[42];
        memcpy(line, lines + i * 40, 40);
        line[40] = '\0';

        iprintf("\x1b[%d;%dH%s", lineNum + i, SCRIPT_EDITOR_X / 8, line);
    }
}

void drawToolbox() {
    consoleSelect(&bottomConsole);
    consoleClear();

    // "go back" button (Top Left)
    iprintf("\x1b[%d;%dH\x1b[105m GO BACK \x1b[0m", 1, 1);

    // Top row: Genre dropdown and Search field
    iprintf("\x1b[%d;%dHgenre:", 3, 2);
    
    // Genre dropdown (currently selected category)
    const char* genreName = "Models";
    switch (toolboxCategory) {
        case ToolboxCategory::Models: genreName = "Models"; break;
        case ToolboxCategory::Meshes: genreName = "Meshes"; break;
        case ToolboxCategory::Decals: genreName = "Decals"; break;
        case ToolboxCategory::Audio: genreName = "Audio"; break;
        default: genreName = "All"; break;
    }
    iprintf("\x1b[%d;%dH\x1b[47m%-10s \x1b[0m", 3, 12, genreName);

    iprintf("\x1b[%d;%dHsearch:", 3, 28);
    iprintf("\x1b[%d;%dH\x1b[47m%-12s \x1b[0m", 3, 38, toolboxSearchInput);

    // Asset list (Middle area)
    int line = 5;
    for (int i = toolboxScroll; i < (int)toolboxAssets.size() && line < 22; i++) {
        const ToolboxAsset& asset = toolboxAssets[i];
        const char* sel = (i == toolboxSelectedIndex) ? ">> " : "   ";
        char assetLine[128];
        snprintf(assetLine, sizeof(assetLine), "\x1b[%d;%dH%s%s (id: %s)", line, 10, sel, asset.name.c_str(), asset.assetId.c_str());
        iprintf("%s", assetLine);
        line++;
    }

    // Bottom row: Page navigation
    iprintf("\x1b[%d;%dH\x1b[105m ◀ \x1b[0m", 27, 10);
    iprintf("\x1b[%d;%dHPage %d  \x1b[0m", 27, 15);
    iprintf("\x1b[%d;%dH\x1b[105m ▶ \x1b[0m", 27, 24);
}

void insertToolboxAsset(const std::string& assetId, Node* dataModel) {
    if (!dataModel || assetId.empty()) return;

    // Get the proxy URL from settings
    std::string proxyUrl = Settings::get().general.toolboxProxy;
    if (proxyUrl.empty()) {
        iprintf("\x1b[1;1H\x1b[41mNo Toolbox Proxy configured!\x1b[0m");
        return;
    }

    // Construct the asset download URL
    std::string downloadUrl = proxyUrl + "/v1/assets/" + assetId;

    // Determine asset type from category
    AssetLoader::AssetType type = AssetLoader::AssetType::Mesh;
    switch (toolboxCategory) {
        case ToolboxCategory::Models: type = AssetLoader::AssetType::Place; break;
        case ToolboxCategory::Meshes: type = AssetLoader::AssetType::Mesh; break;
        case ToolboxCategory::Decals: type = AssetLoader::AssetType::Decal; break;
        case ToolboxCategory::Audio: type = AssetLoader::AssetType::Sound; break;
        default: type = AssetLoader::AssetType::Mesh; break;
    }

    // Start download through asset loader
    AssetLoader::startDownload(assetId, type, downloadUrl);
    printf("Toolbox: Downloading asset %s from proxy %s\n", assetId.c_str(), proxyUrl.c_str());

    // Insert placeholder part into workspace
    Part* part = createPart("Toolbox_" + assetId, 0, 0, 0);
    part->SetParent(dataModel);
    selectedNode = part;

    iprintf("\x1b[1;1H\x1b[42mInserted asset %s\x1b[0m", assetId.c_str());
}

void processInput(Node* dataModel) {
    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    // Touch input
    touchPosition touch;
    touchRead(&touch);

    // Toolbar buttons
    if (kDown & KEY_TOUCH) {
        int tx = touch.px;
        int ty = touch.py;

        // EXIT button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y && ty <= TOOLBAR_Y + TOOLBAR_BTN_H) {
            running = false;
            return;
        }

        // SAVE button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y + TOOLBAR_BTN_H &&
            ty <= TOOLBAR_Y + TOOLBAR_BTN_H * 2) {
            saveWorkspace(dataModel, lastSavedPath);
        }

        // LOAD button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y + TOOLBAR_BTN_H * 2 &&
            ty <= TOOLBAR_Y + TOOLBAR_BTN_H * 3) {
            loadWorkspace(&workspaceNode, "");
        }

        // TEST button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y + TOOLBAR_BTN_H * 3 &&
            ty <= TOOLBAR_Y + TOOLBAR_BTN_H * 4) {
            if (currentMode == StudioMode::PlayTesting) {
                exitPlayMode();
            } else {
                enterPlayMode(dataModel);
            }
        }

        // TOOLBOX button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y + TOOLBAR_BTN_H * 4 &&
            ty <= TOOLBAR_Y + TOOLBAR_BTN_H * 5) {
            if (currentMode == StudioMode::Toolbox) {
                currentMode = StudioMode::Editing;
            } else {
                currentMode = StudioMode::Toolbox;
                toolboxPage = 1;
                strcpy(toolboxSearchInput, "");
                toolboxGenreDropdown = 0;
                fetchToolboxAssets();
            }
        }

        // IMPORT button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y + TOOLBAR_BTN_H * 5 &&
            ty <= TOOLBAR_Y + TOOLBAR_BTN_H * 6) {
            // Import file browser - scan for PNG/GIF files
            DIR* impDir = opendir("sdmc:/OpenBLOX/shareddata/");
            if (impDir) {
                struct dirent* ent;
                while ((ent = readdir(impDir)) != nullptr) {
                    if (strstr(ent->d_name, ".png") || strstr(ent->d_name, ".gif")) {
                        printf("[Import] Found: %s\n", ent->d_name);
                        break;
                    }
                }
                closedir(impDir);
            }
        }

        // ADD button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y + TOOLBAR_BTN_H * 6 &&
            ty <= TOOLBAR_Y + TOOLBAR_BTN_H * 7) {
            // Add object menu - create Part as default
            if (dataModel) {
                std::string name = "NewPart";
                Part* part = createPart(name, 0, 0, 0);
                part->SetParent(dataModel);
            }
        }

        // EXPORT button
        if (tx >= TOOLBAR_X && tx <= TOOLBAR_X + TOOLBAR_W &&
            ty >= TOOLBAR_Y + TOOLBAR_BTN_H * 7 &&
            ty <= TOOLBAR_Y + TOOLBAR_BTN_H * 8) {
            // Export workspace to RBXM/RBXL using serializeNodeToXML
            if (dataModel) {
                FILE* f = fopen("sdmc:/OpenBLOX/export.rbxm", "wb");
                if (f) {
                    fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<roblox version=\"4\">\n");
                    serializeNodeToXML(f, dataModel, 1);
                    fprintf(f, "</roblox>\n");
                    fclose(f);
                    printf("[Export] Saved to export.rbxm\n");
                }
            }
        }

        // Toolbox input handling
        if (currentMode == StudioMode::Toolbox) {
            // "go back" button (top-left)
            if (tx >= 10 && tx <= 70 && ty >= 5 && ty <= 35) {
                currentMode = StudioMode::Editing;
            }

            // Genre dropdown
            if (tx >= 80 && tx <= 200 && ty >= 5 && ty <= 35) {
                toolboxGenreDropdown = (toolboxGenreDropdown + 1) % 7;
            }

            // Search field (open 3DS keyboard)
            if (tx >= 210 && tx <= 310 && ty >= 5 && ty <= 35) {
                osCreateFlex(OS_FLEXPOS_BOTTOM, OS_FLEXSIZE_FULL);
                osInitializeFlex(OS_FLEXPOS_BOTTOM);
                osShowFlex(0);
                char searchBuf[256];
                osGetFlexInput(searchBuf, sizeof(searchBuf), toolboxSearchInput, 30);
            }

            // Page navigation (bottom)
            if (tx >= 100 && tx <= 130 && ty >= 200 && ty <= 230) {
                toolboxPage = std::max(1, toolboxPage - 1);
            }
            if (tx >= 190 && tx <= 220 && ty >= 200 && ty <= 230) {
                toolboxPage++;
            }
        }

        // Explorer touch selection
        if (currentMode == StudioMode::Editing &&
            tx >= EXPLORER_X && tx <= EXPLORER_X + EXPLORER_W) {
            selectByExplorerTouch(dataModel, touch);
        }
    }

    // Keyboard navigation
    if (kDown & KEY_UP) {
        explorerScroll = std::max(0, explorerScroll - 1);
    }
    if (kDown & KEY_DOWN) {
        explorerScroll++;
    }
    if (kDown & KEY_LEFT) {
        propertiesScroll = std::max(0, propertiesScroll - 1);
    }
    if (kDown & KEY_RIGHT) {
        propertiesScroll++;
    }

    // Delete selected (Y button)
    if (kDown & KEY_Y && selectedNode) {
        deleteSelected(dataModel);
    }

    // Create new part (A button)
    if (kDown & KEY_A && dataModel && currentMode == StudioMode::Editing) {
        createPart("NewPart", 0, 0, 0)->SetParent(dataModel);
    }

    // Toggle script editor (X button)
    if (kDown & KEY_X) {
        if (currentMode == StudioMode::ScriptEditor)
            currentMode = StudioMode::Editing;
        else
            currentMode = StudioMode::ScriptEditor;
    }

    // Stop play mode (B button)
    if (kDown & KEY_B && currentMode == StudioMode::PlayTesting) {
        exitPlayMode();
    }

    // Move selected part (D-pad when playing)
    if (selectedNode && (!strcmp(selectedNode->type, "Part") || !strcmp(selectedNode->type, "BasePart"))) {
        Part* part = static_cast<Part*>(selectedNode);
        float moveAmount = 0.5f;

        if (kHeld & KEY_L) part->cf.X -= moveAmount;
        if (kHeld & KEY_R) part->cf.X += moveAmount;
        if (kHeld & KEY_A) part->cf.Y += moveAmount; // Lift
    }
}

// Select node by touching explorer
static void selectByExplorerTouch(Node* root, touchPosition touch) {
    if (!root) return;

    std::vector<ExplorerItem> items;
    collectExplorerItems(root, items, 0);

    int line = (EXPLORER_Y + 20) / 8;
    int idx = (touch.py - line) / 8;

    if (idx >= 0 && idx < (int)items.size()) {
        selectedNode = items[idx + explorerScroll].node;
    }
}

void selectPart(Node* node) {
    selectedNode = node;
}

Part* createPart(const std::string& name, float x, float y, float z) {
    Part* part = new Part();
    strncpy(part->name, name.c_str(), 63);
    part->name[63] = '\0';
    strncpy(part->type, "Part", 31);
    part->type[31] = '\0';
    part->cf.X = x;
    part->cf.Y = y;
    part->cf.Z = z;
    part->scale.x = 4;
    part->scale.y = 4;
    part->scale.z = 4;
    part->color = 2;
    part->transparency = 0;
    part->material = Material_Plastic;
    part->parent = nullptr;
    return part;
}

void deleteSelected(Node* dataModel) {
    if (!selectedNode || !dataModel) return;

    // Remove from parent's children
    if (selectedNode->parent) {
        auto& children = selectedNode->parent->children;
        children.erase(std::remove(children.begin(), children.end(), selectedNode), children.end());
    }

    delete selectedNode;
    selectedNode = nullptr;
}

bool saveWorkspace(Node* dataModel, const std::string& path) {
    if (!path.empty() && dataModel) {
        FILE* f = fopen(path.c_str(), "wb");
        if (f) {
            fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<roblox version=\"4\">\n");
            serializeNodeToXML(f, dataModel, 1);
            fprintf(f, "</roblox>\n");
            fclose(f);
            lastSavedPath = path;
            return true;
        }
    }
    return false;
}

bool loadWorkspace(Node** dataModel, const std::string& path) {
    std::string loadPath = path.empty() ? "sdmc:/OpenBLOX/untitled.rbxl" : path;
    FILE* f = fopen(loadPath.c_str(), "rb");
    if (f) {
        *dataModel = parseRBXL(f);
        fclose(f);
        return *dataModel != nullptr;
    }
    return false;
}

void enterPlayMode(Node* dataModel) {
    currentMode = StudioMode::PlayTesting;
}

void exitPlayMode() {
    currentMode = StudioMode::Editing;
}

Node* getSelectedNode() {
    return selectedNode;
}

void setSelectedNode(Node* node) {
    selectedNode = node;
}

void shutdown() {
    running = false;
    selectedNode = nullptr;
    workspaceNode = nullptr;
}

} // namespace StudioUI