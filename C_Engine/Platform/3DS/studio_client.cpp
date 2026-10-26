#include "studio_client.h"
#include "citro_renderer.h"
#include "font_manager.h"
#include "content_system.h"
#include "render_pipeline.h"
#include "materials.h"
#include "texture_loader.h"
#include "gif_decoder.h"
#include "studio_objects.h"
#include "script_engine.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

using namespace StudioClient;

// UI layout constants for bottom screen (320x240)
static const int TOOLBAR_X = 5;
static const int TOOLBAR_Y = 5;
static const int TOOLBAR_W = 45;
static const int TOOLBAR_BTN_H = 25;
static const int TOOLBAR_BTN_TEXT_SIZE = 8;

static const int EXPLORER_X = 55;
static const int EXPLORER_Y = 5;
static const int EXPLORER_W = 140;
static const int EXPLORER_H = 110;

static const int PROPERTIES_X = 55;
static const int PROPERTIES_Y = 120;
static const int PROPERTIES_W = 260;
static const int PROPERTIES_H = 115;

static const int SCRIPT_X = 55;
static const int SCRIPT_Y = 5;
static const int SCRIPT_W = 260;
static const int SCRIPT_H = 230;

// Toolbar button labels
static const char* buttonLabels[] = {"exit", "save", "load", "test", "import", "add"};
static const int TOOLBAR_BTN_COUNT = 6;

// State
static StudioState studioState;
static int toolbarCursor = 0;
static int explorerCursor = 0;
static int propertiesCursor = 0;
static bool studioRunning = false;
static float viewportRotY = 0.0f;
static PrintConsole consoleTop;
static PrintConsole consoleBottom;

// Viewport camera
static float camDist = 80.0f;
static float camHeight = 40.0f;

void StudioClient::init() {
    // Initialize console outputs
    consoleInit(GFX_TOP, &consoleTop);
    consoleInit(GFX_BOTTOM, &consoleBottom);
    
    studioState = StudioState();
    toolbarCursor = 0;
    explorerCursor = 0;
    propertiesCursor = 0;
    studioRunning = false;
    viewportRotY = 0.0f;
    
    // Initialize studio object templates
    StudioObjects::init();
    
    // Add default BasePlatform part
    addInstance("BasePart", -1);
    StudioInstance* base = getInstance(0);
    if (base) {
        base->name = "BasePlatform";
        base->position[0] = 0; base->position[1] = -1; base->position[2] = 0;
        base->size[0] = 256; base->size[1] = 2; base->size[2] = 256;
        base->color[0] = 0.39f; base->color[1] = 0.86f; base->color[2] = 0.47f; // Bright green
    }
}

std::string StudioClient::runStudio() {
    init();
    
    studioRunning = true;
    std::string result = "";
    
    while (studioRunning) {
        // Draw top screen viewport
        drawViewport();
        
        // Draw bottom screen UI
        consoleSelect(&consoleBottom);
        consoleClear();
        
        // Draw appropriate tab
        switch (studioState.activeTab) {
            case StudioTab::Explorer:
                drawExplorer();
                drawProperties();
                break;
            case StudioTab::ScriptEditor:
                drawScriptEditor();
                break;
            case StudioTab::Properties:
                drawProperties();
                break;
        }
        
        // Draw toolbar (always visible)
        drawToolbar();
        
        // Swap frames - use C2D SwapBuffers
        GSPGPU_FlushDataCache(NULL, 0);
        
        // Process input
        hidScanInput();
        processInput();
        
        // Auto-rotate viewport
        if (!studioState.isTesting) {
            viewportRotY += 0.3f;
            if (viewportRotY > 360.0f) viewportRotY -= 360.0f;
        }
        
        svcSleepThread(16666666ULL); // ~60fps
    }
    
    return result;
}

