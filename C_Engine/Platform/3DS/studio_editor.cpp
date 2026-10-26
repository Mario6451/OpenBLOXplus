#include "studio_editor.h"
#include "../importantIncludes.h"
#include "citro_renderer.h"
#include "font_manager.h"
#include "render_pipeline.h"
#include "asset_loader.h"
#include "settings.h"
#include "content_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;
using namespace StudioEditor;

namespace StudioEditor {

}

// UI layout constants for bottom screen
static const int TOOLBAR_X = 5;

// Stub for Draw::FillRectangle (Draw namespace not available)
namespace Draw { inline void FillRectangle(int x, int y, int w, int h, uint32_t color) {} }

// Bottom screen console
static PrintConsole consoleBottom;

static const int TOOLBAR_Y = 5;
static const int TOOLBAR_W = 50;
static const int TOOLBAR_BTN_H = 28;
static const int EXPLORER_X = 60;
static const int EXPLORER_Y = 5;
static const int EXPLORER_W = 230;
static const int EXPLORER_H = 130;
static const int PROPERTIES_X = 60;
static const int PROPERTIES_Y = 140;
static const int PROPERTIES_W = 230;
static const int PROPERTIES_H = 95;
static const int SCRIPT_EDITOR_X = 60;
static const int SCRIPT_EDITOR_Y = 5;
static const int SCRIPT_EDITOR_W = 330;
static const int SCRIPT_EDITOR_H = 225;

// Flat list for explorer scrolling
static vector<ExplorerNode*> flatExplorerList;

void StudioEditor::init() {
    dataModel = nullptr;
    selectedNode = nullptr;
    currentPlacePath = "";
    currentMode = StudioMode::Edit;
    explorerScroll = 0;
    explorerSelectedIndex = 0;
    propertiesScroll = 0;
    scriptBuffer = "";
    editingScript = nullptr;
    viewCameraX = 0; viewCameraY = -20; viewCameraZ = 20;
    viewCameraRX = -45; viewCameraRY = 0;
    
    // Initialize classic tools
    activeTool = StudioTool::Select;
    selectedNodes.clear();
    dropperSource = nullptr;
    groupedNodes.clear();
    snapGridEnabled = true;
    snapIncrementVal = 0.4f;
    hammerSize = 4;
    hammerColor = 2;
    rotationAngle = 0;
    resizeScale = 1.0f;
    
    print_message("Studio: Initialized\n");
}

bool StudioEditor::loadPlace(const std::string& path) {
    currentPlacePath = path;
    print_message("Studio: Loading place %s\n", path.c_str());
    
    // Load the RBXL file using existing parser
    FILE* f = fopen(path.c_str(), "rb");
    dataModel = nullptr;
    if (f) {
        dataModel = parseRBXL(f);
        fclose(f);
    }
    if (!dataModel) {
        dataModel = new Node();
        strcpy(dataModel->name, "Workspace");
        strcpy(dataModel->type, "Workspace");
    }
    
    // Build explorer tree from dataModel
    explorerTree.clear();
    buildExplorerTree(dataModel, explorerTree, 0);
    flattenExplorerTree();
    
    return true;
}

void StudioEditor::buildExplorerTree(Node* node, std::vector<ExplorerNode>& nodes, int depth) {
    for (Node* child : node->children) {
        ExplorerNode en;
        en.name = child->name;
        en.className = child->type;
        en.node = child;
        en.depth = depth;
        en.expanded = (depth < 2); // Auto-expand top levels
        en.selected = (child == selectedNode);
        
        // Recursively add children if expanded
        if (en.expanded && !child->children.empty()) {
            buildExplorerTree(child, en.children, depth + 1);
        }
        
        nodes.push_back(en);
    }
}

void StudioEditor::flattenExplorerTree() {
    flatExplorerList.clear();
    for (auto& node : explorerTree) {
        flatExplorerList.push_back(&node);
        flattenExplorerChildren(node);
    }
}

void StudioEditor::flattenExplorerChildren(const ExplorerNode& node) {
    for (auto& child : node.children) {
        flatExplorerList.push_back(const_cast<ExplorerNode*>(&child));
        flattenExplorerChildren(child);
    }
}

bool StudioEditor::savePlace() {
    if (currentPlacePath.empty()) {
        return false;
    }
    print_message("Studio: Saving place to %s\n", currentPlacePath.c_str());
    // RBXL serialization via serializeNodeToXML
    FILE* f = fopen(currentPlacePath.c_str(), "wb");
    if (f) {
        // Write basic RBXL header
        fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
        fprintf(f, "<roblox version=\"4\">\n");
        serializeNodeToXML(f, dataModel, 1);
        fprintf(f, "</roblox>\n");
        fclose(f);
        print_message("Studio: Place saved successfully!\n");
        return true;
    }
    print_message("Studio: Failed to save place!\n");
    return false;
}

void StudioEditor::serializeNodeToXML(FILE* f, Node* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) fprintf(f, "  ");
    fprintf(f, "<%s name=\"%s\">\n", node->type, node->name);
    
    // Serialize properties based on type
    if (strcmp(node->type, "Part") == 0) {
        Part* part = static_cast<Part*>(node);
        for (int i = 0; i < indent; i++) fprintf(f, "  ");
        fprintf(f, "<BrickColor>%d</BrickColor>\n", part->color);
        for (int i = 0; i < indent; i++) fprintf(f, "  ");
        fprintf(f, "<Size>%f %f %f</Size>\n", part->scale.x, part->scale.y, part->scale.z);
        for (int i = 0; i < indent; i++) fprintf(f, "  ");
        fprintf(f, "<CFrame>X %f\n", part->cf.X);
    }
    
    if (strcmp(node->type, "Script") == 0 || strcmp(node->type, "LocalScript") == 0) {
        Script* script = static_cast<Script*>(node);
        for (int i = 0; i < indent; i++) fprintf(f, "  ");
        fprintf(f, "<Source><![CDATA[%s]]></Source>\n", script->source ? script->source : "");
    }
    
    // Recursively serialize children
    for (Node* child : node->children) {
        serializeNodeToXML(f, child, indent + 1);
    }
    
    for (int i = 0; i < indent; i++) fprintf(f, "  ");
    fprintf(f, "</%s>\n", node->type);
}

