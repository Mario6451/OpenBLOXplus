#pragma once

#include <string>
#include <vector>
#include <map>
#include <3ds.h>
#include "CoreEngine/Node/nodes.h"

// OpenBLOX Studio Client
// Provides a Roblox Studio-like editing experience on Nintendo 3DS
// Top screen: 3D viewport showing the edited place
// Bottom screen: Explorer (left) + Properties (right) OR Script Editor

// Studio tab types
enum class StudioTab {
    Explorer,
    ScriptEditor,
    Properties
};

// Studio instance (editable object)
struct StudioInstance {
    std::string name;
    std::string className;
    int parentId = -1;
    float position[3] = {0, 0, 0};
    float size[3] = {4, 4, 4};
    float color[3] = {0.8f, 0.2f, 0.2f};
    float rotation[3] = {0, 0, 0};
    std::string scriptSource;
    std::string texturePath;
    bool isLocalScript = false;
    std::map<std::string, std::string> properties;
};

// Studio editor state
struct StudioState {
    std::vector<StudioInstance> instances;
    int selectedInstance = -1;
    int scriptEditorScroll = 0;
    bool isTesting = false;
    bool modified = false;
    std::string currentFile;
    StudioTab activeTab = StudioTab::Explorer;
};

namespace StudioClient {

    // === Studio Mode ===
    enum class StudioMode {
        Editing,          // Explorer + Properties view
        ScriptEditing,    // Script editor view
        Testing           // Play test mode
    };

    // === Explorer Tree Node ===
    struct ExplorerNode {
        Node* node;
        bool expanded;
        bool selected;
        std::vector<ExplorerNode*> children;

        ExplorerNode() : node(nullptr), expanded(false), selected(false) {}
        ExplorerNode(Node* n) : node(n), expanded(false), selected(false) {}
    };

    // === Property Entry ===
    struct PropertyEntry {
        std::string name;
        std::string value;
        std::string type;    // "string", "int", "float", "bool", "Vector3", "Color3", "BrickColor"
        bool isEditable;
    };

    // === Script File ===
    struct ScriptFile {
        std::string name;
        std::string source;
        Node* scriptNode;
    };

    // === Public API (called from Lua scripts) ===
    namespace API {
        // Instance operations (Roblox API compatible)
        Node* newInstance(const std::string& className);
        void destroyInstance(Node* node);
        Node* cloneInstance(Node* node);
        void setParent(Node* node, Node* parent);
        Node* findFirstChild(Node* parent, const std::string& name);
        std::vector<Node*> getChildren(Node* parent);

        // Property operations
        void setProperty(Node* node, const std::string& name, const std::string& value);
        std::string getProperty(Node* node, const std::string& name);
        void setPropertyVec3(Node* node, const std::string& name, float x, float y, float z);
        void setPropertyColor3(Node* node, const std::string& name, float r, float g, float b);
        void setPropertyBrickColor(Node* node, const std::string& name, int index);

        // Workspace operations
        Node* getWorkspace();
        Node* getPlayers();
        Node* getLighting();
        Node* getReplicatedStorage();
        Node* getServerScriptService();
    }

    // === Core Functions ===
    void init();
    void runEditor();
    std::string runStudio();  // Entry point for main.cpp - runs editor and returns result
    void shutdown();

    // === Save/Load ===
    bool savePlace(const std::string& path);
    bool loadPlace(const std::string& path);

    // === Drawing ===
    void drawViewport();      // Top screen 3D viewport
    void drawExplorer();      // Bottom screen left panel
    void drawProperties();    // Bottom screen right panel
    void drawToolbar();       // Bottom screen left toolbar (exit, save, load, test, import, add)
    void drawScriptEditor();  // Script editing panel
    void drawStatusBar();     // Bottom status bar

    // === Asset Management ===
    void importAssets();      // Import images/GIFs from SD card
    void showAddMenu();       // Show object creation menu

    // === Input ===
    void processInput();

    // === Instance Management ===
    int addInstance(const std::string& className, int parent = -1);
    void deleteInstance(int id);
    void duplicateInstance(int id);
    StudioInstance* getInstance(int id);
    int selectInstance(int id);
    std::vector<int> getChildren(int parentId);
    void setProperty(int instanceId, const std::string& prop, const std::string& value);
    std::string getProperty(int instanceId, const std::string& prop);

    // === Test Mode ===
    void startTest();
    void stopTest();

    // === State ===
    Node* getSelectedPart();
    void setSelectedPart(Node* part);
    StudioMode getCurrentMode();
    void setCurrentMode(StudioMode mode);
}