void StudioClient::drawViewport() {
    consoleSelect(&consoleTop);
    CitroRenderer::startFrame();
    CitroRenderer::clearScreen(0.15f, 0.15f, 0.2f, 1.0f);
    
    // Setup camera orbiting around center
    CitroRenderer::perspective(60.0f, 400.0f/240.0f, 0.1f, 1000.0f);
    
    float camX = camDist * cosf(viewportRotY * 0.0174533f);
    float camZ = camDist * sinf(viewportRotY * 0.0174533f);
    
    CitroRenderer::lookAt(camX, camHeight, camZ, 0, 0, 0, 0, 1, 0);
    
    // Draw grid
    for (int i = -120; i <= 120; i += 20) {
        // Horizontal lines
        CitroRenderer::setColor(0.25f, 0.25f, 0.3f);
        CitroRenderer::drawBox(i, -0.5f, -120, 0.2f, 0.2f, 240, 1, 1, 1);
        CitroRenderer::drawBox(-120, -0.5f, i, 240, 0.2f, 0.2f, 1, 1, 1);
    }
    
    // Draw all instances (parts only)
    for (const auto& inst : studioState.instances) {
        if (inst.className == "BasePart" || inst.className == "Part" || inst.className == "MeshPart") {
            float r = inst.color[0], g = inst.color[1], b = inst.color[2];
            
            // Highlight selected
            if (studioState.instances.data() + (&inst - studioState.instances.data()) == 
                studioState.instances.data() + studioState.selectedInstance) {
                r = std::min(1.0f, r + 0.3f);
                g = std::min(1.0f, g + 0.3f);
                b = std::min(1.0f, b + 0.3f);
            }
            
            CitroRenderer::drawBox(
                inst.position[0], inst.position[1], inst.position[2],
                inst.size[0], inst.size[1], inst.size[2],
                r, g, b
            );
            
            // Draw selection outline
            if (studioState.selectedInstance >= 0 && 
                (int)(&inst - studioState.instances.data()) == studioState.selectedInstance) {
                float s0 = inst.size[0]/2 + 0.3f, s1 = inst.size[1]/2 + 0.3f, s2 = inst.size[2]/2 + 0.3f;
                CitroRenderer::setColor(1, 1, 0); // Yellow outline
                // Wireframe box edges (simplified)
                CitroRenderer::drawBox(inst.position[0], inst.position[1], inst.position[2],
                    s0*2, 0.3f, 0.3f, 1, 1, 0);
                CitroRenderer::drawBox(inst.position[0], inst.position[1], inst.position[2],
                    0.3f, s1*2, 0.3f, 1, 1, 0);
            }
        }
    }
    
    // Draw "INSERT RENDERED GAME THAT YOU ARE EDITING" text if no instances
    if (studioState.instances.empty()) {
        consoleSelect(&consoleTop);
        iprintf("\x1b[32;10HINSERT RENDERED GAME\nTHAT YOU ARE EDITING");
    }
    
    // Draw mode indicator
    if (studioState.isTesting) {
        consoleSelect(&consoleTop);
        iprintf("\x1b[1;1H[TEST MODE]");
    }
    
    CitroRenderer::endFrame();
}

void StudioClient::drawToolbar() {
    consoleSelect(&consoleBottom);
    
    for (int i = 0; i < TOOLBAR_BTN_COUNT; i++) {
        int y = TOOLBAR_Y + i * (TOOLBAR_BTN_H + 2);
        bool isCursor = (i == toolbarCursor);
        
        // Button background
        int r = isCursor ? 70 : 40;
        int g = isCursor ? 70 : 40;
        int b = isCursor ? 70 : 40;
        
        // Button text
        iprintf("\x1b[%d;%dH%s", (y + 12) / 8, (TOOLBAR_X + 8) / 8, buttonLabels[i]);
    }
}