bool StudioEditor::savePlaceAs(const std::string& path) {
    currentPlacePath = path;
    return savePlace();
}

void StudioEditor::testPlace() {
    print_message("Studio: Starting test mode\n");
    savePlace();
    // Transition handled by main.cpp via __TEST_MODE__ return value
}

Node* StudioEditor::getSelectedNode() {
    return selectedNode;
}

void StudioEditor::setSelectedNode(Node* node) {
    selectedNode = node;
    scriptBuffer = "";
    editingScript = nullptr;
}

std::string StudioEditor::getScriptSource(Node* scriptNode) {
    if (!scriptNode) return "";
    if (strcmp(scriptNode->type, "Script") != 0 && strcmp(scriptNode->type, "LocalScript") != 0) {
        return "";
    }
    Script* script = static_cast<Script*>(scriptNode);
    return script->source ? script->source : "";
}

void StudioEditor::setScriptSource(Node* scriptNode, const std::string& source) {
    if (!scriptNode) return;
    Script* script = static_cast<Script*>(scriptNode);
    if (script->source) free(script->source);
    script->source = (char*)malloc(source.size() + 1);
    strcpy(script->source, source.c_str());
}

Node* StudioEditor::createInstance(const std::string& className) {
    if (!dataModel) return nullptr;
    
    Node* newNode = nullptr;
    
    if (className == "Part") {
        Part* part = new Part();
        strcpy(part->name, "Part");
        strcpy(part->type, "Part");
        part->scale.x = 4; part->scale.y = 1; part->scale.z = 4;
        part->cf = CFrame();
        part->cf.X = 0; part->cf.Y = 5; part->cf.Z = 0;
        part->color = 2; // Bright blue
        newNode = part;
    } else if (className == "Model") {
        Model* model = new Model();
        strcpy(model->name, "Model");
        strcpy(model->type, "Model");
        newNode = model;
    } else if (className == "Script") {
        Script* script = new Script();
        strcpy(script->name, "Script");
        strcpy(script->type, "Script");
        script->source = nullptr;
        script->runContext = true;
        newNode = script;
    } else if (className == "LocalScript") {
        Script* script = new Script();
        strcpy(script->name, "LocalScript");
        strcpy(script->type, "LocalScript");
        script->source = nullptr;
        script->runContext = false;
        newNode = script;
    } else if (className == "SpawnLocation") {
        SpawnLocation* spawn = new SpawnLocation();
        strcpy(spawn->name, "SpawnLocation");
        strcpy(spawn->type, "SpawnLocation");
        spawn->scale.x = 4; spawn->scale.y = 1; spawn->scale.z = 4;
        spawn->cf = CFrame();
        spawn->cf.X = 0; spawn->cf.Y = 5; spawn->cf.Z = 0;
        newNode = spawn;
    }
    
    if (newNode) {
        if (selectedNode) {
            newNode->SetParent(selectedNode);
        } else {
            newNode->SetParent(dataModel);
        }
        // Rebuild explorer tree
        explorerTree.clear();
        buildExplorerTree(dataModel, explorerTree, 0);
        flattenExplorerTree();
    }
    
    return newNode;
}

void StudioEditor::deleteSelected() {
    if (!selectedNode || !selectedNode->parent) return;
    
    // Remove from parent's children
    Node* parent = selectedNode->parent;
    for (auto it = parent->children.begin(); it != parent->children.end(); ++it) {
        if (*it == selectedNode) {
            parent->children.erase(it);
            break;
        }
    }
    
    // Delete the node recursively
    deleteNodeRecursively(selectedNode);
    selectedNode = nullptr;
    
    // Rebuild explorer tree
    explorerTree.clear();
    buildExplorerTree(dataModel, explorerTree, 0);
    flattenExplorerTree();
}

void StudioEditor::deleteNodeRecursively(Node* node) {
    if (!node) return;
    for (Node* child : node->children) {
        deleteNodeRecursively(child);
    }
    delete node;
}

std::vector<PropertyCategory> StudioEditor::getProperties(Node* node) {
    std::vector<PropertyCategory> categories;
    if (!node) return categories;
    
    // General properties
    PropertyCategory general;
    general.name = "General";
    
    Property pName;
    pName.name = "Name"; pName.type = PropertyType::String;
    pName.currentValue = node->name; pName.categoryName = "General";
    general.properties.push_back(pName);
    
    Property pClass;
    pClass.name = "ClassName"; pClass.type = PropertyType::String;
    pClass.currentValue = node->type; pClass.categoryName = "General";
    general.properties.push_back(pClass);
    
    Property pAnchored;
    pAnchored.name = "Anchored"; pAnchored.type = PropertyType::Bool;
    pAnchored.currentValue = node->anchored ? "true" : "false";
    pAnchored.categoryName = "General";
    general.properties.push_back(pAnchored);
    
    Property pLocked;
    pLocked.name = "Locked"; pLocked.type = PropertyType::Bool;
    pLocked.currentValue = node->locked ? "true" : "false";
    pLocked.categoryName = "General";
    general.properties.push_back(pLocked);
    
    Property pCanCollide;
    pCanCollide.name = "CanCollide"; pCanCollide.type = PropertyType::Bool;
    pCanCollide.currentValue = node->canCollide ? "true" : "false";
    pCanCollide.categoryName = "General";
    general.properties.push_back(pCanCollide);
    
    Property pVisible;
    pVisible.name = "Visible"; pVisible.type = PropertyType::Bool;
    pVisible.currentValue = node->visible ? "true" : "false";
    pVisible.categoryName = "General";
    general.properties.push_back(pVisible);
    
    categories.push_back(general);
    
    // Part-specific properties
    if (strcmp(node->type, "Part") == 0 || strcmp(node->type, "MeshPart") == 0 || 
        strcmp(node->type, "SpawnLocation") == 0) {
        Part* part = static_cast<Part*>(node);
        
        PropertyCategory appearance;
        appearance.name = "Appearance";
        
        Property pColor;
        pColor.name = "Color"; pColor.type = PropertyType::Int;
        pColor.currentValue = std::to_string(part->color);
        pColor.categoryName = "Appearance";
        appearance.properties.push_back(pColor);
        
        Property pTransparency;
        pTransparency.name = "Transparency"; pTransparency.type = PropertyType::Float;
        pTransparency.currentValue = std::to_string(part->transparency);
        pTransparency.categoryName = "Appearance";
        appearance.properties.push_back(pTransparency);
        
        categories.push_back(appearance);
        
        PropertyCategory layout;
        layout.name = "Layout";
        
        Property pSize;
        pSize.name = "Size"; pSize.type = PropertyType::Vector3;
        pSize.currentValue = std::to_string(part->scale.x) + " " + std::to_string(part->scale.y) + " " + std::to_string(part->scale.z);
        pSize.categoryName = "Layout";
        layout.properties.push_back(pSize);
        
        Property pPosition;
        pPosition.name = "Position"; pPosition.type = PropertyType::Vector3;
        pPosition.currentValue = std::to_string(part->cf.X) + " " + std::to_string(part->cf.Y) + " " + std::to_string(part->cf.Z);
        pPosition.categoryName = "Layout";
        layout.properties.push_back(pPosition);
        
        categories.push_back(layout);
    }
    
    // Script-specific properties
    if (strcmp(node->type, "Script") == 0 || strcmp(node->type, "LocalScript") == 0) {
        Script* script = static_cast<Script*>(node);
        
        PropertyCategory scriptCat;
        scriptCat.name = "Script";
        
        Property pSource;
        pSource.name = "Source"; pSource.type = PropertyType::String;
        pSource.currentValue = script->source ? script->source : "";
        pSource.categoryName = "Script";
        scriptCat.properties.push_back(pSource);
        
        Property pRunContext;
        pRunContext.name = "RunContext"; pRunContext.type = PropertyType::Bool;
        pRunContext.currentValue = script->runContext ? "Server" : "Client";
        pRunContext.categoryName = "Script";
        scriptCat.properties.push_back(pRunContext);
        
        categories.push_back(scriptCat);
    }
    
    return categories;
}

