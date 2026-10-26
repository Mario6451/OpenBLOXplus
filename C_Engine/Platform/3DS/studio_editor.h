#pragma once

#include <vector>
#include <string>
#include <map>
#include <3ds.h>
#include "CoreEngine/Node/nodes.h"

class Script;

// OpenBLOX Studio Client
// Allows editing RBXL place files with Explorer, Properties, and Script Editor

namespace StudioEditor {

    // Property types for the properties panel
    enum class PropertyType {
        String,
        Int,
        Float,
        Bool,
        Vector3,
        Color3,
        BrickColor,
        Enum,
        CFrame
    };

    // A single property in the properties panel
    struct Property {
        std::string name;
        PropertyType type;
        std::string currentValue;
        std::string categoryName;
    };

    // A category of properties (Appearance, Layout, etc.)
    struct PropertyCategory {
        std::string name;
        std::vector<Property> properties;
        bool expanded = true;
    };

    // Explorer tree node
    struct ExplorerNode {
        std::string name;
        std::string className;
        Node* node;           // Actual game node
        int depth;
        bool expanded;
        bool selected;
        std::vector<ExplorerNode> children;
    };

    // Studio UI modes
    enum class StudioMode {
        Edit,          // Explorer + Properties
        ScriptEditor   // Script editing
    };

    // === Core API ===
    
    // Initialize Studio editor
    void init();

    // Load a place file for editing
    bool loadPlace(const std::string& path);

    // Save current place file
    bool savePlace();

    // Save current place file as new path
    bool savePlaceAs(const std::string& path);

    // Enter test mode (runs the game)
    void testPlace();

    // Run Studio main loop
    void runStudio();

    // Get selected node
    Node* getSelectedNode();

    // Set selected node
    void setSelectedNode(Node* node);

    // Get current script source for editing
    std::string getScriptSource(Node* scriptNode);

    // Set script source
    void setScriptSource(Node* scriptNode, const std::string& source);

    // Create new instance (part, model, script, etc.)
    Node* createInstance(const std::string& className);

    // Delete selected instance
    void deleteSelected();

    // Recursively delete a node and all children
    void deleteNodeRecursively(Node* node);

    // Get properties for selected node
    std::vector<PropertyCategory> getProperties(Node* node);

    // Set property value
    void setProperty(Node* node, const std::string& propName, const std::string& value);

    // Shutdown
    void shutdown();

    // === Internal State ===
    Node* dataModel;
    Node* selectedNode;
    std::string currentPlacePath;
    StudioMode currentMode;
    
    // Explorer state
    std::vector<ExplorerNode> explorerTree;
    int explorerScroll;
    int explorerSelectedIndex;
    int explorerVisibleCount;
    
    // Properties state
    int propertiesScroll;
    
    // Script editor state
    std::string scriptBuffer;
    int scriptScroll;
    Script* editingScript;
    
    // Camera state for 3D viewport
    float viewCameraX, viewCameraY, viewCameraZ;
    float viewCameraRX, viewCameraRY;

    // === Classic Roblox Studio Tools (Internal State) ===
    // Tool types for Studio editing
    enum class StudioTool {
        Select,       // Select parts in 3D viewport
        Move,         // Move selected parts
        Rotate,       // Rotate selected parts
        Resize,       // Scale selected parts
        Delete,       // Remove selected parts
        Clone,        // Duplicate selected parts
        Hammer,       // Quick-place blocks
        Dropper,      // Copy properties from part to part
        Group,        // Group selected parts into Model
        Material,     // Change material of selected part
        Color,        // Change color of selected part
        Anchor,       // Anchor/unanchor selected part
        Transparency, // Change transparency of selected part
        Count
    };

    StudioTool activeTool;
    std::vector<Node*> selectedNodes;
    Node* dropperSource;
    std::vector<Node*> groupedNodes;
    bool snapGridEnabled;
    float snapIncrementVal;
    int hammerSize;
    int hammerColor;
    float rotationAngle;
    float resizeScale;

    // Tool icon names (for rendering)
    static const char* getToolIconName(StudioTool tool);
    static const char* getToolTooltip(StudioTool tool);

    // === Classic Roblox Studio Tools API ===

    // Get currently active tool
    StudioTool getActiveTool();

    // Set active tool
    void setActiveTool(StudioTool tool);

    // Handle tool-specific input
    void handleToolInput(float dt);

    // Render tool toolbar on bottom screen
    void renderToolToolbar(float x, float y, float width, float height);

    // Tool options
    bool snapToGrid();
    void setSnapToGrid(bool enabled);
    float snapIncrement();
    void setSnapIncrement(float increment);
    int hammerBrushSize();
    void setHammerBrushSize(int size);

    // Dropper state
    Node* getDropperSource();
    void setDropperSource(Node* node);

    // Group state
    std::vector<Node*> getGroupedNodes();
    void clearGroupedNodes();

    // Selection
    std::vector<Node*>& getSelectedNodes();
    void clearSelection();
    void selectNode(Node* node);
    void addToSelection(Node* node);

    // Internal helpers
    void buildExplorerTree(Node* node, std::vector<ExplorerNode>& nodes, int depth);
    void flattenExplorerTree();
    void flattenExplorerChildren(const ExplorerNode& node);
    void serializeNodeToXML(FILE* f, Node* node, int indent);
    void drawBottomScreenEditUI();
    void drawBottomScreenScriptEditorUI();
    void drawTopScreenViewport();
    void processInput();

};