void StudioClient::drawExplorer() {
    consoleSelect(&consoleBottom);
    
    // Panel header
    iprintf("\x1b[%d;%dH[EXPLORER]", (EXPLORER_Y + 10) / 8, (EXPLORER_X + 3) / 8);
    
    // Draw workspace header
    iprintf("\x1b[%d;%dH[+] Workspace", (EXPLORER_Y + 22) / 8, (EXPLORER_X + 2) / 8);
    
    // Draw instances
    int line = 30;
    for (size_t i = 0; i < studioState.instances.size(); i++) {
        if (i < (size_t)explorerCursor || line >= EXPLORER_H - 5) break;
        
        const auto& inst = studioState.instances[i];
        int idx = (int)i - std::max(0, (int)i - (EXPLORER_H / 14 - 3));
        if (i >= (size_t)std::max(0, explorerCursor - (EXPLORER_H / 14 - 4))) {
            bool isSelected = ((int)i == studioState.selectedInstance);
            
            // Get icon from StudioObjects registry
            char icon = '?';
            const StudioObjects::ObjectTemplate* tmpl = StudioObjects::getTemplate(inst.className);
            if (tmpl) {
                icon = tmpl->icon;
            } else if (inst.className == "BasePart" || inst.className == "Part") {
                icon = 'P';
            } else if (inst.className == "Model") {
                icon = '#';
            } else if (inst.className == "Folder") {
                icon = 'F';
            } else if (inst.className == "Script" || inst.className == "LocalScript") {
                icon = 'S';
            } else if (inst.className == "Decal") {
                icon = 'D';
            } else if (inst.className.find("Gui") != std::string::npos || 
                       inst.className.find("Label") != std::string::npos ||
                       inst.className.find("Button") != std::string::npos ||
                       inst.className.find("Frame") != std::string::npos) {
                icon = 'U';
            } else if (inst.className.find("Value") != std::string::npos) {
                icon = 'V';
            } else if (inst.className.find("Constraint") != std::string::npos ||
                       inst.className == "Weld" || inst.className == "Motor6D") {
                icon = 'C';
            }
            
            char buf[120];
            snprintf(buf, sizeof(buf), "\x1b[%d;%dH%s [%c] %s", 
                     (EXPLORER_Y + line) / 8, (EXPLORER_X + 4) / 8,
                     isSelected ? ">" : " ", icon, inst.name.c_str());
            iprintf("%s", buf);
            line += 14;
        }
    }
}

void StudioClient::drawProperties() {
    consoleSelect(&consoleBottom);
    
    // Panel header
    iprintf("\x1b[%d;%dH[PROPERTIES]", (PROPERTIES_Y + 10) / 8, (PROPERTIES_X + 3) / 8);
    
    if (studioState.selectedInstance < 0 || 
        studioState.selectedInstance >= (int)studioState.instances.size()) {
        iprintf("\x1b[%d;%dHNo selection", (PROPERTIES_Y + 22) / 8, (PROPERTIES_X + 3) / 8);
        return;
    }
    
    const auto& inst = studioState.instances[studioState.selectedInstance];
    
    int line = PROPERTIES_Y + 22;
    
    // Name property
    char buf[160];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dHName: %s", line / 8, (PROPERTIES_X + 3) / 8, inst.name.c_str());
    iprintf("%s", buf);
    line += 12;
    
    snprintf(buf, sizeof(buf), "\x1b[%d;%dHClass: %s", line / 8, (PROPERTIES_X + 3) / 8, inst.className.c_str());
    iprintf("%s", buf);
    line += 12;
    
    // Position
    snprintf(buf, sizeof(buf), "\x1b[%d;%dHPosition: %.1f, %.1f, %.1f", line / 8, (PROPERTIES_X + 3) / 8,
             inst.position[0], inst.position[1], inst.position[2]);
    iprintf("%s", buf);
    line += 12;
    
    // Size
    snprintf(buf, sizeof(buf), "\x1b[%d;%dHSize: %.1f, %.1f, %.1f", line / 8, (PROPERTIES_X + 3) / 8,
             inst.size[0], inst.size[1], inst.size[2]);
    iprintf("%s", buf);
    line += 12;
    
    // Color
    snprintf(buf, sizeof(buf), "\x1b[%d;%dHColor: %.2f, %.2f, %.2f", line / 8, (PROPERTIES_X + 3) / 8,
             inst.color[0], inst.color[1], inst.color[2]);
    iprintf("%s", buf);
    line += 12;
    
    // Rotation
    snprintf(buf, sizeof(buf), "\x1b[%d;%dHRotation: %.0f, %.0f, %.0f", line / 8, (PROPERTIES_X + 3) / 8,
             inst.rotation[0], inst.rotation[1], inst.rotation[2]);
    iprintf("%s", buf);
    
    // Scrollbar indicator
    iprintf("\x1b[%d;%dH[v]", (PROPERTIES_Y + PROPERTIES_H - 5) / 8, (PROPERTIES_W - 10) / 8);
}