void StudioEditor::setProperty(Node* node, const std::string& propName, const std::string& value) {
    if (!node) return;
    
    if (propName == "Name") {
        strncpy(node->name, value.c_str(), sizeof(node->name) - 1);
        node->name[sizeof(node->name) - 1] = '\0';
    } else if (propName == "Anchored") {
        node->anchored = (value == "true");
    } else if (propName == "Locked") {
        node->locked = (value == "true");
    } else if (propName == "CanCollide") {
        node->canCollide = (value == "true");
    } else if (propName == "Visible") {
        node->visible = (value == "true");
    } else if (strcmp(node->type, "Part") == 0 || strcmp(node->type, "MeshPart") == 0 || 
               strcmp(node->type, "SpawnLocation") == 0) {
        Part* part = static_cast<Part*>(node);
        if (propName == "Color") {
            part->color = std::stoi(value);
        } else if (propName == "Transparency") {
            part->transparency = std::stof(value);
        } else if (propName == "Size") {
            // Parse "x y z" format
            sscanf(value.c_str(), "%f %f %f", &part->scale.x, &part->scale.y, &part->scale.z);
        } else if (propName == "Position") {
            sscanf(value.c_str(), "%f %f %f", &part->cf.X, &part->cf.Y, &part->cf.Z);
        }
    } else if (strcmp(node->type, "Script") == 0 || strcmp(node->type, "LocalScript") == 0) {
        Script* script = static_cast<Script*>(node);
        if (propName == "RunContext") {
            script->runContext = (value == "Server");
        }
    }
}

