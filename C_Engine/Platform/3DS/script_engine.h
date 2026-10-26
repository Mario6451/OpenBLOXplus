#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// Forward declarations
typedef struct lua_State lua_State;
class Node;

// Lua script engine for OpenBLOX - Phase 1: Basic execution + Instance API
// Provides Roblox-like Lua API for running game scripts
// Supports: Lua 5.1, Lua 5.4, Luau (Roblox's custom dialect)

namespace ScriptEngine {

    // Lua/Luau version detection
    enum class LuaVersion {
        Lua51,      // Lua 5.1 (legacy Roblox scripts)
        Lua54,      // Lua 5.4 (current default)
        Luau        // Roblox Luau dialect
    };

    // Detect Lua version from file path
    LuaVersion detectLuaVersion(const std::string& filePath);

    // Detect Lua version from source content
    LuaVersion detectLuaVersionFromSource(const std::string& source);

    // Convert Luau-specific syntax to valid Lua 5.4
    // Handles: type annotations, continue, compound assignments, numeric literals
    std::string convertLuauToLua(const std::string& luauSource);

    // Convert Lua 5.1 deprecated features to Lua 5.4 compatible
    std::string convertLua51ToLua54(const std::string& lua51Source);

    // Preprocess script source based on detected version
    std::string preprocessScript(const std::string& source, LuaVersion version);

    // Deprecated Roblox API registry
    void setupDeprecatedAPIs();

    // A Lua connection (event callback)
    struct LuaConnection {
        int reference;  // Lua reference ID
        bool connected;
        LuaConnection() : reference(-1), connected(false) {}
    };

    // A loaded script with its Lua thread
    struct LoadedScript {
        std::string path;
        int thread_ref;  // Reference to coroutine thread
        bool running;
        std::vector<LuaConnection> connections;
        LoadedScript() : thread_ref(-1), running(false) {}
    };

    // Game service types
    enum class ServiceType {
        RunService,
        Players,
        Lighting,
        Workspace,
        SoundService,
        StaterPlayer,
        GuiService,
        Unknown
    };

    // Initialize the Lua script engine
    // Returns true on success, false if Lua cannot be initialized
    bool init();

    // Shutdown and cleanup Lua state
    void shutdown();

    // Load and execute a Lua script file
    // Returns true if script loaded successfully
    bool loadScript(const std::string& filePath);

    // Load and execute a Lua script from a string
    bool loadScriptString(const std::string& scriptName, const std::string& scriptSource);

    // Unload a specific script
    void unloadScript(const std::string& scriptPath);

    // Unload all scripts (call when leaving a game)
    void unloadAllScripts();

    // Call the Roblox wait(n) function - yields for n seconds
    // Called from Lua via os.execute or similar
    double luaWait(double seconds);

    // Get the current Lua state (for advanced debugging)
    lua_State* getLuaState();

    // Print a message from Lua (redirects to console/debug output)
    void luaPrint(const std::string& message);

    // Get total number of loaded scripts
    int getLoadedScriptCount();

    // Get list of loaded script paths
    std::vector<std::string> getLoadedScriptPaths();

    // Check if engine is initialized
    bool isInitialized();

    // Get total Lua memory usage in bytes
    size_t getLuaMemoryUsage();

    // Roblox API: game object access
    void setupGameEnvironment();

    // Roblox API: Instance operations
    void setupInstanceAPI();

    // Roblox API: Vector3 operations  
    void setupVector3API();

    // Roblox API: CFrame operations
    void setupCFrameAPI();

    // Roblox API: BrickColor operations
    void setupBrickColorAPI();

    // Roblox API: Color3 operations
    void setupColor3API();

    // Roblox API: Event/connection system
    void setupEventSystem();

    // Roblox API: Service system (Players, Workspace, RunService, etc.)
    void setupServiceSystem();

    // Roblox API: Humanoid system (TakeDamage, etc.)
    void setupHumanoidAPI();

    // Roblox API: Sound system (play, stop, etc.)
    void setupSoundAPI();

    // Roblox API: Mouse system (for local scripts)
    void setupMouseAPI();

    // Load scripts from a directory (scans for .lua files)
    void loadScriptsFromDirectory(const std::string& directory);

    // Load scripts associated with a Node (finds Script child nodes)
    void loadScriptsFromNode(Node* node);

    // Error handling - get last error message
    std::string getLastErrorMessage();

    // Set a timeout for script execution (seconds, 0 = no timeout)
    void setScriptTimeout(double timeout);

    // Get script timeout
    double getScriptTimeout();

    // Enable/disable script execution (for pause menu, etc.)
    void setScriptsEnabled(bool enabled);
    bool areScriptsEnabled();

    // Get/set the "script" instance for the current script context
    void setCurrentScript(Node* scriptNode);
    Node* getCurrentScript();

    // Node type for Script instances
    extern const char* SCRIPT_NODE_TYPE;
};

// Helper function to check if a string is a valid Lua file path
bool isLuaFile(const std::string& path);

// Helper function to get Lua file extension
std::string getLuaExtension(const std::string& path);