void StudioClient::drawScriptEditor() {
    consoleSelect(&consoleBottom);
    
    // Panel header
    iprintf("\x1b[%d;%dH[SCRIPT EDITOR]", (SCRIPT_Y + 10) / 8, (SCRIPT_X + 3) / 8);
    
    // Find selected script instance
    StudioInstance* scriptInst = nullptr;
    if (studioState.selectedInstance >= 0 && 
        studioState.selectedInstance < (int)studioState.instances.size()) {
        auto& inst = studioState.instances[studioState.selectedInstance];
        if (inst.className == "Script" || inst.className == "LocalScript") {
            scriptInst = &inst;
        }
    }
    
    // If no script selected, find any script
    if (!scriptInst) {
        for (auto& inst : studioState.instances) {
            if (inst.className == "Script" || inst.className == "LocalScript") {
                scriptInst = &inst;
                break;
            }
        }
    }
    
    int line = SCRIPT_Y + 22;
    
    if (scriptInst && !scriptInst->scriptSource.empty()) {
        // Draw script lines with line numbers
        const char* src = scriptInst->scriptSource.c_str();
        int lineNum = studioState.scriptEditorScroll + 1;
        
        for (int i = studioState.scriptEditorScroll; i < studioState.scriptEditorScroll + 15; i++) {
            // Find next line
            char lineBuf[200];
            int j = i;
            int k = 0;
            // Simple line extraction
            const char* start = src;
            for (int c = 0; c < i; c++) {
                if (*start == '\n') start++;
                start++;
            }
            while (*start && *start != '\n' && k < 199) {
                lineBuf[k++] = *start++;
            }
            lineBuf[k] = 0;
            
            char buf[256];
            snprintf(buf, sizeof(buf), "\x1b[%d;%dH%3d: %s", line / 8, (SCRIPT_X + 1) / 8, lineNum, lineBuf);
            iprintf("%s", buf);
            line += 12;
            lineNum++;
        }
    } else {
        // No script to edit - draw placeholder
        iprintf("\x1b[%d;%dHscript editor here", (SCRIPT_Y + 80) / 8, (SCRIPT_X + 30) / 8);
        iprintf("\x1b[%d;%dHSelect or create a Script", (SCRIPT_Y + 100) / 8, (SCRIPT_X + 20) / 8);
    }
}