void StudioEditor::drawBottomScreenEditUI() {
    consoleSelect(&consoleBottom);
    consoleClear();
    
    // Draw toolbar
    uint32_t toolbarColor = 0x333333FF;
    Draw::FillRectangle(TOOLBAR_X, TOOLBAR_Y, TOOLBAR_W, 240, toolbarColor);
    
    // Draw toolbar buttons
    const char* buttons[] = {"exit", "save", "load", "test"};
    for (int i = 0; i < 4; i++) {
        int btnY = TOOLBAR_Y + 5 + i * TOOLBAR_BTN_H;
        uint32_t btnColor = 0x555555FF;
        Draw::FillRectangle(TOOLBAR_X + 2, btnY, TOOLBAR_W - 4, TOOLBAR_BTN_H - 2, btnColor);
        
        FontManager::drawText(buttons[i], TOOLBAR_X + 8, btnY + 10, 255, 255, 255, FontSize::Size12);
    }
    
    // Draw explorer panel
    uint32_t explorerBg = 0x222222FF;
    Draw::FillRectangle(EXPLORER_X, EXPLORER_Y, EXPLORER_W, EXPLORER_H, explorerBg);
    
    // Explorer title
    uint32_t titleBg = 0x444444FF;
    Draw::FillRectangle(EXPLORER_X, EXPLORER_Y, EXPLORER_W, 20, titleBg);
    FontManager::drawText("explorer", EXPLORER_X + 5, EXPLORER_Y + 5, 255, 255, 255, FontSize::Size12);
    
    // Explorer content with scrollbar
    int contentY = EXPLORER_Y + 22;
    int lineH = 16;
    int visible = (EXPLORER_H - 22) / lineH;
    
    for (int i = 0; i < visible && (explorerScroll + i) < (int)flatExplorerList.size(); i++) {
        int idx = explorerScroll + i;
        ExplorerNode* en = flatExplorerList[idx];
        if (!en) continue;
        
        // Draw selection highlight
        if (idx == explorerSelectedIndex) {
            uint32_t selColor = 0x0000AA88;
            Draw::FillRectangle(EXPLORER_X + 2, contentY + i * lineH, EXPLORER_W - 4, lineH, selColor);
        }
        
        // Draw indentation
        int xOff = en->depth * 10;
        std::string line = std::string(en->depth, ' ') + en->name;
        
        uint32_t textColor = (en->className == "Script" || en->className == "LocalScript") ? 
            0x44FF44FF : 0xFFFFFFFF;
        FontManager::drawText(line.c_str(), EXPLORER_X + 3 + xOff, contentY + i * lineH + 3, 
                             (idx == explorerSelectedIndex) ? 255 : 200, 
                             (idx == explorerSelectedIndex) ? 255 : 200, 
                             (idx == explorerSelectedIndex) ? 255 : 200, FontSize::Size12);
    }
    
    // Draw scrollbar if needed
    if ((int)flatExplorerList.size() > visible) {
        int barX = EXPLORER_X + EXPLORER_W - 8;
        int barW = 6;
        int barH = std::max(20, (visible * EXPLORER_H) / (int)flatExplorerList.size());
        int barY = EXPLORER_Y + 22 + ((explorerScroll * (EXPLORER_H - 22 - barH)) / 
                      std::max(1, (int)flatExplorerList.size() - visible));
        Draw::FillRectangle(barX, barY, barW, barH, 0x888888FF);
    }
    
    // Draw properties panel
    uint32_t propsBg = 0x1A1A1AFF;
    Draw::FillRectangle(PROPERTIES_X, PROPERTIES_Y, PROPERTIES_W, PROPERTIES_H, propsBg);
    
    // Properties title
    Draw::FillRectangle(PROPERTIES_X, PROPERTIES_Y, PROPERTIES_W, 20, titleBg);
    FontManager::drawText("properties with scrollbar", PROPERTIES_X + 5, PROPERTIES_Y + 5, 255, 255, 255, FontSize::Size12);
    
    // Properties content
    if (selectedNode) {
        auto categories = getProperties(selectedNode);
        int propY = PROPERTIES_Y + 24;
        int propLineH = 14;
        
        for (auto& cat : categories) {
            int catVisible = (PROPERTIES_H - 24) / propLineH;
            int drawn = 0;
            
            for (auto& prop : cat.properties) {
                if (propertiesScroll + drawn >= 0 && drawn < catVisible) {
                    // Draw property row
                    std::string line = prop.name + ": " + prop.currentValue;
                    // Truncate if too long
                    if (line.length() > 30) line = line.substr(0, 27) + "...";
                    
                    FontManager::drawText(line.c_str(), PROPERTIES_X + 3, propY + (propertiesScroll + drawn) * propLineH, 
                                         200, 200, 200, FontSize::Size12);
                }
                drawn++;
            }
        }
        
        // Properties scrollbar
        int totalProps = 0;
        for (auto& cat : categories) totalProps += cat.properties.size();
        if (totalProps > (PROPERTIES_H - 24) / propLineH) {
            int barX = PROPERTIES_X + PROPERTIES_W - 8;
            int barH = std::max(15, ((PROPERTIES_H - 24) * (PROPERTIES_H - 24)) / (totalProps * propLineH));
            int barY = PROPERTIES_Y + 22 + ((propertiesScroll * (PROPERTIES_H - 22 - barH)) / 
                          std::max(1, totalProps - (PROPERTIES_H - 24) / propLineH));
            Draw::FillRectangle(barX, barY, 6, barH, 0x888888FF);
        }
    } else {
        FontManager::drawText("(no selection)", PROPERTIES_X + 5, PROPERTIES_Y + 30, 100, 100, 100, FontSize::Size12);
    }
    
    // Draw classic tool toolbar at bottom of screen
    renderToolToolbar(TOOLBAR_X + TOOLBAR_W, 272 - 32, 256 - TOOLBAR_W, 32);
    
    // Draw active tool indicator
    const char* toolName = getToolTooltip(activeTool);
    FontManager::drawText(toolName, 160, 273 - 35, 255, 255, 100, FontSize::Size12);
    
    // Draw snap indicator
    if (snapGridEnabled) {
        FontManager::drawText("SNAP", 240, 273 - 35, 100, 255, 100, FontSize::Size12);
    }
}

void StudioEditor::drawBottomScreenScriptEditorUI() {
    consoleSelect(&consoleBottom);
    consoleClear();
    
    // Draw toolbar
    uint32_t toolbarColor = 0x333333FF;
    Draw::FillRectangle(TOOLBAR_X, TOOLBAR_Y, TOOLBAR_W, 240, toolbarColor);
    
    // Draw toolbar buttons
    const char* buttons[] = {"exit", "save"};
    for (int i = 0; i < 2; i++) {
        int btnY = TOOLBAR_Y + 5 + i * TOOLBAR_BTN_H;
        uint32_t btnColor = 0x555555FF;
        Draw::FillRectangle(TOOLBAR_X + 2, btnY, TOOLBAR_W - 4, TOOLBAR_BTN_H - 2, btnColor);
        
        FontManager::drawText(buttons[i], TOOLBAR_X + 8, btnY + 10, 255, 255, 255, FontSize::Size12);
    }
    
    // Draw script editor area
    uint32_t editorBg = 0x1E1E1EFF;
    Draw::FillRectangle(SCRIPT_EDITOR_X, SCRIPT_EDITOR_Y, SCRIPT_EDITOR_W, SCRIPT_EDITOR_H, editorBg);
    
    // Script title bar
    uint32_t titleBg = 0x444444FF;
    Draw::FillRectangle(SCRIPT_EDITOR_X, SCRIPT_EDITOR_Y, SCRIPT_EDITOR_W, 20, titleBg);
    
    std::string title = "script editor here";
    if (editingScript) {
        title = "Script: " + std::string(editingScript->name);
    }
    FontManager::drawText(title.c_str(), SCRIPT_EDITOR_X + 5, SCRIPT_EDITOR_Y + 5, 255, 255, 255, FontSize::Size12);
    
    // Script content with line numbers
    int contentY = SCRIPT_EDITOR_Y + 22;
    int lineH = 14;
    int visible = (SCRIPT_EDITOR_H - 22) / lineH;
    
    // Split scriptBuffer into lines
    std::vector<std::string> lines;
    std::string line;
    for (char c : scriptBuffer) {
        if (c == '\n') {
            lines.push_back(line);
            line = "";
        } else {
            line += c;
        }
    }
    if (!line.empty() || scriptBuffer.empty()) lines.push_back(line);
    
    // Draw lines
    for (int i = 0; i < visible && (scriptScroll + i) < (int)lines.size(); i++) {
        int idx = scriptScroll + i;
        // Line number
        std::string lineNum = std::to_string(idx + 1) + ": ";
        FontManager::drawText(lineNum.c_str(), SCRIPT_EDITOR_X + 3, contentY + i * lineH, 100, 100, 100, FontSize::Size12);
        
        // Line content
        std::string content = lines[idx];
        if (content.length() > 40) content = content.substr(0, 37) + "...";
        FontManager::drawText(content.c_str(), SCRIPT_EDITOR_X + 45, contentY + i * lineH, 200, 255, 200, FontSize::Size12);
    }
    
    // Scrollbar
    if ((int)lines.size() > visible) {
        int barX = SCRIPT_EDITOR_X + SCRIPT_EDITOR_W - 8;
        int barW = 6;
        int barH = std::max(15, (visible * (SCRIPT_EDITOR_H - 22)) / (int)lines.size());
        int barY = SCRIPT_EDITOR_Y + 22 + ((scriptScroll * (SCRIPT_EDITOR_H - 22 - barH)) /
                      std::max(1, (int)lines.size() - visible));
        Draw::FillRectangle(barX, barY, barW, barH, 0x888888FF);
    }
}

