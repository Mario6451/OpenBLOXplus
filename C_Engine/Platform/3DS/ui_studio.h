#pragma once

#include <3ds/types.h>
#include <3ds/gfx.h>
#include <string>
#include <vector>

class Node;
class Part;

namespace StudioUI {
    // Explorer tree node
    struct ExplorerItem {
        Node* node;
        std::string name;
        std::string type;
        int depth;
        bool expanded;
        bool selected;
    };

    // Property entry
    struct PropertyEntry {
        std::string name;
        std::string type;
        bool isEditable;
    };

    // Studio UI state
    enum class StudioMode {
        Editing,
        PlayTesting,
        ScriptEditor,
        Toolbox       // Toolbox panel for fetching assets from proxy
    };

    // Toolbox asset entry
    struct ToolboxAsset {
        std::string name;
        std::string assetId;
        std::string thumbnail;
        std::string creator;
        int downloads;
        bool isFavorite;
    };

    // Toolbox categories
    enum class ToolboxCategory {
        Models,
        Meshes,
        Decals,
        Audio,
        Plugins,
        Count
    };

    // Initialize Studio UI
    void init();

    // Run Studio loop (returns when exiting)
    void runStudio(Node* dataModel);

    // === Toolbox ===
    void drawToolbox();
    void insertToolboxAsset(const std::string& assetId, Node* dataModel);

    // Draw top screen (3D workspace)
    void drawWorkspace(Node* dataModel);

    // Draw bottom screen UI
    void drawBottomUI(Node* dataModel);

    // Draw Explorer panel
    void drawExplorer(Node* root, int scrollOffset);

    // Draw Properties panel
    void drawProperties(Node* selected, int scrollOffset);

    // Draw Script Editor
    void drawScriptEditor();

    // Draw left toolbar (Exit, Save, Load, Test, Toolbox)
    void drawToolbar();

    // Process input
    void processInput(Node* dataModel);

    // Select part by name
    void selectPart(Node* node);

    // Create new part
    Part* createPart(const std::string& name, float x, float y, float z);

    // Delete selected part
    void deleteSelected(Node* dataModel);

    // Save current workspace to .rbxl
    bool saveWorkspace(Node* dataModel, const std::string& path);

    // Load workspace from .rbxl
    bool loadWorkspace(Node** dataModel, const std::string& path);

    // Switch to play test mode
    void enterPlayMode(Node* dataModel);

    // Exit play mode
    void exitPlayMode();

    // Get selected node
    Node* getSelectedNode();

    // Set selected node
    void setSelectedNode(Node* node);

    // Shutdown
    void shutdown();
}