void StudioClient::processInput() {
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    
    // Toolbar navigation
    if (kDown & KEY_UP) {
        toolbarCursor--;
        if (toolbarCursor < 0) toolbarCursor = TOOLBAR_BTN_COUNT - 1;
    }
    if (kDown & KEY_DOWN) {
        toolbarCursor++;
        if (toolbarCursor >= TOOLBAR_BTN_COUNT) toolbarCursor = 0;
    }
    
    // Toolbar action
    if (kDown & KEY_A) {
        switch (toolbarCursor) {
            case 0: // Exit
                studioRunning = false;
                break;
            case 1: // Save
                if (!studioState.currentFile.empty()) {
                    savePlace(studioState.currentFile);
                } else {
                    std::string path = "sdmc:/OpenBLOX/untitled.rbxl";
                    savePlace(path);
                    studioState.currentFile = path;
                }
                studioState.modified = false;
                break;
            case 2: // Load
                break;
            case 3: // Test
                if (studioState.isTesting) {
                    stopTest();
                } else {
                    startTest();
                }
                break;
            case 4: // Import (Images/GIFs)
                importAssets();
                break;
            case 5: // Add (Object menu)
                showAddMenu();
                break;
        }
    }
    
    // Explorer navigation (when not in toolbar focus)
    if (studioState.activeTab == StudioTab::Explorer) {
        if (kDown & KEY_RIGHT) {
            explorerCursor++;
            if (explorerCursor >= (int)studioState.instances.size()) 
                explorerCursor = 0;
            
            // Select instance in viewport
            studioState.selectedInstance = explorerCursor;
        }
        if (kDown & KEY_LEFT) {
            explorerCursor--;
            if (explorerCursor < 0) 
                explorerCursor = studioState.instances.size() - 1;
            studioState.selectedInstance = explorerCursor;
        }
    }
    
    // Property editing
    if (studioState.selectedInstance >= 0 && 
        studioState.selectedInstance < (int)studioState.instances.size()) {
        auto& inst = studioState.instances[studioState.selectedInstance];
        
        if (kDown & KEY_X) {
            // Toggle tab
            switch (studioState.activeTab) {
                case StudioTab::Explorer:
                    studioState.activeTab = StudioTab::ScriptEditor;
                    break;
                case StudioTab::ScriptEditor:
                    studioState.activeTab = StudioTab::Properties;
                    break;
                case StudioTab::Properties:
                    studioState.activeTab = StudioTab::Explorer;
                    break;
            }
        }
        
        if (kDown & KEY_Y) {
            // Quick add Part
            addInstance("BasePart", -1);
            studioState.modified = true;
        }
        
        if (kDown & KEY_ZR) {
            // Quick add Script
            addInstance("Script", -1);
            studioState.modified = true;
        }
        
        if (kDown & KEY_R) {
            // Rotate selected part
            inst.rotation[1] += 45;
            if (inst.rotation[1] >= 360) inst.rotation[1] -= 360;
            studioState.modified = true;
        }
        
        if (kDown & KEY_L) {
            // Delete selected
            deleteInstance(studioState.selectedInstance);
            studioState.selectedInstance = -1;
            studioState.modified = true;
        }
    }
    
    // Touch input for toolbar buttons
    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        touchRead(&touch);
        
        // Check toolbar area
        if (touch.px >= TOOLBAR_X && touch.px <= TOOLBAR_X + TOOLBAR_W) {
            int btn = (touch.py - TOOLBAR_Y) / (TOOLBAR_BTN_H + 2);
            if (btn >= 0 && btn < TOOLBAR_BTN_COUNT) {
                toolbarCursor = btn;
                // Simulate A press for selected button
                switch (btn) {
                    case 0:
                        studioRunning = false;
                        break;
                    case 1:
                        if (!studioState.currentFile.empty()) {
                            savePlace(studioState.currentFile);
                        }
                        studioState.modified = false;
                        break;
                    case 2:
                        break; // Load
                    case 3:
                        if (studioState.isTesting) stopTest();
                        else startTest();
                        break;
                    case 4:
                        importAssets();
                        break;
                    case 5:
                        showAddMenu();
                        break;
                }
            }
        }
    }
    
    // Camera control with touch on top screen
    if (kHeld & (KEY_TOUCH | KEY_CSTICK_LEFT | KEY_CSTICK_RIGHT | KEY_CSTICK_UP | KEY_CSTICK_DOWN)) {
        viewportRotY += 1.0f;
    }
    
    // Zoom
    if (kDown & KEY_R) {
        camDist = std::max(20.0f, camDist - 10.0f);
    }
    if (kDown & KEY_L) {
        camDist = std::min(200.0f, camDist + 10.0f);
    }
}