void StudioEditor::drawTopScreenViewport() {
    // Draw the 3D viewport
    start3DFrame();
    
    // Clear with dark background
    CitroRenderer::clearScreen(0.1f, 0.1f, 0.15f, 1.0f);
    
    // Set up camera
    perspectiveModeGL();
    
    // Apply camera transforms
    scale_rotate_translate(1, 1, 1, viewCameraRX, viewCameraRY, 0, 0, 0, 0);
    scale_rotate_translate(1, 1, 1, 0, 0, 0, viewCameraX, viewCameraY, viewCameraZ);
    
    // Render data model
    if (dataModel) {
        renderWorld(dataModel);
    }
    
    // Draw selection indicator
    if (selectedNode && (strcmp(selectedNode->type, "Part") == 0 || 
        strcmp(selectedNode->type, "MeshPart") == 0 || strcmp(selectedNode->type, "SpawnLocation") == 0)) {
        Part* part = static_cast<Part*>(selectedNode);
        // Draw wireframe box around selected part
        CitroRenderer::setColor(1.0f, 1.0f, 0.0f);
        CitroRenderer::drawBox(part->cf.X, part->cf.Y, part->cf.Z, 
            part->scale.x, part->scale.y, part->scale.z, 1.0f, 1.0f, 0.0f);
    }
    
    endFrame();
}