// Instance management
int StudioClient::addInstance(const std::string& className, int parent) {
    StudioInstance inst;
    inst.className = className;
    inst.name = className + "_" + std::to_string(studioState.instances.size() + 1);
    inst.parentId = parent;
    
    // Set defaults based on class using StudioObjects registry
    const StudioObjects::ObjectTemplate* tmpl = StudioObjects::getTemplate(className);
    if (tmpl) {
        inst.position[0] = tmpl->defaultPosition[0];
        inst.position[1] = tmpl->defaultPosition[1];
        inst.position[2] = tmpl->defaultPosition[2];
        inst.size[0] = tmpl->defaultSize[0];
        inst.size[1] = tmpl->defaultSize[1];
        inst.size[2] = tmpl->defaultSize[2];
        inst.color[0] = tmpl->defaultColor[0];
        inst.color[1] = tmpl->defaultColor[1];
        inst.color[2] = tmpl->defaultColor[2];
    } else if (className == "BasePart" || className == "Part") {
        // Fallback for BasePart
        inst.position[0] = 0; inst.position[1] = 5; inst.position[2] = 0;
        inst.size[0] = 4; inst.size[1] = 4; inst.size[2] = 4;
        inst.color[0] = 0.8f; inst.color[1] = 0.2f; inst.color[2] = 0.2f;
    } else if (className == "Script" || className == "LocalScript") {
        inst.isLocalScript = (className == "LocalScript");
        inst.scriptSource = "-- " + className + " created in OpenBLOX Studio\n\n";
    }
    
    studioState.instances.push_back(inst);
    return (int)studioState.instances.size() - 1;
}

void StudioClient::deleteInstance(int id) {
    if (id >= 0 && id < (int)studioState.instances.size()) {
        studioState.instances.erase(studioState.instances.begin() + id);
        if (studioState.selectedInstance >= (int)studioState.instances.size()) {
            studioState.selectedInstance = -1;
        }
    }
}

void StudioClient::duplicateInstance(int id) {
    if (id >= 0 && id < (int)studioState.instances.size()) {
        auto copy = studioState.instances[id];
        copy.name += "_Copy";
        copy.position[0] += 8;
        studioState.instances.push_back(copy);
    }
}

// Import images and GIFs from SD card
void StudioClient::importAssets() {
    // Scan for image files in shareddata/models/ and shareddata/assets/
    DIR* dir = opendir("sdmc:/OpenBLOX/shareddata/");
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() < 5) continue;
        
        std::string ext = name.substr(name.size() - 4);
        if (ext == ".png" || ext == ".jpg" || ext == ".gif" || ext == ".PNG" || ext == ".JPG" || ext == ".GIF") {
            // Create a Decal or AnimatedTexture instance
            addInstance("Decal", -1);
            StudioInstance* decal = getInstance((int)studioState.instances.size() - 1);
            if (decal) {
                decal->name = name.substr(0, name.size() - 4);
                decal->properties["Texture"] = "sdmc:/OpenBLOX/shareddata/" + name;
                decal->properties["IsAnimated"] = (ext == ".gif" || ext == ".GIF") ? "true" : "false";
            }
            studioState.modified = true;
        }
    }
    closedir(dir);
    
    // Also scan models folder
    dir = opendir("sdmc:/OpenBLOX/shareddata/models/");
    if (dir) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name.size() < 5) continue;
            
            std::string ext = name.substr(name.size() - 4);
            if (ext == ".png" || ext == ".jpg" || ext == ".gif") {
                addInstance("Decal", -1);
                StudioInstance* decal = getInstance((int)studioState.instances.size() - 1);
                if (decal) {
                    decal->name = name.substr(0, name.size() - 4);
                    decal->properties["Texture"] = "sdmc:/OpenBLOX/shareddata/models/" + name;
                    decal->properties["IsAnimated"] = (ext == ".gif") ? "true" : "false";
                }
                studioState.modified = true;
            }
        }
        closedir(dir);
    }
}

// Show the "Add" menu for new objects
void StudioClient::showAddMenu() {
    // Display a list of all available object templates
    consoleSelect(&consoleBottom);
    consoleClear();
    
    const auto& templates = StudioObjects::getAllTemplates();
    int line = 5;
    
    iprintf("\x1b[%d;%dH[ADD OBJECT]", 5, 5);
    line = 18;
    
    int idx = 0;
    for (const auto& tmpl : templates) {
        char buf[120];
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH [%c] %s", line / 8, 3, tmpl.icon, tmpl.displayName.c_str());
        iprintf("%s", buf);
        line += 12;
        idx++;
        
        if (idx >= 15 || line >= 220) break;
    }
    
    // Wait for selection (A to confirm, B to cancel)
    u32 kDown;
    int selection = 0;
    while (true) {
        hidScanInput();
        kDown = hidKeysDown();
        
        if (kDown & KEY_UP) {
            selection--;
            if (selection < 0) selection = std::min(14, (int)templates.size() - 1);
        }
        if (kDown & KEY_DOWN) {
            selection++;
            if (selection >= (int)templates.size()) selection = 0;
        }
        if (kDown & KEY_A) {
            // Add selected object
            if (selection >= 0 && selection < (int)templates.size()) {
                addInstance(templates[selection].className, -1);
                studioState.modified = true;
            }
            break;
        }
        if (kDown & KEY_B) {
            break; // Cancel
        }
        
        svcSleepThread(16666666ULL);
    }
}

StudioInstance* StudioClient::getInstance(int id) {
    if (id >= 0 && id < (int)studioState.instances.size()) {
        return &studioState.instances[id];
    }
    return nullptr;
}

std::vector<int> StudioClient::getChildren(int parentId) {
    std::vector<int> children;
    for (size_t i = 0; i < studioState.instances.size(); i++) {
        if (studioState.instances[i].parentId == parentId) {
            children.push_back((int)i);
        }
    }
    return children;
}

// Property management
void StudioClient::setProperty(int instanceId, const std::string& prop, const std::string& value) {
    if (instanceId >= 0 && instanceId < (int)studioState.instances.size()) {
        auto& inst = studioState.instances[instanceId];
        inst.properties[prop] = value;
        studioState.modified = true;
    }
}

std::string StudioClient::getProperty(int instanceId, const std::string& prop) {
    if (instanceId >= 0 && instanceId < (int)studioState.instances.size()) {
        const auto& inst = studioState.instances[instanceId];
        auto it = inst.properties.find(prop);
        if (it != inst.properties.end()) return it->second;
    }
    return "";
}

// File operations
bool StudioClient::savePlace(const std::string& path) {
    // Save as RBXL format (XML-based)
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    
    fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    fprintf(f, "<Roblox version=\"4\">");
    fprintf(f, "<Item class=\"DataModel\" referent=\"RBX0\">\n");
    fprintf(f, "  <Properties></Properties>\n");
    fprintf(f, "  <Items>\n");
    
    // Save Workspace
    fprintf(f, "    <Item class=\"Workspace\" referent=\"RBX1\">\n");
    fprintf(f, "      <Properties></Properties>\n");
    fprintf(f, "      <Items>\n");
    
    for (const auto& inst : studioState.instances) {
        fprintf(f, "        <Item class=\"%s\">\n", inst.className.c_str());
        fprintf(f, "          <Properties>\n");
        fprintf(f, "            <string name=\"Name\">%s</string>\n", inst.name.c_str());
        fprintf(f, "            <float3 name=\"Position\">%.2f %.2f %.2f</float3>\n",
                inst.position[0], inst.position[1], inst.position[2]);
        fprintf(f, "            <float3 name=\"Size\">%.2f %.2f %.2f</float3>\n",
                inst.size[0], inst.size[1], inst.size[2]);
        fprintf(f, "            <float3 name=\"Color\">%.2f %.2f %.2f</float3>\n",
                inst.color[0], inst.color[1], inst.color[2]);
        fprintf(f, "            <float3 name=\"Rotation\">%.0f %.0f %.0f</float3>\n",
                inst.rotation[0], inst.rotation[1], inst.rotation[2]);
        
        if (!inst.scriptSource.empty()) {
            fprintf(f, "            <string name=\"Source\">%s</string>\n", inst.scriptSource.c_str());
        }
        
        fprintf(f, "          </Properties>\n");
        fprintf(f, "        </Item>\n");
    }
    
    fprintf(f, "      </Items>\n");
    fprintf(f, "    </Item>\n");
    fprintf(f, "  </Items>\n");
    fprintf(f, "</Item>\n");
    fprintf(f, "</Roblox>\n");
    
    fclose(f);
    studioState.modified = false;
    return true;
}