void StudioEditor::processInput() {
    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    
    // Handle toolbar button clicks via touch
    touchPosition touch;
    if (kDown & KEY_TOUCH) {
        touchRead(&touch);
        
        // Check toolbar buttons
        if (touch.px >= TOOLBAR_X && touch.px <= TOOLBAR_X + TOOLBAR_W) {
            if (currentMode == StudioMode::Edit) {
                // Edit mode buttons
                if (touch.py >= TOOLBAR_Y + 5 && touch.py <= TOOLBAR_Y + 5 + TOOLBAR_BTN_H) {
                    // Exit
                    print_message("Studio: Exiting\n");
                    return;
                } else if (touch.py >= TOOLBAR_Y + 5 + TOOLBAR_BTN_H && 
                           touch.py <= TOOLBAR_Y + 5 + 2 * TOOLBAR_BTN_H) {
                    // Save
                    savePlace();
                } else if (touch.py >= TOOLBAR_Y + 5 + 2 * TOOLBAR_BTN_H && 
                           touch.py <= TOOLBAR_Y + 5 + 3 * TOOLBAR_BTN_H) {
                    // Load - scan for .rbxl files
                    DIR* dir = opendir("sdmc:/OpenBLOX/");
                    if (dir) {
                        struct dirent* ent;
                        while ((ent = readdir(dir)) != nullptr) {
                            if (strstr(ent->d_name, ".rbxl")) {
                                loadPlace(std::string("sdmc:/OpenBLOX/") + ent->d_name);
                                goto load_done;
                            }
                        }
                        closedir(dir);
                    }
                    load_done:;
                } else if (touch.py >= TOOLBAR_Y + 5 + 3 * TOOLBAR_BTN_H && 
                           touch.py <= TOOLBAR_Y + 5 + 4 * TOOLBAR_BTN_H) {
                    // Test
                    testPlace();
                }
            } else {
                // Script editor buttons
                if (touch.py >= TOOLBAR_Y + 5 && touch.py <= TOOLBAR_Y + 5 + TOOLBAR_BTN_H) {
                    // Exit script editor
                    currentMode = StudioMode::Edit;
                } else if (touch.py >= TOOLBAR_Y + 5 + TOOLBAR_BTN_H && 
                           touch.py <= TOOLBAR_Y + 5 + 2 * TOOLBAR_BTN_H) {
                    // Save script
                    if (editingScript) {
                        setScriptSource(editingScript, scriptBuffer);
                        print_message("Studio: Script saved\n");
                    }
                }
            }
        }
        
        // Check explorer clicks
        if (currentMode == StudioMode::Edit && 
            touch.px >= EXPLORER_X && touch.px <= EXPLORER_X + EXPLORER_W &&
            touch.py >= EXPLORER_Y + 22 && touch.py <= EXPLORER_Y + EXPLORER_H) {
            int line = (touch.py - EXPLORER_Y - 22) / 16;
            int idx = explorerScroll + line;
            if (idx >= 0 && idx < (int)flatExplorerList.size()) {
                explorerSelectedIndex = idx;
                ExplorerNode* en = flatExplorerList[idx];
                if (en && en->node) {
                    selectedNode = en->node;
                    
                    // If it's a script, switch to script editor
                    if (strcmp(en->node->type, "Script") == 0 || 
                        strcmp(en->node->type, "LocalScript") == 0) {
                        editingScript = static_cast<Script*>(en->node);
                        scriptBuffer = getScriptSource(en->node);
                        currentMode = StudioMode::ScriptEditor;
                    }
                }
            }
        }
    }
    
    // Handle D-pad navigation in explorer
    if (currentMode == StudioMode::Edit) {
        if (kDown & KEY_UP && explorerSelectedIndex > 0) {
            explorerSelectedIndex--;
            if (explorerSelectedIndex < explorerScroll) explorerScroll = explorerSelectedIndex;
        }
        if (kDown & KEY_DOWN && explorerSelectedIndex < (int)flatExplorerList.size() - 1) {
            explorerSelectedIndex++;
            int visible = (EXPLORER_H - 22) / 16;
            if (explorerSelectedIndex >= explorerScroll + visible) 
                explorerScroll = explorerSelectedIndex - visible + 1;
        }
        if (kDown & KEY_RIGHT && explorerSelectedIndex < (int)flatExplorerList.size() - 1) {
            explorerSelectedIndex++;
            ExplorerNode* en = flatExplorerList[explorerSelectedIndex];
            if (en) selectedNode = en->node;
        }
        if (kDown & KEY_LEFT && explorerSelectedIndex > 0) {
            explorerSelectedIndex--;
            ExplorerNode* en = flatExplorerList[explorerSelectedIndex];
            if (en) selectedNode = en->node;
        }
        
        // Scroll explorer with L/R
        if (kHeld & KEY_L) {
            explorerScroll = std::max(0, explorerScroll - 1);
            if (explorerSelectedIndex >= 0 && explorerSelectedIndex < (int)flatExplorerList.size()) {
                ExplorerNode* en = flatExplorerList[explorerSelectedIndex];
                if (en) selectedNode = en->node;
            }
        }
        if (kHeld & KEY_R) {
            explorerScroll = std::min((int)flatExplorerList.size() - (EXPLORER_H - 22) / 16, explorerScroll + 1);
            if (explorerSelectedIndex >= 0 && explorerSelectedIndex < (int)flatExplorerList.size()) {
                ExplorerNode* en = flatExplorerList[explorerSelectedIndex];
                if (en) selectedNode = en->node;
            }
        }
        
        // A button to confirm selection / open script editor
        if (kDown & KEY_A && selectedNode) {
            if (strcmp(selectedNode->type, "Script") == 0 || strcmp(selectedNode->type, "LocalScript") == 0) {
                editingScript = static_cast<Script*>(selectedNode);
                scriptBuffer = getScriptSource(selectedNode);
                currentMode = StudioMode::ScriptEditor;
            }
        }
        
        // B button to deselect
        if (kDown & KEY_B) {
            selectedNode = nullptr;
        }
        
        // X button to create Part
        if (kDown & KEY_X) {
            createInstance("Part");
        }
        
        // Y button to delete selected
        if (kDown & KEY_Y && selectedNode) {
            deleteSelected();
        }
        
        // Scroll properties with L/R
        if (kHeld & KEY_L) {
            propertiesScroll = std::max(0, propertiesScroll - 1);
        }
        if (kHeld & KEY_R) {
            propertiesScroll = std::max(0, propertiesScroll + 1);
        }
        
        // Handle classic tools input (always active in edit mode)
        handleToolInput(1.0f / 60.0f);
        
        // Tool toolbar touch (bottom row of bottom screen)
        if (kDown & KEY_TOUCH) {
            touchRead(&touch);
            int toolbarY = 272; // Bottom of bottom screen
            int numTools = (int)StudioTool::Count;
            float toolW = 256.0f / numTools;
            if (touch.py >= toolbarY - 32 && touch.py <= toolbarY) {
                int toolIdx = (int)(touch.px / toolW);
                if (toolIdx >= 0 && toolIdx < numTools) {
                    setActiveTool((StudioTool)toolIdx);
                }
            }
        }
    } else {
        // Script editor mode
        if (kDown & KEY_B) {
            // Save and exit to edit mode
            if (editingScript) {
                setScriptSource(editingScript, scriptBuffer);
            }
            currentMode = StudioMode::Edit;
        }
        
        // Scroll script editor with L/R
        if (kHeld & KEY_L) {
            scriptScroll = std::max(0, scriptScroll - 1);
        }
        if (kHeld & KEY_R) {
            scriptScroll = std::max(0, scriptScroll + 1);
        }
        
        // Scroll script editor with touch drag
        static int lastTouchY = 0;
        if (kHeld & KEY_TOUCH) {
            touchRead(&touch);
            if (touch.py >= SCRIPT_EDITOR_Y + 22 && touch.py <= SCRIPT_EDITOR_Y + SCRIPT_EDITOR_H) {
                int delta = (lastTouchY - touch.py) / 14;
                if (delta != 0) {
                    scriptScroll = std::max(0, scriptScroll + delta);
                }
                lastTouchY = touch.py;
            }
        }
        if (!(kHeld & KEY_TOUCH)) lastTouchY = 0;
    }
    
    // Circlepad camera control
    circlePosition pos;
    circleRead(&pos);
    if (kHeld || pos.dx || pos.dy) {
        viewCameraRY += (float)pos.dy * 2.0f;
        viewCameraRX -= (float)pos.dx * 2.0f;
    }
    
    // Clamp camera pitch
    viewCameraRX = std::max(-89.0f, std::min(89.0f, viewCameraRX));
}

void StudioEditor::runStudio() {
    init();
    
    bool running = true;
    
    while (running) {
        // Draw top screen 3D viewport
        drawTopScreenViewport();
        
        // Draw bottom screen UI based on mode
        if (currentMode == StudioMode::Edit) {
            drawBottomScreenEditUI();
        } else {
            drawBottomScreenScriptEditorUI();
        }
        
        // Process input
        processInput();
        
        // Check for exit (A button on toolbar or START button)
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            running = false;
        }
        
        // Wait for next frame
        svcSleepThread(10000000ULL); // 10ms
    }
    
    // Save on exit
    if (!currentPlacePath.empty()) {
        savePlace();
    }
}

void StudioEditor::shutdown() {
    dataModel = nullptr;
    selectedNode = nullptr;
    explorerTree.clear();
    flatExplorerList.clear();
    editingScript = nullptr;
}

// ============================================================================
// Classic Roblox Studio Tools Implementation
// ============================================================================

const char* StudioEditor::getToolIconName(StudioTool tool) {
    switch (tool) {
        case StudioTool::Select:       return "select";
        case StudioTool::Move:         return "move";
        case StudioTool::Rotate:       return "rotate";
        case StudioTool::Resize:       return "resize";
        case StudioTool::Delete:       return "delete";
        case StudioTool::Clone:        return "clone";
        case StudioTool::Hammer:       return "hammer";
        case StudioTool::Dropper:      return "dropper";
        case StudioTool::Group:        return "group";
        case StudioTool::Material:     return "material";
        case StudioTool::Color:        return "color";
        case StudioTool::Anchor:       return "anchor";
        case StudioTool::Transparency: return "transparency";
        default:                       return "unknown";
    }
}

const char* StudioEditor::getToolTooltip(StudioTool tool) {
    switch (tool) {
        case StudioTool::Select:       return "Select (Parts)";
        case StudioTool::Move:         return "Move (Position)";
        case StudioTool::Rotate:       return "Rotate (Angle)";
        case StudioTool::Resize:       return "Resize (Scale)";
        case StudioTool::Delete:       return "Delete (Remove)";
        case StudioTool::Clone:        return "Clone (Duplicate)";
        case StudioTool::Hammer:       return "Hammer (Place)";
        case StudioTool::Dropper:      return "Dropper (Copy Props)";
        case StudioTool::Group:        return "Group (Combine)";
        case StudioTool::Material:     return "Material (Texture)";
        case StudioTool::Color:        return "Color (Paint)";
        case StudioTool::Anchor:       return "Anchor (Lock)";
        case StudioTool::Transparency: return "Transparency (Alpha)";
        default:                       return "Unknown Tool";
    }
}

StudioTool StudioEditor::getActiveTool() {
    return activeTool;
}

void StudioEditor::setActiveTool(StudioTool tool) {
    activeTool = tool;
    print_message("Studio: Tool changed to %s\n", getToolTooltip(tool));
}

bool StudioEditor::snapToGrid() { return snapGridEnabled; }
void StudioEditor::setSnapToGrid(bool enabled) { snapGridEnabled = enabled; }
float StudioEditor::snapIncrement() { return snapIncrementVal; }
void StudioEditor::setSnapIncrement(float increment) { snapIncrementVal = increment; }
int StudioEditor::hammerBrushSize() { return hammerSize; }
void StudioEditor::setHammerBrushSize(int size) { hammerSize = size; }

Node* StudioEditor::getDropperSource() { return dropperSource; }
void StudioEditor::setDropperSource(Node* node) { dropperSource = node; }

std::vector<Node*>& StudioEditor::getSelectedNodes() { return selectedNodes; }
void StudioEditor::clearSelection() { selectedNodes.clear(); }
void StudioEditor::selectNode(Node* node) { selectedNodes.clear(); if (node) selectedNodes.push_back(node); }
void StudioEditor::addToSelection(Node* node) {
    if (node) {
        bool exists = false;
        for (Node* n : selectedNodes) {
            if (n == node) { exists = true; break; }
        }
        if (!exists) selectedNodes.push_back(node);
    }
}

std::vector<Node*> StudioEditor::getGroupedNodes() { return groupedNodes; }
void StudioEditor::clearGroupedNodes() { groupedNodes.clear(); }