bool StudioClient::loadPlace(const std::string& path) {
    // Simple RBXL parser for studio
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(fsize + 1);
    if (!buffer) { fclose(f); return false; }
    
    fread(buffer, 1, fsize, f);
    buffer[fsize] = 0;
    fclose(f);
    
    // Clear existing instances
    studioState.instances.clear();
    
    // Parse XML for Item entries
    char* ptr = buffer;
    while ((ptr = strstr(ptr, "<Item class=\"")) != nullptr) {
        ptr += 13; // skip "<Item class=\""
        char className[64];
        int i = 0;
        while (*ptr && *ptr != '"' && i < 63) className[i++] = *ptr++;
        className[i] = 0;
        ptr++; // skip closing quote
        
        // Skip Workspace and DataModel
        if (strcmp(className, "Workspace") == 0 || strcmp(className, "DataModel") == 0) {
            continue;
        }
        
        // Parse properties
        StudioInstance inst;
        inst.className = className;
        inst.name = className;
        
        // Find Properties block
        char* propsStart = strstr(ptr, "<Properties>");
        char* propsEnd = strstr(ptr, "</Properties>");
        if (propsStart && propsEnd) {
            propsStart += 12;
            char propsBuf[2048];
            int len = (int)(propsEnd - propsStart);
            if (len > 2047) len = 2047;
            strncpy(propsBuf, propsStart, len);
            propsBuf[len] = 0;
            
            char* p = propsBuf;
            
            // Parse Name
            char* nameTag = strstr(p, "<string name=\"Name\">");
            if (nameTag) {
                nameTag += 19;
                char* nameEnd = strstr(nameTag, "</string>");
                if (nameEnd) {
                    int nlen = (int)(nameEnd - nameTag);
                    if (nlen < 64) {
                        char name[64];
                        strncpy(name, nameTag, nlen);
                        name[nlen] = 0;
                        inst.name = name;
                    }
                }
            }
            
            // Parse Position
            char* posTag = strstr(p, "<float3 name=\"Position\">");
            if (posTag) {
                posTag += 22;
                sscanf(posTag, "%f %f %f", &inst.position[0], &inst.position[1], &inst.position[2]);
            }
            
            // Parse Size
            char* sizeTag = strstr(p, "<float3 name=\"Size\">");
            if (sizeTag) {
                sizeTag += 18;
                sscanf(sizeTag, "%f %f %f", &inst.size[0], &inst.size[1], &inst.size[2]);
            }
            
            // Parse Color
            char* colorTag = strstr(p, "<float3 name=\"Color\">");
            if (colorTag) {
                colorTag += 19;
                sscanf(colorTag, "%f %f %f", &inst.color[0], &inst.color[1], &inst.color[2]);
            }
            
            // Parse Rotation
            char* rotTag = strstr(p, "<float3 name=\"Rotation\">");
            if (rotTag) {
                rotTag += 22;
                sscanf(rotTag, "%f %f %f", &inst.rotation[0], &inst.rotation[1], &inst.rotation[2]);
            }
            
            // Parse Source (for scripts)
            char* srcTag = strstr(p, "<string name=\"Source\">");
            if (srcTag) {
                srcTag += 20;
                char* srcEnd = strstr(srcTag, "</string>");
                if (srcEnd) {
                    int slen = (int)(srcEnd - srcTag);
                    char* src = (char*)malloc(slen + 1);
                    strncpy(src, srcTag, slen);
                    src[slen] = 0;
                    inst.scriptSource = src;
                    free(src);
                }
            }
        }
        
        studioState.instances.push_back(inst);
    }
    
    free(buffer);
    return true;
}

// Test mode
void StudioClient::startTest() {
    studioState.isTesting = true;
    // Load game scripts and run them via ScriptEngine
    ScriptEngine::init();
    for (auto& inst : studioState.instances) {
        if (inst.className == "Script" || inst.className == "LocalScript") {
            ScriptEngine::loadScriptString(inst.name, inst.scriptSource);
        }
    }
}

void StudioClient::stopTest() {
    studioState.isTesting = false;
    // ScriptEngine::shutdown();
}

void StudioClient::shutdown() {
    studioState.instances.clear();
    StudioObjects::shutdown();
}