void StudioEditor::handleToolInput(float dt) {
    using namespace std;
    using std::begin;
    using std::end;
    using Vector = vector<Node*>;
    if (!dataModel) return;
    Vector& selNodes = selectedNodes;
    
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    circlePosition pos;
    circleRead(&pos);
    float moveSpeed = 2.0f;
    float rotateSpeed = 45.0f;
    float resizeSpeed = 0.5f;
    
    switch (activeTool) {
        case StudioTool::Select:
            viewCameraRY += (float)pos.dy * 2.0f;
            viewCameraRX -= (float)pos.dx * 2.0f;
            viewCameraRX = std::max(-89.0f, std::min(89.0f, viewCameraRX));
            break;
            
        case StudioTool::Move: {
            if (selNodes.empty()) return;
            float dmovex = (float)pos.dy * moveSpeed * dt;
            float dmovey = (float)pos.dx * moveSpeed * dt;
            float dz = 0.0f;
            if (kHeld & KEY_L) dz -= moveSpeed * dt;
            if (kHeld & KEY_R) dz += moveSpeed * dt;
            
            for (size_t i = 0; i < selNodes.size(); i++) {
                Node* node = selNodes[i];
                if (!node) continue;
                if (strcmp(node->type, "Part") == 0) {
                    Part* part = static_cast<Part*>(node);
                    part->cf.X += dmovex;
                    part->cf.Y += dmovey;
                    part->cf.Z += dz;
                }
            }
            break;
        }
        
        case StudioTool::Rotate: {
            if (selNodes.empty()) return;
            float angle = (float)pos.dy * rotateSpeed * dt;
            
            for (size_t i = 0; i < selNodes.size(); i++) {
                Node* node = selNodes[i];
                if (!node) continue;
                if (strcmp(node->type, "Part") == 0) {
                    Part* part = static_cast<Part*>(node);
                    // Rotate around Y axis - apply rotation matrix to CFrame
                    float ca = cosf(angle), sa = sinf(angle);
                    CFrame old = part->cf;
                    // Pre-multiply by Y-axis rotation matrix
                    part->cf.R00 = ca * old.R00 + sa * old.R10;
                    part->cf.R01 = ca * old.R01 + sa * old.R11;
                    part->cf.R02 = ca * old.R02 + sa * old.R12;
                    part->cf.R10 = -sa * old.R00 + ca * old.R10;
                    part->cf.R11 = -sa * old.R01 + ca * old.R11;
                    part->cf.R12 = -sa * old.R02 + ca * old.R12;
                    // Z row unchanged for Y-axis rotation
                }
            }
            break;
        }
        
        case StudioTool::Resize: {
            if (selNodes.empty()) return;
            float scale = 1.0f + (float)pos.dy * resizeSpeed * dt;
            
            for (size_t i = 0; i < selNodes.size(); i++) {
                Node* node = selNodes[i];
                if (!node) continue;
                if (strcmp(node->type, "Part") == 0) {
                    Part* part = static_cast<Part*>(node);
                    part->scale.x *= scale;
                    part->scale.y *= scale;
                    part->scale.z *= scale;
                }
            }
            break;
        }
        
        case StudioTool::Delete:
            if (kDown & KEY_A) {
                deleteSelected();
                clearSelection();
            }
            break;
            
        case StudioTool::Clone: {
            if (kDown & KEY_A && !selNodes.empty()) {
                for (size_t i = 0; i < selNodes.size(); i++) {
                    Node* orig = selNodes[i];
                    if (!orig) continue;
                    if (strcmp(orig->type, "Part") == 0) {
                        Part* part = static_cast<Part*>(orig);
                        Part* clone = new Part();
                        strcpy(clone->name, "Clone_");
                        strcat(clone->name, part->name);
                        strcpy(clone->type, "Part");
                        clone->scale = part->scale;
                        clone->cf = part->cf;
                        clone->cf.X += 2.0f;
                        clone->color = part->color;
                        if (selectedNode) {
                            clone->SetParent(selectedNode);
                        } else {
                            clone->SetParent(dataModel);
                        }
                        selNodes.push_back(clone);
                    }
                }
                explorerTree.clear();
                buildExplorerTree(dataModel, explorerTree, 0);
                flattenExplorerTree();
            }
            break;
        }
        
        case StudioTool::Hammer: {
            if (kDown & KEY_A) {
                Part* part = new Part();
                strcpy(part->name, "HammerPart");
                strcpy(part->type, "Part");
                part->scale.x = (float)hammerSize;
                part->scale.y = (float)hammerSize;
                part->scale.z = (float)hammerSize;
                part->cf = CFrame();
                part->cf.X = viewCameraX;
                part->cf.Y = viewCameraY;
                part->cf.Z = viewCameraZ;
                part->color = hammerColor;
                part->SetParent(dataModel);
                
                explorerTree.clear();
                buildExplorerTree(dataModel, explorerTree, 0);
                flattenExplorerTree();
            }
            break;
        }
        
        case StudioTool::Dropper: {
            if (kDown & KEY_A) {
                if (selectedNode && strcmp(selectedNode->type, "Part") == 0) {
                    Part* src = static_cast<Part*>(selectedNode);
                    Part* dst = nullptr;
                    for (Node* n : selectedNodes) {
                        if (n && n != selectedNode && strcmp(n->type, "Part") == 0) {
                            dst = static_cast<Part*>(n);
                            break;
                        }
                    }
                    if (dst) {
                        dst->scale = src->scale;
                        dst->cf = src->cf;
                        dst->color = src->color;
                    }
                }
            }
            break;
        }
        
        case StudioTool::Group: {
            if (kDown & KEY_A && !selectedNodes.empty()) {
                Model* model = new Model();
                strcpy(model->name, "GroupModel");
                strcpy(model->type, "Model");
                model->SetParent(dataModel);
                
                for (Node* node : selectedNodes) {
                    if (!node) continue;
                    Node* parent = node->parent;
                    for (auto it = parent->children.begin(); it != parent->children.end(); ++it) {
                        if (*it == node) {
                            parent->children.erase(it);
                            break;
                        }
                    }
                    node->SetParent(model);
                }
                
                groupedNodes = selectedNodes;
                selectedNodes.clear();
                selectedNodes.push_back(model);
                selectedNode = model;
                
                explorerTree.clear();
                buildExplorerTree(dataModel, explorerTree, 0);
                flattenExplorerTree();
            }
            break;
        }
        
        case StudioTool::Material:
            if (kDown & KEY_A) {
                for (size_t i = 0; i < selNodes.size(); i++) {
                    Node* node = selNodes[i];
                    if (node && strcmp(node->type, "Part") == 0) {
                        Part* part = static_cast<Part*>(node);
                        part->color = (part->color % 255) + 1;
                        print_message("Studio: Changing material of %s to color %d\n", part->name, part->color);
                    }
                }
            }
            break;
            
        case StudioTool::Color:
            if (kDown & KEY_A) {
                for (size_t i = 0; i < selNodes.size(); i++) {
                    Node* node = selNodes[i];
                    if (node && strcmp(node->type, "Part") == 0) {
                        Part* part = static_cast<Part*>(node);
                        part->color = (part->color + 1) % 100;
                    }
                }
            }
            break;
            
        case StudioTool::Anchor:
            if (kDown & KEY_A) {
                for (size_t i = 0; i < selNodes.size(); i++) {
                    Node* node = selNodes[i];
                    if (node && strcmp(node->type, "Part") == 0) {
                        Part* part = static_cast<Part*>(node);
                        part->anchored = !part->anchored;
                        print_message("Studio: %s is now %s\n", part->name, part->anchored ? "anchored" : "unanchored");
                    }
                }
            }
            break;
            
        case StudioTool::Transparency:
            if (kDown & KEY_A) {
                for (size_t i = 0; i < selNodes.size(); i++) {
                    Node* node = selNodes[i];
                    if (node && strcmp(node->type, "Part") == 0) {
                        Part* part = static_cast<Part*>(node);
                        part->transparency += 0.1f;
                        if (part->transparency > 1.0f) part->transparency = 0.0f;
                    }
                }
            }
            if (kDown & KEY_B) {
                for (size_t i = 0; i < selNodes.size(); i++) {
                    Node* node = selNodes[i];
                    if (node && strcmp(node->type, "Part") == 0) {
                        Part* part = static_cast<Part*>(node);
                        part->transparency -= 0.1f;
                        if (part->transparency < 0.0f) part->transparency = 1.0f;
                    }
                }
            }
            break;
        default:
            break;
    }
}

void StudioEditor::renderToolToolbar(float x, float y, float width, float height) {
    // Render horizontal toolbar with tool icons
    int numTools = (int)StudioTool::Count;
    float toolW = width / numTools;
    float toolH = height;
    
    // Background
    RenderPipeline::drawRect(x, y, width, height, 0.15f, 0.15f, 0.15f);
    
    for (int i = 0; i < numTools; i++) {
        StudioTool tool = (StudioTool)i;
        float tx = x + i * toolW;
        
        // Tool button background
        if (tool == activeTool) {
            // Active tool highlight
            RenderPipeline::drawRect(tx + 1, y + 1, toolW - 2, toolH - 2, 0.3f, 0.5f, 0.8f);
        } else {
            RenderPipeline::drawRect(tx + 1, y + 1, toolW - 2, toolH - 2, 0.25f, 0.25f, 0.25f);
        }
        
        // Tool icon text - label area
    }
    
    // Tool name display
    // Show active tool name on screen
}
