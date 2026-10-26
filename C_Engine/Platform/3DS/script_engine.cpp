#include "script_engine.h"
#include "luau_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#ifndef LUA_OK
#define LUA_OK 0
#endif

#include "../../CoreEngine/Node/nodes.h"
#include "studio_objects.h"
#include "roblox_services.h"

// Simple JSON parser (for HttpService:JSONDecode)
#include <cctype>

// External references
extern Node *dataModel;
extern Node *characterModel;

namespace ScriptEngine {

const char* SCRIPT_NODE_TYPE = "Script";

// Internal state
static lua_State* L = nullptr;
static bool initialized = false;
static bool scriptsEnabled = true;
static double scriptTimeout = 0.0;
static std::string lastErrorMessage;
static std::vector<LoadedScript> loadedScripts;
static std::unordered_map<uintptr_t, Node*> nodeRegistry;
static Node* currentScriptNode = nullptr;

// Service registry
static std::unordered_map<std::string, ServiceType> serviceRegistry;
static std::unordered_map<ServiceType, Node*> serviceNodes;

bool init() {
    if (initialized) return true;
    
    // Initialize studio object templates
    StudioObjects::init();

#if defined(HAVE_LUA)
    try {
        L = luaL_newstate();
        if (!L) {
            lastErrorMessage = "Failed to create Lua state";
            return false;
        }
        
        // Load standard libraries
        luaL_openlibs(L);
        
        // Setup Roblox API
        setupGameEnvironment();
        setupInstanceAPI();
        setupVector3API();
        setupCFrameAPI();
        setupBrickColorAPI();
        setupColor3API();
        setupEventSystem();
        setupServiceSystem();
        setupHumanoidAPI();
        setupSoundAPI();
        setupMouseAPI();
        setupDeprecatedAPIs();
        
        // Initialize Luau Hybrid VM manager
        LuauManager::init();
        
        initialized = true;
        return true;
    } catch (const std::exception& e) {
        lastErrorMessage = std::string("Lua init error: ") + e.what();
        if (L) {
            lua_close(L);
            L = nullptr;
        }
        return false;
    }
#else
    // Fallback: engine can run without Lua
    initialized = true;
    return true;
#endif
}

void shutdown() {
    if (!initialized) return;
    
    LuauManager::shutdown();
    
    unloadAllScripts();
    nodeRegistry.clear();
    serviceRegistry.clear();
    serviceNodes.clear();
    
#if defined(HAVE_LUA)
    if (L) {
        lua_close(L);
        L = nullptr;
    }
#endif
    
    initialized = false;
    scriptsEnabled = true;
    scriptTimeout = 0.0;
    currentScriptNode = nullptr;
}

lua_State* getLuaState() {
    return L;
}

bool isInitialized() {
    return initialized;
}

size_t getLuaMemoryUsage() {
#if defined(HAVE_LUA)
    if (L) return lua_gc(L, LUA_GCCOUNT, 0) * 1024;
#endif
    return 0;
}

// ============================================================================
// Script Loading & Execution
// ============================================================================

bool loadScript(const std::string& filePath) {
    if (!initialized || !scriptsEnabled) return false;
    
    // Check if already loaded
    for (auto& s : loadedScripts) {
        if (s.path == filePath) return false;
    }
    
#if defined(HAVE_LUA)
    // Check file exists
    FILE* f = fopen(filePath.c_str(), "r");
    if (!f) {
        lastErrorMessage = "Cannot open script: " + filePath;
        return false;
    }
    fclose(f);
    
    // Read file contents
    f = fopen(filePath.c_str(), "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::string source(size, '\0');
    fread(&source[0], 1, size, f);
    fclose(f);
    
    // Use LuauManager to detect VM type and preprocess if needed
    LuauVMType vmType = LuauManager::detectVMType(filePath, source);
    std::string processedSource = source;
    
    if (vmType == LuauVMType::Luau) {
        processedSource = LuauManager::preprocessLuau(source);
        printf("[ScriptEngine] Using Luau preprocessing for %s\n", filePath.c_str());
    }
    
    // Create coroutine for this script
    lua_State* thread = lua_newthread(L);
    int threadRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    // Push script as chunk
    if (luaL_loadbuffer(thread, processedSource.c_str(), processedSource.size(), filePath.c_str())) {
        lastErrorMessage = "Script load error: " + std::string(lua_tostring(thread, -1));
        lua_pop(thread, 1);
        luaL_unref(L, LUA_REGISTRYINDEX, threadRef);
        return false;
    }
    
    // Setup script environment
    setCurrentScript(nullptr);  // Will be set when loading from Node
    
    // Push 'script' object into script environment
    lua_pushlightuserdata(thread, &filePath);
    lua_setglobal(thread, "script");
    
    // Execute in coroutine
    int status = lua_pcall(thread, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        lastErrorMessage = "Script error: " + std::string(lua_tostring(thread, -1));
        lua_pop(thread, 1);
        luaL_unref(L, LUA_REGISTRYINDEX, threadRef);
        return false;
    }
    
    // Store loaded script info
    LoadedScript ls;
    ls.path = filePath;
    ls.thread_ref = threadRef;
    ls.running = true;
    loadedScripts.push_back(ls);
    
    return true;
#else
    lastErrorMessage = "Lua not available - compile with HAVE_LUA";
    return false;
#endif
}

bool loadScriptString(const std::string& scriptName, const std::string& scriptSource) {
    if (!initialized || !scriptsEnabled) return false;
    
#if defined(HAVE_LUA)
    // Detect Lua version and preprocess using LuauManager
    std::string source = preprocessScript(scriptSource, detectLuaVersion(scriptName));
    
    // Create coroutine
    lua_State* thread = lua_newthread(L);
    int threadRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    if (luaL_loadbuffer(thread, source.c_str(), source.size(), scriptName.c_str())) {
        lastErrorMessage = "Script load error: " + std::string(lua_tostring(thread, -1));
        lua_pop(thread, 1);
        luaL_unref(L, LUA_REGISTRYINDEX, threadRef);
        return false;
    }
    
    int status = lua_pcall(thread, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        lastErrorMessage = "Script error: " + std::string(lua_tostring(thread, -1));
        lua_pop(thread, 1);
        luaL_unref(L, LUA_REGISTRYINDEX, threadRef);
        return false;
    }
    
    LoadedScript ls;
    ls.path = scriptName;
    ls.thread_ref = threadRef;
    ls.running = true;
    loadedScripts.push_back(ls);
    
    return true;
#else
    lastErrorMessage = "Lua not available";
    return false;
#endif
}

void unloadScript(const std::string& scriptPath) {
    for (auto it = loadedScripts.begin(); it != loadedScripts.end(); ++it) {
        if (it->path == scriptPath) {
            it->running = false;
            
#if defined(HAVE_LUA)
            if (it->thread_ref >= 0) {
                luaL_unref(L, LUA_REGISTRYINDEX, it->thread_ref);
            }
#endif
            
            // Disconnect all connections
            for (auto& conn : it->connections) {
                if (conn.connected) {
#if defined(HAVE_LUA)
                    luaL_unref(L, LUA_REGISTRYINDEX, conn.reference);
#endif
                    conn.connected = false;
                }
            }
            
            it->connections.clear();
            loadedScripts.erase(it);
            break;
        }
    }
}

void unloadAllScripts() {
    for (auto& s : loadedScripts) {
        s.running = false;
#if defined(HAVE_LUA)
        if (s.thread_ref >= 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, s.thread_ref);
        }
        for (auto& conn : s.connections) {
            if (conn.connected && conn.reference >= 0) {
                luaL_unref(L, LUA_REGISTRYINDEX, conn.reference);
            }
        }
#endif
    }
    loadedScripts.clear();
}

int getLoadedScriptCount() {
    return (int)loadedScripts.size();
}

std::vector<std::string> getLoadedScriptPaths() {
    std::vector<std::string> paths;
    for (auto& s : loadedScripts) {
        paths.push_back(s.path);
    }
    return paths;
}

void loadScriptsFromDirectory(const std::string& directory) {
    DIR* dir = opendir(directory.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        std::string ext = "";
        if (name.size() > 4) ext = name.substr(name.size() - 4);
        if (name.size() > 5) {
            std::string ext5 = name.substr(name.size() - 5);
            if (ext5 == ".luau" || ext5 == ".Luau" || ext5 == ".LUAU")
                ext = ext5;
        }
        if (ext == ".lua" || ext == ".Lua" || ext == ".LUA" ||
            ext == ".luau" || ext == ".Luau" || ext == ".LUAU") {
            std::string fullPath = directory + "/" + name;
            loadScript(fullPath);
        }
    }
    
    closedir(dir);
}

void loadScriptsFromNode(Node* node) {
    if (!node) return;
    
    // Check if this node is a Script
    if (strcmp(node->type, SCRIPT_NODE_TYPE) == 0) {
        // Script source should be in a property or loaded from file
        // For now, we'll just mark it as loaded
        return;
    }
    
    // Recurse into children
    for (Node* child : node->children) {
        loadScriptsFromNode(child);
    }
}

// ============================================================================
// Roblox API: Game Environment
// ============================================================================

void setupGameEnvironment() {
#if defined(HAVE_LUA)
    // Create 'game' table
    lua_newtable(L);
    lua_setglobal(L, "game");

    // game:GetService(serviceName)
    lua_getglobal(L, "game");
    lua_pushcfunction(L, &luaGameGetService);
    lua_setfield(L, -2, "GetService");
    lua_pop(L, 1);

    // game:service(serviceName) (legacy)
    lua_getglobal(L, "game");
    lua_pushcfunction(L, &luaGameService);
    lua_setfield(L, -2, "service");
    lua_pop(L, 1);

    // game:FindFirstChild(name)
    lua_getglobal(L, "game");
    lua_pushcfunction(L, &luaGameFindFirstChild);
    lua_setfield(L, -2, "FindFirstChild");
    lua_pop(L, 1);

    // Global functions
    // spawn(function)
    lua_pushcfunction(L, &luaSpawn);
    lua_setglobal(L, "spawn");

    // delay(seconds, function)
    lua_pushcfunction(L, &luaDelay);
    lua_setglobal(L, "delay");

    // tick()
    lua_pushcfunction(L, &luaTick);
    lua_setglobal(L, "tick");

    // time() - alias for tick()
    lua_pushcfunction(L, &luaTime);
    lua_setglobal(L, "time");

    // elapsedTime()
    lua_pushcfunction(L, &luaElapsedTime);
    lua_setglobal(L, "elapsedTime");

    // wait(seconds)
    lua_pushcfunction(L, &luaWait);
    lua_setglobal(L, "wait");

    // print(...)
    lua_pushcfunction(L, &luaPrint);
    lua_setglobal(L, "print");

    // warn(...)
    lua_pushcfunction(L, &luaWarn);
    lua_setglobal(L, "warn");

    // error(message)
    lua_pushcfunction(L, &luaError);
    lua_setglobal(L, "error");
#endif
}

// ============================================================================
// Roblox API: Instance System
// ============================================================================

void setupInstanceAPI() {
#if defined(HAVE_LUA)
    // Instance.new(className)
    lua_pushcfunction(L, &luaInstanceNew);
    lua_setglobal(L, "Instance");
    
    // Instance methods
    // :findFirstChild(name)
    // :FindFirstChild(name)
    // :GetChildren()
    // :GetFullName()
    // :IsA(className)
    // etc.
#endif
}

// ============================================================================
// Roblox API: Vector3
// ============================================================================

void setupVector3API() {
#if defined(HAVE_LUA)
    // Vector3.new(x, y, z)
    lua_pushcfunction(L, &luaVector3New);
    lua_setglobal(L, "Vector3");
    
    // Vector3 constants
    lua_getglobal(L, "Vector3");
    
    lua_newtable(L);
    lua_pushnumber(L, 0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "zero");
    
    lua_newtable(L);
    lua_pushnumber(L, 1); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "right");
    
    lua_newtable(L);
    lua_pushnumber(L, 0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 1); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "up");
    
    lua_newtable(L);
    lua_pushnumber(L, 0); lua_setfield(L, -2, "x");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "y");
    lua_pushnumber(L, 1); lua_setfield(L, -2, "z");
    lua_setfield(L, -2, "forward");
    
    lua_pop(L, 1);
#endif
}

// ============================================================================
// Roblox API: CFrame
// ============================================================================

void setupCFrameAPI() {
#if defined(HAVE_LUA)
    // CFrame.new(x, y, z) or CFrame.new(pos, lookAt)
    lua_pushcfunction(L, &luaCFrameNew);
    lua_setglobal(L, "CFrame");
    
    // CFrame constants
    lua_getglobal(L, "CFrame");
    
    lua_newtable(L);  // identity
    lua_pushnumber(L, 0); lua_setfield(L, -2, "X");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "Y");
    lua_pushnumber(L, 0); lua_setfield(L, -2, "Z");
    lua_setfield(L, -2, "identity");
    
    lua_pop(L, 1);
#endif
}

// ============================================================================
// Roblox API: BrickColor
// ============================================================================

void setupBrickColorAPI() {
#if defined(HAVE_LUA)
    // BrickColor.new(numberOrString)
    lua_pushcfunction(L, &luaBrickColorNew);
    lua_setglobal(L, "BrickColor");
    
    // Common BrickColor constants (255 colors)
    // These are mapped to the Roblox color palette
#endif
}

// ============================================================================
// Roblox API: Color3
// ============================================================================

void setupColor3API() {
#if defined(HAVE_LUA)
    // Color3.new(r, g, b)
    // Color3.fromRGB(r, g, b)
    // Color3.fromHSV(h, s, v)
    lua_pushcfunction(L, &luaColor3New);
    lua_setglobal(L, "Color3");
#endif
}

// ============================================================================
// Roblox API: Events & Connections
// ============================================================================

void setupEventSystem() {
#if defined(HAVE_LUA)
    // Event:connect(callback)
    // Returns RBXScriptConnection
    // Connection:disconnect()
#endif
}

// ============================================================================
// Roblox API: Services
// ============================================================================

void setupServiceSystem() {
#if defined(HAVE_LUA)
    // Register services
    serviceRegistry["RunService"] = ServiceType::RunService;
    serviceRegistry["Players"] = ServiceType::Players;
    serviceRegistry["Lighting"] = ServiceType::Lighting;
    serviceRegistry["Workspace"] = ServiceType::Workspace;
    serviceRegistry["SoundService"] = ServiceType::SoundService;
    serviceRegistry["StarterPlayer"] = ServiceType::StaterPlayer;
    serviceRegistry["GuiService"] = ServiceType::GuiService;

    // Additional services via RobloxServices layer
    serviceRegistry["HttpService"] = ServiceType::Unknown;
    serviceRegistry["TeamsService"] = ServiceType::Unknown;
    serviceRegistry["AssetService"] = ServiceType::Unknown;
    serviceRegistry["CollectionService"] = ServiceType::Unknown;
    serviceRegistry["DataStoreService"] = ServiceType::Unknown;
    serviceRegistry["LocalizationService"] = ServiceType::Unknown;
    serviceRegistry["MarketPlaceService"] = ServiceType::Unknown;
    serviceRegistry["TweenService"] = ServiceType::Unknown;
    serviceRegistry["PhysicsService"] = ServiceType::Unknown;
    serviceRegistry["ContentProvider"] = ServiceType::Unknown;
    serviceRegistry["StarterGui"] = ServiceType::Unknown;
    serviceRegistry["StarterPack"] = ServiceType::Unknown;
    serviceRegistry["ReplicatedStorage"] = ServiceType::Unknown;
    serviceRegistry["ServerScriptStorage"] = ServiceType::Unknown;
    serviceRegistry["ServerStorage"] = ServiceType::Unknown;
    serviceRegistry["Statistics"] = ServiceType::Unknown;

    // Create service nodes
    Node* workspace = new Node();
    strcpy(workspace->name, "Workspace");
    strcpy(workspace->type, "DataModel");
    serviceNodes[ServiceType::Workspace] = workspace;

    // Create HttpService node
    Node* httpService = new Node();
    strcpy(httpService->name, "HttpService");
    strcpy(httpService->type, "HttpService");
    serviceNodes[ServiceType::Unknown] = httpService;

    // Create TeamsService node
    Node* teamsService = new Node();
    strcpy(teamsService->name, "TeamsService");
    strcpy(teamsService->type, "TeamsService");

    // Create Lighting node
    Node* lighting = new Node();
    strcpy(lighting->name, "Lighting");
    strcpy(lighting->type, "Lighting");
    serviceNodes[ServiceType::Lighting] = lighting;

    // Create Players node
    Node* players = new Node();
    strcpy(players->name, "Players");
    strcpy(players->type, "Players");
    serviceNodes[ServiceType::Players] = players;

    // Register RobloxServices Lua bindings
    RobloxServices::registerAllServices(L);

    // Setup game:GetService() to retrieve services
    // (This is already handled in luaGameGetService below)
    printf("[ScriptEngine] Service system setup complete\n");
#endif
}

// ============================================================================
// Roblox API: Humanoid
// ============================================================================

void setupHumanoidAPI() {
#if defined(HAVE_LUA)
    // Humanoid:TakeDamage(amount)
    // Humanoid.Health
    // Humanoid.MaxHealth
    // Humanoid.HealthChanged
    // Humanoid.Died
#endif
}

// ============================================================================
// Roblox API: Sound
// ============================================================================

void setupSoundAPI() {
#if defined(HAVE_LUA)
    // Sound:play()
    // Sound:stop()
    // Sound:pause()
    // Sound.SoundId
    // Sound.Volume
    // Sound.Playing
#endif
}

// ============================================================================
// Roblox API: Mouse
// ============================================================================

void setupMouseAPI() {
#if defined(HAVE_LUA)
    // Mouse.Button1Down
    // Mouse.Button1Up
    // Mouse.Button1Click
    // Mouse.Icon
    // Mouse.Hit
    // Mouse.Target
#endif
}

// ============================================================================
// Lua C Functions (Roblox API Bindings)
// ============================================================================

// Helper: Extract Node pointer from Lua userdata argument
static Node* getInstanceFromLua(lua_State* L, int idx) {
    if (lua_type(L, idx) == LUA_TUSERDATA) {
        void* ud = lua_touserdata(L, idx);
        if (ud) {
            Node* node = *(Node**)ud;
            return node;
        }
    } else if (lua_type(L, idx) == LUA_TLIGHTUSERDATA) {
        Node* node = (Node*)lua_touserdata(L, idx);
        return node;
    }
    return nullptr;
}

static int luaGameService(lua_State* L) {
    const char* serviceName = lua_tostring(L, 2);
    if (!serviceName) return 0;
    
#if defined(HAVE_LUA)
    // Find service
    auto it = serviceRegistry.find(serviceName);
    if (it != serviceRegistry.end()) {
        auto nit = serviceNodes.find(it->second);
        if (nit != serviceNodes.end()) {
            // Push service node userdata
            lua_pushlightuserdata(L, nit->second);
            return 1;
        }
    }
#endif
    return 0;
}

static int luaInstanceNew(lua_State* L) {
    const char* className = lua_tostring(L, 1);
    if (!className) return 0;
    
    // Create new Node based on class name
    Node* node = new Node();
    strncpy(node->name, className, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    strncpy(node->type, className, sizeof(node->type) - 1);
    node->type[sizeof(node->type) - 1] = '\0';
    
    // Check if we have a template for this class and apply defaults
    const auto* tmpl = StudioObjects::getTemplate(className);
    if (tmpl) {
        node->position[0] = tmpl->defaultPosition[0];
        node->position[1] = tmpl->defaultPosition[1];
        node->position[2] = tmpl->defaultPosition[2];
        node->size[0] = tmpl->defaultSize[0];
        node->size[1] = tmpl->defaultSize[1];
        node->size[2] = tmpl->defaultSize[2];
        // color is set via BrickColor API for Part nodes
    }
    
#if defined(HAVE_LUA)
    // Register node
    nodeRegistry[(uintptr_t)node] = node;
    
    // Create userdata
    Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
    *ud = node;
    
    // Setup metatable
    luaL_newmetatable(L, "Instance");
    lua_pushcfunction(L, &luaInstanceFindFirstChild);
    lua_setfield(L, -2, "findFirstChild");
    lua_pushcfunction(L, &luaInstanceFindFirstChild);
    lua_setfield(L, -2, "FindFirstChild");
    lua_pushcfunction(L, &luaInstanceGetName);
    lua_setfield(L, -2, "GetName");
    lua_pushcfunction(L, &luaInstanceGetParent);
    lua_setfield(L, -2, "GetParent");
    
    // Property getters/setters
    lua_pushvalue(L, lua_upvalueindex(1));  // __index
    lua_setfield(L, -2, "__index");
    lua_pushvalue(L, lua_upvalueindex(2));  // __newindex
    lua_setfield(L, -2, "__newindex");
    
    lua_setmetatable(L, -2);
    return 1;
#else
    return 0;
#endif
}

// ============================================================================
// Roblox API: Time & Global Functions
// ============================================================================

static int luaTick(lua_State* L) {
    // tick() - Returns current time in seconds since Unix epoch
    lua_pushnumber(L, (double)time(nullptr));
    return 1;
}

static int luaTime(lua_State* L) {
    // time() - Same as tick()
    lua_pushnumber(L, (double)time(nullptr));
    return 1;
}

static int luaElapsedTime(lua_State* L) {
    // elapsedTime() - Returns time since game started
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - start).count();
    lua_pushnumber(L, elapsed);
    return 1;
}

static int luaWait(lua_State* L) {
    // wait(seconds) - Yields for specified seconds
    double seconds = luaL_optnumber(L, 1, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(seconds * 1000)));
    lua_pushnumber(L, seconds);
    return 1;
}

static int luaPrint(lua_State* L) {
    // print(...) - Print arguments to console
    std::string output = "[LUA] ";
    for (int i = 1; i <= lua_gettop(L); i++) {
        if (i > 1) output += " ";
        lua_tolstring(L, i, nullptr);
        output += lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    printf("%s\n", output.c_str());
    return 0;
}

static int luaWarn(lua_State* L) {
    // warn(...) - Print warning to console
    std::string output = "[LUA WARN] ";
    for (int i = 1; i <= lua_gettop(L); i++) {
        if (i > 1) output += " ";
        lua_tolstring(L, i, nullptr);
        output += lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    printf("%s\n", output.c_str());
    return 0;
}

static int luaError(lua_State* L) {
    // error(message) - Throw error
    const char* msg = lua_tostring(L, 1);
    if (!msg) msg = "Script error";
    lua_error(L);
    return 0;
}

// ============================================================================
// Roblox API: math module extensions
// ============================================================================

static int luaMathRandom(lua_State* L) {
    // math.random([min, max])
    int top = lua_gettop(L);
    if (top == 0) {
        // Return random float 0-1
        lua_pushnumber(L, (double)(rand() % 10000) / 10000.0);
    } else if (top == 1) {
        int max = (int)luaL_checknumber(L, 1);
        lua_pushnumber(L, (rand() % max) + 1);
    } else {
        int min = (int)luaL_checknumber(L, 1);
        int max = (int)luaL_checknumber(L, 2);
        lua_pushnumber(L, (rand() % (max - min + 1)) + min);
    }
    return 1;
}

static int luaMathAbs(lua_State* L) {
    lua_pushnumber(L, fabs((double)luaL_checknumber(L, 1)));
    return 1;
}

static int luaMathFloor(lua_State* L) {
    lua_pushnumber(L, floor((double)luaL_checknumber(L, 1)));
    return 1;
}

static int luaMathCeil(lua_State* L) {
    lua_pushnumber(L, ceil((double)luaL_checknumber(L, 1)));
    return 1;
}

static int luaMathSqrt(lua_State* L) {
    lua_pushnumber(L, sqrt((double)luaL_checknumber(L, 1)));
    return 1;
}

static int luaMathSin(lua_State* L) {
    lua_pushnumber(L, sin((double)luaL_checknumber(L, 1)));
    return 1;
}

static int luaMathCos(lua_State* L) {
    lua_pushnumber(L, cos((double)luaL_checknumber(L, 1)));
    return 1;
}

static int luaMathTan(lua_State* L) {
    lua_pushnumber(L, tan((double)luaL_checknumber(L, 1)));
    return 1;
}

static int luaMathDeg(lua_State* L) {
    lua_pushnumber(L, (double)luaL_checknumber(L, 1) * (180.0 / 3.14159265358979323846));
    return 1;
}

static int luaMathRad(lua_State* L) {
    lua_pushnumber(L, (double)luaL_checknumber(L, 1) * (3.14159265358979323846 / 180.0));
    return 1;
}

// ============================================================================
// Roblox API: string functions
// ============================================================================

static int luaStringLen(lua_State* L) {
    size_t len = strlen(luaL_checkstring(L, 1));
    lua_pushnumber(L, len);
    return 1;
}

static int luaStringSub(lua_State* L) {
    const char* s = luaL_checkstring(L, 1);
    int start = (int)luaL_checknumber(L, 2);
    int end = (int)luaL_optnumber(L, 3, -1);
    
    int len = strlen(s);
    if (start < 1) start = 1;
    if (end < 0) end = len + 1 + end;
    if (end > len) end = len;
    
    int subLen = end - start + 1;
    lua_pushlstring(L, s + start - 1, subLen);
    return 1;
}

static int luaStringGsub(lua_State* L) {
    const char* s = luaL_checkstring(L, 1);
    const char* pattern = luaL_checkstring(L, 2);
    const char* repl = luaL_checkstring(L, 3);
    
    char* result = new char[strlen(s) + 1];
    strcpy(result, s);
    int count = 0;
    
    char* pos = result;
    while ((pos = strstr(pos, pattern)) != nullptr) {
        count++;
        int patternLen = strlen(pattern);
        int replLen = strlen(repl);
        memmove(pos + replLen, pos + patternLen, strlen(pos + patternLen) + 1);
        memcpy(pos, repl, replLen);
        pos += replLen;
    }
    
    lua_pushstring(L, result);
    lua_pushnumber(L, count);
    delete[] result;
    return 2;
}

// ============================================================================
// Roblox API: workspace (DataModel) 
// ============================================================================

static int luaWorkspaceGetChildren(lua_State* L) {
    // workspace:GetChildren()
    Node* workspace = nullptr;
    auto it = serviceNodes.find(ServiceType::Workspace);
    if (it != serviceNodes.end()) {
        workspace = it->second;
    }
    
    lua_newtable(L);
    int i = 1;
    if (workspace) {
        for (Node* child : workspace->children) {
            Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
            *ud = child;
            lua_rawseti(L, -2, i++);
        }
    }
    return 1;
}

static int luaWorkspaceFindFirstChild(lua_State* L) {
    // workspace:FindFirstChild(name)
    const char* name = lua_tostring(L, 2);
    Node* workspace = nullptr;
    auto it = serviceNodes.find(ServiceType::Workspace);
    if (it != serviceNodes.end()) {
        workspace = it->second;
    }
    
    if (workspace && name) {
        for (Node* child : workspace->children) {
            if (strcmp(child->name, name) == 0) {
                Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
                *ud = child;
                return 1;
            }
        }
    }
    return 0;
}

static int luaWorkspaceCurrentCamera(lua_State* L) {
    // workspace.CurrentCamera getter/setter
    // Returns nil for now (camera not exposed yet)
    return 0;
}

// ============================================================================
// Roblox API: game object
// ============================================================================

static int luaGameGetService(lua_State* L) {
    // game:GetService(className)
    const char* className = luaL_checkstring(L, 2);
    
    auto it = serviceRegistry.find(className);
    if (it != serviceRegistry.end()) {
        auto nit = serviceNodes.find(it->second);
        if (nit != serviceNodes.end()) {
            Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
            *ud = nit->second;
            luaL_getmetatable(L, "Instance");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
    
    // Create service on demand if not registered
    if (strcmp(className, "Workspace") == 0 || strcmp(className, "workspace") == 0) {
        auto wit = serviceNodes.find(ServiceType::Workspace);
        if (wit != serviceNodes.end()) {
            Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
            *ud = wit->second;
            return 1;
        }
    }
    return 0;
}

static int luaGameFindFirstChild(lua_State* L) {
    // game:FindFirstChild(name)
    const char* name = lua_tostring(L, 2);
    if (!name) return 0;
    
    // Check services first
    auto it = serviceRegistry.find(name);
    if (it != serviceRegistry.end()) {
        auto nit = serviceNodes.find(it->second);
        if (nit != serviceNodes.end()) {
            Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
            *ud = nit->second;
            return 1;
        }
    }
    return 0;
}

// ============================================================================
// Roblox API: Instance class (comprehensive)
// ============================================================================

static int luaInstanceFindFirstChild(lua_State* L) {
    // instance:FindFirstChild(name, recursive)
    Node* node = getInstanceFromLua(L, 1);
    const char* name = lua_tostring(L, 2);
    bool recursive = lua_toboolean(L, 3);
    
    if (!node || !name) return 0;
    
    if (recursive) {
        // Depth-first search
        for (Node* child : node->children) {
            if (strcmp(child->name, name) == 0) {
                Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
                *ud = child;
                luaL_getmetatable(L, "Instance");
                lua_setmetatable(L, -2);
                return 1;
            }
            // Recurse into child
            for (Node* grandchild : child->children) {
                if (strcmp(grandchild->name, name) == 0) {
                    Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
                    *ud = grandchild;
                    luaL_getmetatable(L, "Instance");
                    lua_setmetatable(L, -2);
                    return 1;
                }
            }
        }
    } else {
        for (Node* child : node->children) {
            if (strcmp(child->name, name) == 0) {
                Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
                *ud = child;
                luaL_getmetatable(L, "Instance");
                lua_setmetatable(L, -2);
                return 1;
            }
        }
    }
    return 0;
}

static int luaInstanceWaitForChild(lua_State* L) {
    // instance:WaitForChild(name, timeout)
    Node* node = getInstanceFromLua(L, 1);
    const char* name = lua_tostring(L, 2);
    double timeout = lua_tonumber(L, 3);
    
    if (!node || !name) return 0;
    
    // Check immediately
    for (Node* child : node->children) {
        if (strcmp(child->name, name) == 0) {
            Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
            *ud = child;
            luaL_getmetatable(L, "Instance");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
    
    // Wait up to timeout (default 30 seconds)
    if (timeout <= 0) timeout = 30;
    int elapsed = 0;
    while (elapsed < (int)timeout) {
        for (Node* child : node->children) {
            if (strcmp(child->name, name) == 0) {
                Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
                *ud = child;
                luaL_getmetatable(L, "Instance");
                lua_setmetatable(L, -2);
                return 1;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        elapsed += 100;
    }
    return 0;
}

static int luaInstanceGetChildren(lua_State* L) {
    // instance:GetChildren()
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    lua_newtable(L);
    int i = 1;
    for (Node* child : node->children) {
        Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
        *ud = child;
        luaL_getmetatable(L, "Instance");
        lua_setmetatable(L, -2);
        lua_rawseti(L, -2, i++);
    }
    return 1;
}

static int luaInstanceClone(lua_State* L) {
    // instance:Clone()
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    Node* clone = new Node();
    strcpy(clone->name, node->name);
    strcpy(clone->type, node->type);
    clone->parent = nullptr;
    
    // Deep clone children
    for (Node* child : node->children) {
        Node* childClone = new Node();
        strcpy(childClone->name, child->name);
        strcpy(childClone->type, child->type);
        childClone->parent = clone;
        clone->children.push_back(childClone);
    }
    
    nodeRegistry[(uintptr_t)clone] = clone;
    
    Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
    *ud = clone;
    luaL_getmetatable(L, "Instance");
    lua_setmetatable(L, -2);
    return 1;
}

static int luaInstanceDestroy(lua_State* L) {
    // instance:Destroy()
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    // Remove from parent
    if (node->parent) {
        for (auto it = node->parent->children.begin(); it != node->parent->children.end(); ++it) {
            if (*it == node) {
                node->parent->children.erase(it);
                break;
            }
        }
    }
    
    // Remove from registry
    nodeRegistry.erase((uintptr_t)node);
    
    // Delete children
    for (Node* child : node->children) {
        nodeRegistry.erase((uintptr_t)child);
        delete child;
    }
    
    delete node;
    return 0;
}

static int luaInstanceIsA(lua_State* L) {
    // instance:IsA(className)
    Node* node = getInstanceFromLua(L, 1);
    const char* className = lua_tostring(L, 2);
    if (!node || !className) return 0;
    
    // Check if type matches
    bool result = (strcmp(node->type, className) == 0);
    lua_pushboolean(L, result);
    return 1;
}

static int luaInstanceIsDescendantOf(lua_State* L) {
    // instance:IsDescendantOf(ancestor)
    Node* node = getInstanceFromLua(L, 1);
    Node* ancestor = getInstanceFromLua(L, 2);
    if (!node || !ancestor) return 0;
    
    Node* current = node->parent;
    while (current) {
        if (current == ancestor) {
            lua_pushboolean(L, 1);
            return 1;
        }
        current = current->parent;
    }
    lua_pushboolean(L, 0);
    return 1;
}

// Property access for Instance
static int luaInstanceGetParent(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node || !node->parent) return 0;
    
    Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
    *ud = node->parent;
    luaL_getmetatable(L, "Instance");
    lua_setmetatable(L, -2);
    return 1;
}

static int luaInstanceSetName(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    const char* name = lua_tostring(L, 2);
    if (name) {
        strncpy(node->name, name, sizeof(node->name) - 1);
        node->name[sizeof(node->name) - 1] = '\0';
    }
    return 0;
}

static int luaInstanceGetFullName(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    std::string fullName = std::string(node->name);
    Node* current = node->parent;
    while (current) {
        fullName = std::string(current->name) + "." + fullName;
        current = current->parent;
    }
    lua_pushstring(L, fullName.c_str());
    return 1;
}

// ============================================================================
// Roblox API: BasePart/Part properties
// ============================================================================

static int luaPartGetPosition(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    lua_newtable(L);
    lua_pushnumber(L, node->position[0]); lua_setfield(L, -2, "X");
    lua_pushnumber(L, node->position[0]); lua_setfield(L, -2, "x");
    lua_pushnumber(L, node->position[1]); lua_setfield(L, -2, "Y");
    lua_pushnumber(L, node->position[1]); lua_setfield(L, -2, "y");
    lua_pushnumber(L, node->position[2]); lua_setfield(L, -2, "Z");
    lua_pushnumber(L, node->position[2]); lua_setfield(L, -2, "z");
    return 1;
}

static int luaPartSetPosition(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->position[0] = (float)luaL_optnumber(L, 2, node->position[0]);
    node->position[1] = (float)luaL_optnumber(L, 3, node->position[1]);
    node->position[2] = (float)luaL_optnumber(L, 4, node->position[2]);
    return 0;
}

static int luaPartGetRotation(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    lua_newtable(L);
    lua_pushnumber(L, node->rotation[0]); lua_setfield(L, -2, "X");
    lua_pushnumber(L, node->rotation[1]); lua_setfield(L, -2, "Y");
    lua_pushnumber(L, node->rotation[2]); lua_setfield(L, -2, "Z");
    return 1;
}

static int luaPartSetRotation(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->rotation[0] = (float)luaL_optnumber(L, 2, node->rotation[0]);
    node->rotation[1] = (float)luaL_optnumber(L, 3, node->rotation[1]);
    node->rotation[2] = (float)luaL_optnumber(L, 4, node->rotation[2]);
    return 0;
}

static int luaPartGetSize(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    lua_newtable(L);
    lua_pushnumber(L, node->size[0]); lua_setfield(L, -2, "X");
    lua_pushnumber(L, node->size[1]); lua_setfield(L, -2, "Y");
    lua_pushnumber(L, node->size[2]); lua_setfield(L, -2, "Z");
    return 1;
}

static int luaPartSetSize(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->size[0] = (float)luaL_optnumber(L, 2, node->size[0]);
    node->size[1] = (float)luaL_optnumber(L, 3, node->size[1]);
    node->size[2] = (float)luaL_optnumber(L, 4, node->size[2]);
    return 0;
}

static int luaPartSetAnchored(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->anchored = lua_toboolean(L, 2);
    return 0;
}

static int luaPartGetAnchored(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    lua_pushboolean(L, node->anchored);
    return 1;
}

static int luaPartSetTransparency(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->transparency = (float)luaL_optnumber(L, 2, node->transparency);
    return 0;
}

static int luaPartGetTransparency(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    lua_pushnumber(L, node->transparency);
    return 1;
}

static int luaPartSetMaterial(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    Part* part = dynamic_cast<Part*>(node);
    if (part) part->material = (int)luaL_checknumber(L, 2);
    return 0;
}

static int luaPartGetMaterial(lua_State* L) {
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    Part* part = dynamic_cast<Part*>(node);
    if (part) lua_pushnumber(L, part->material);
    return 1;
}

static int luaPartMoveTo(lua_State* L) {
    // part:MoveTo(position)
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    // Get position from table
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "X");
        if (lua_isnumber(L, -1)) node->position[0] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 2, "Y");
        if (lua_isnumber(L, -1)) node->position[1] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 2, "Z");
        if (lua_isnumber(L, -1)) node->position[2] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    } else {
        node->position[0] = (float)luaL_optnumber(L, 2, node->position[0]);
        node->position[1] = (float)luaL_optnumber(L, 3, node->position[1]);
        node->position[2] = (float)luaL_optnumber(L, 4, node->position[2]);
    }
    return 0;
}

// ============================================================================
// Roblox API: BrickColor (255 classic colors)
// ============================================================================

static const struct {
    int number;
    const char* name;
    float r, g, b;
} classicBrickColors[] = {
    { 1, "Really red",                  0.75f, 0.05f, 0.05f },
    { 2, "Bright red",                  0.90f, 0.18f, 0.18f },
    { 3, "Medium red",                  0.70f, 0.13f, 0.13f },
    { 4, "Dark red",                    0.53f, 0.09f, 0.09f },
    { 5, "Really black",                0.09f, 0.09f, 0.09f },
    { 6, "Dark gray",                   0.35f, 0.35f, 0.35f },
    { 7, "Medium gray",                 0.55f, 0.55f, 0.55f },
    { 8, "Bright gray",                 0.75f, 0.75f, 0.75f },
    { 9, "White",                       0.95f, 0.95f, 0.95f },
    { 10, "Light yellow",               0.93f, 0.93f, 0.70f },
    { 11, "Yellow",                     0.91f, 0.91f, 0.32f },
    { 12, "Neon green",                 0.39f, 1.00f, 0.33f },
    { 13, "Lime green",                 0.62f, 0.84f, 0.31f },
    { 14, "Bright green",               0.16f, 0.75f, 0.10f },
    { 15, "Medium green",               0.19f, 0.58f, 0.17f },
    { 16, "Dark green",                 0.11f, 0.42f, 0.09f },
    { 17, "Earth green",                0.31f, 0.52f, 0.15f },
    { 18, "Forest green",               0.13f, 0.46f, 0.13f },
    { 19, "Bright blue",                0.09f, 0.51f, 0.85f },
    { 20, "Medium blue",                0.13f, 0.38f, 0.68f },
    { 21, "Dark blue",                  0.09f, 0.21f, 0.50f },
    { 22, "Navy blue",                  0.16f, 0.22f, 0.48f },
    { 23, "Pastel light blue",          0.45f, 0.77f, 0.96f },
    { 24, "Light blue",                 0.41f, 0.71f, 0.87f },
    { 25, "Cyan",                       0.19f, 0.76f, 0.81f },
    { 26, "Bright violet",              0.56f, 0.27f, 0.68f },
    { 27, "Med violet",                 0.38f, 0.22f, 0.53f },
    { 28, "Dark violet",                0.30f, 0.14f, 0.41f },
    { 29, "Brick yellow",               0.89f, 0.72f, 0.22f },
    { 30, "Bright orange",              0.95f, 0.56f, 0.14f },
    { 31, "Orange",                     0.90f, 0.44f, 0.12f },
    { 32, "Dark orange",                0.85f, 0.31f, 0.09f },
    { 33, "Gold",                       0.88f, 0.71f, 0.27f },
    { 34, "Bright yellow",              0.98f, 0.93f, 0.24f },
    { 35, "Bright purple",              0.67f, 0.33f, 0.85f },
    { 36, "Med purple",                 0.47f, 0.26f, 0.63f },
    { 37, "Dark purple",                0.34f, 0.17f, 0.46f },
    { 38, "Light purple",               0.84f, 0.52f, 0.90f },
    { 39, "Lilac",                      0.76f, 0.62f, 0.80f },
    { 40, "Magenta",                    0.85f, 0.26f, 0.68f },
    { 41, "Pink",                       0.98f, 0.41f, 0.68f },
    { 42, "Really blue",                0.12f, 0.35f, 0.86f },
    { 43, "Hot pink",                   1.00f, 0.23f, 0.52f },
    { 44, "Brown",                      0.49f, 0.30f, 0.16f },
    { 45, "Dark brown",                 0.34f, 0.23f, 0.14f },
    { 46, "Medium stone blue",          0.45f, 0.49f, 0.53f },
    { 47, "Dark stone blue",            0.32f, 0.36f, 0.40f },
    { 48, "Really bright blue",         0.20f, 0.63f, 1.00f },
    { 49, "Copper",                     0.74f, 0.42f, 0.25f },
    { 50, "Silver",                     0.75f, 0.76f, 0.76f },
    { 51, "Steel light",                0.55f, 0.60f, 0.65f },
    { 52, "Steel dark",                 0.43f, 0.47f, 0.51f },
    { 53, "Nickel",                     0.46f, 0.50f, 0.53f },
    { 54, "Glue",                       0.67f, 0.63f, 0.58f },
    { 55, "Light orange",               0.97f, 0.75f, 0.34f },
    { 56, "Caramel",                    0.87f, 0.62f, 0.33f },
    { 57, "Sand blue",                  0.42f, 0.61f, 0.85f },
    { 58, "Sand violet",                0.65f, 0.44f, 0.71f },
    { 59, "Sand yellow",                0.93f, 0.87f, 0.57f },
    { 60, "Light red",                  0.98f, 0.50f, 0.49f },
    { 61, "Sand green",                 0.48f, 0.71f, 0.42f },
    { 62, "Sand blue",                  0.57f, 0.82f, 0.84f },
    { 63, "Bittersweet",                0.94f, 0.58f, 0.65f },
    { 64, "Lava",                       0.89f, 0.34f, 0.24f },
    { 65, "Pastel violet",              0.76f, 0.62f, 0.80f },
    { 66, "Pastel blue",                0.55f, 0.75f, 0.90f },
    { 67, "Pastel green",               0.62f, 0.85f, 0.64f },
    { 68, "Pastel yellow",              0.97f, 0.94f, 0.59f },
    { 69, "Pastel orange",              1.00f, 0.75f, 0.50f },
    { 70, "Pastel brown",               0.90f, 0.76f, 0.64f },
    { 71, "Mint",                       0.67f, 0.92f, 0.83f },
    { 72, "Light orange",               0.97f, 0.75f, 0.34f },
    { 73, "Coral",                      0.86f, 0.36f, 0.46f },
    { 74, "Bright bluish green",        0.15f, 0.85f, 0.65f },
    { 75, "Dark bluish green",          0.17f, 0.51f, 0.42f },
    { 76, "Dark azure",                 0.20f, 0.44f, 0.72f },
    { 77, "Dark turquoise",             0.23f, 0.64f, 0.63f },
    { 78, "Bright green",               0.16f, 0.75f, 0.10f },
    { 79, "Yellowish orange",           0.99f, 0.64f, 0.23f },
    { 80, "Bright yellowish orange",    1.00f, 0.74f, 0.26f },
    { 81, "Bright orange",              0.95f, 0.56f, 0.14f },
    { 82, "Really orange",              1.00f, 0.54f, 0.16f },
    { 83, "Bright orange",              0.95f, 0.56f, 0.14f },
    { 84, "Bright purple",              0.67f, 0.33f, 0.85f },
    { 85, "Bright red",                 0.90f, 0.18f, 0.18f },
    { 86, "Bright yellow",              0.98f, 0.93f, 0.24f },
    { 87, "Bright green",               0.16f, 0.75f, 0.10f },
    { 88, "Bright blue",                0.09f, 0.51f, 0.85f },
    { 89, "Bright magenta",             0.98f, 0.25f, 0.71f },
    { 90, "Bright cyan",                0.20f, 0.85f, 0.85f },
    { 91, "Bright lime green",          0.40f, 0.92f, 0.27f },
    { 92, "Bright turquoise",           0.20f, 0.85f, 0.72f },
    { 93, "Bright violet",              0.56f, 0.27f, 0.68f },
    {100, "Really black",               0.09f, 0.09f, 0.09f },
    {101, "Pearlescent",                0.95f, 0.93f, 0.89f },
    {102, "Marble",                     0.85f, 0.83f, 0.80f },
    {103, "Fossilized",                 0.75f, 0.73f, 0.69f },
    {104, "Cloudy grey",                0.63f, 0.63f, 0.63f },
    {105, "Sand",                       0.90f, 0.80f, 0.62f },
    {106, "Toast",                      0.77f, 0.55f, 0.37f },
    {107, "Lemon",                      1.00f, 0.95f, 0.30f },
    {116, "Forest green",               0.13f, 0.46f, 0.13f },
    {117, "Ocean blue",                 0.16f, 0.32f, 0.68f },
    {118, "Royal purple",               0.38f, 0.22f, 0.53f },
    {119, "Dove grey",                  0.63f, 0.63f, 0.63f },
    {120, "Rust",                       0.70f, 0.29f, 0.12f },
    {121, "Tan",                        0.82f, 0.69f, 0.53f },
    {122, "Dark orange grey",           0.65f, 0.40f, 0.25f },
    {123, "Light blue",                 0.41f, 0.71f, 0.87f },
    {124, "Dark yellow",                0.77f, 0.74f, 0.22f },
    {125, "Medium yellow",              0.84f, 0.81f, 0.27f },
    {126, "Dark green",                 0.11f, 0.42f, 0.09f },
    {127, "Bright blue",                0.09f, 0.51f, 0.85f },
    {128, "Med blue",                   0.13f, 0.38f, 0.68f },
    {129, "Bright purple",              0.67f, 0.33f, 0.85f },
    {130, "Lime green",                 0.62f, 0.84f, 0.31f },
    {131, "Neon orange",                1.00f, 0.43f, 0.10f },
    {132, "Neon green",                 0.39f, 1.00f, 0.33f },
    {134, "Mid green",                  0.18f, 0.58f, 0.17f },
    {135, "Earth orange",               0.75f, 0.41f, 0.22f },
    {136, "Black",                      0.09f, 0.09f, 0.09f },
    {137, "Oyster",                     0.81f, 0.76f, 0.72f },
    {138, "Dirt brown",                 0.44f, 0.32f, 0.22f },
    {139, "Lemon",                      1.00f, 0.95f, 0.30f },
    {140, "Dark grey",                  0.35f, 0.35f, 0.35f },
    {141, "Really red",                 0.75f, 0.05f, 0.05f },
    {142, "Lily white",                 0.97f, 0.94f, 0.91f },
    {143, "Dark orange",                0.85f, 0.31f, 0.09f },
    {144, "Reddish orange",             0.94f, 0.37f, 0.14f },
    {145, "Bright orange brown",        0.88f, 0.46f, 0.16f },
    {146, "Gold",                       0.88f, 0.71f, 0.27f },
    {147, "Light orange",               0.97f, 0.75f, 0.34f },
    {148, "Warm yellowish orange",      0.99f, 0.64f, 0.23f },
    {149, "Yellowish orange",           0.99f, 0.64f, 0.23f },
    {150, "Light blue",                 0.41f, 0.71f, 0.87f },
    {151, "Really bright blue",         0.20f, 0.63f, 1.00f },
    {152, "Bright bluish green",        0.15f, 0.85f, 0.65f },
    {153, "Bright green",               0.16f, 0.75f, 0.10f },
    {154, "Dark orange",                0.85f, 0.31f, 0.09f },
    {155, "Really bright blue",         0.20f, 0.63f, 1.00f },
    {156, "Brick yellow",               0.89f, 0.72f, 0.22f },
    {157, "Yellow",                     0.91f, 0.91f, 0.32f },
    {158, "Bright yellow",              0.98f, 0.93f, 0.24f },
    {159, "Lime green",                 0.62f, 0.84f, 0.31f },
    {160, "Bright green",               0.16f, 0.75f, 0.10f },
    {161, "Light yellow",               0.93f, 0.93f, 0.70f },
    {162, "Earth green",                0.31f, 0.52f, 0.15f },
    {163, "Forest green",               0.13f, 0.46f, 0.13f },
    {164, "Dark stone blue",            0.32f, 0.36f, 0.40f },
    {165, "Medium stone blue",          0.45f, 0.49f, 0.53f },
    {166, "Bright blue",                0.09f, 0.51f, 0.85f },
    {167, "Med blue",                   0.13f, 0.38f, 0.68f },
    {168, "Really blue",                0.12f, 0.35f, 0.86f },
    {169, "Bright violet",              0.56f, 0.27f, 0.68f },
    {170, "Bright yellowish green",     0.70f, 0.92f, 0.27f },
    {171, "Bright yellowish orange",    1.00f, 0.74f, 0.26f },
    {172, "Really red",                 0.75f, 0.05f, 0.05f },
    {173, "Bright red",                 0.90f, 0.18f, 0.18f },
    {174, "Medium red",                 0.70f, 0.13f, 0.13f },
    {175, "Dark red",                   0.53f, 0.09f, 0.09f },
    {176, "Really black",               0.09f, 0.09f, 0.09f },
    {177, "Dark gray",                  0.35f, 0.35f, 0.35f },
    {178, "Medium gray",                0.55f, 0.55f, 0.55f },
    {179, "Bright gray",                0.75f, 0.75f, 0.75f },
    {180, "White",                      0.95f, 0.95f, 0.95f },
    {181, "Pastel blue",                0.55f, 0.75f, 0.90f },
    {182, "Pastel violet",              0.76f, 0.62f, 0.80f },
    {183, "Pastel green",               0.62f, 0.85f, 0.64f },
    {184, "Pastel yellow",              0.97f, 0.94f, 0.59f },
    {185, "Pastel orange",              1.00f, 0.75f, 0.50f },
    {186, "Pastel brown",               0.90f, 0.76f, 0.64f },
    {187, "Neon green",                 0.39f, 1.00f, 0.33f },
    {188, "Lime green",                 0.62f, 0.84f, 0.31f },
    {189, "Light orange",               0.97f, 0.75f, 0.34f },
    {190, "Caramel",                    0.87f, 0.62f, 0.33f },
    {191, "Sand blue",                  0.42f, 0.61f, 0.85f },
    {192, "Sand violet",                0.65f, 0.44f, 0.71f },
    {193, "Sand yellow",                0.93f, 0.87f, 0.57f },
    {194, "Light red",                  0.98f, 0.50f, 0.49f },
    {195, "Sand green",                 0.48f, 0.71f, 0.42f },
    {196, "Sand blue",                  0.57f, 0.82f, 0.84f },
    {197, "Bittersweet",                0.94f, 0.58f, 0.65f },
    {198, "Lava",                       0.89f, 0.34f, 0.24f },
    {199, "Bright bluish green",        0.15f, 0.85f, 0.65f },
    {200, "Bright yellowish green",     0.70f, 0.92f, 0.27f },
    {201, "Bright yellowish orange",    1.00f, 0.74f, 0.26f },
    {202, "Bright orange",              0.95f, 0.56f, 0.14f },
    {203, "Really orange",              1.00f, 0.54f, 0.16f },
    {204, "Bright purple",              0.67f, 0.33f, 0.85f },
    {205, "Really red",                 0.75f, 0.05f, 0.05f },
    {206, "Bright yellow",              0.98f, 0.93f, 0.24f },
    {207, "Bright green",               0.16f, 0.75f, 0.10f },
    {208, "Bright blue",                0.09f, 0.51f, 0.85f },
    {209, "Bright magenta",             0.98f, 0.25f, 0.71f },
    {210, "Bright cyan",                0.20f, 0.85f, 0.85f },
    {211, "Bright green",               0.16f, 0.75f, 0.10f },
    {212, "Bright turquoise",           0.20f, 0.85f, 0.72f },
    {213, "Bright violet",              0.56f, 0.27f, 0.68f },
    {214, "Bright purple",              0.67f, 0.33f, 0.85f },
    {220, "Med red",                    0.70f, 0.13f, 0.13f },
    {221, "Really black",               0.09f, 0.09f, 0.09f },
    {222, "Dark orange",                0.85f, 0.31f, 0.09f },
    {223, "Brick yellow",               0.89f, 0.72f, 0.22f },
    {224, "Bright orange",              0.95f, 0.56f, 0.14f },
    {225, "Bright yellow",              0.98f, 0.93f, 0.24f },
    {226, "Neon orange",                1.00f, 0.43f, 0.10f },
    {227, "Bright orange brown",        0.88f, 0.46f, 0.16f },
    {228, "Earth orange",               0.75f, 0.41f, 0.22f },
    {229, "Really red",                 0.75f, 0.05f, 0.05f },
    {230, "Dark orange brown",          0.65f, 0.40f, 0.25f },
    {231, "Copper",                     0.74f, 0.42f, 0.25f },
    {232, "Really red",                 0.75f, 0.05f, 0.05f },
    {233, "Really black",               0.09f, 0.09f, 0.09f },
    {234, "Dark orange",                0.85f, 0.31f, 0.09f },
    {235, "Brick yellow",               0.89f, 0.72f, 0.22f },
    {236, "Orange",                     0.90f, 0.44f, 0.12f },
    {237, "Bright orange",              0.95f, 0.56f, 0.14f },
    {238, "Bright orange",              0.95f, 0.56f, 0.14f },
    {239, "Really orange",              1.00f, 0.54f, 0.16f },
    {240, "Dark orange",                0.85f, 0.31f, 0.09f },
    {241, "Really black",               0.09f, 0.09f, 0.09f },
    {242, "Lava",                       0.89f, 0.34f, 0.24f },
    {243, "Bright red",                 0.90f, 0.18f, 0.18f },
    {244, "Bright orange",              0.95f, 0.56f, 0.14f },
    {245, "Bright yellowish green",     0.70f, 0.92f, 0.27f },
    {246, "Bright green",               0.16f, 0.75f, 0.10f },
    {247, "Bright bluish green",        0.15f, 0.85f, 0.65f },
    {248, "Bright turquoise",           0.20f, 0.85f, 0.72f },
    {249, "Bright blue",                0.09f, 0.51f, 0.85f },
    {250, "Bright violet",              0.56f, 0.27f, 0.68f },
    {251, "Bright purple",              0.67f, 0.33f, 0.85f },
    {252, "Bright magenta",             0.98f, 0.25f, 0.71f },
    {253, "Bright yellow",              0.98f, 0.93f, 0.24f },
    {254, "Bright cyan",                0.20f, 0.85f, 0.85f },
    {255, "Bright yellowish green",     0.70f, 0.92f, 0.27f },
};

static void brickColorFromNumber(int num, float& r, float& g, float& b, char nameOut[64]) {
    for (auto& bc : classicBrickColors) {
        if (bc.number == num) {
            r = bc.r; g = bc.g; b = bc.b;
            strncpy(nameOut, bc.name, 63); nameOut[63]=0;
            return;
        }
    }
    r = 0.5f; g = 0.5f; b = 0.5f;
    strncpy(nameOut, "Unknown", 63); nameOut[63]=0;
}

static void brickColorFromName(const std::string& name, float& r, float& g, float& b, char nameOut[64]) {
    for (auto& bc : classicBrickColors) {
        if (strcasecmp(bc.name, name.c_str()) == 0) {
            r = bc.r; g = bc.g; b = bc.b;
            strncpy(nameOut, bc.name, 63); nameOut[63]=0;
            return;
        }
    }
    r = 0.5f; g = 0.5f; b = 0.5f;
    strncpy(nameOut, name.c_str(), 63); nameOut[63]=0;
}

static int luaBrickColorNew(lua_State* L) {
    // BrickColor.new(number) or BrickColor.new("Name")
    float r=0.5f, g=0.5f, b=0.5f;
    char nameOut[64]="";
    int numResult = 0;

    if (lua_isnumber(L, 1)) {
        int num = (int)lua_tonumber(L, 1);
        brickColorFromNumber(num, r, g, b, nameOut);
        numResult = num;
    } else if (lua_isstring(L, 1)) {
        const char* name = lua_tostring(L, 1);
        brickColorFromName(std::string(name), r, g, b, nameOut);
        for (auto& bc : classicBrickColors) {
            if (strcasecmp(bc.name, name) == 0) { numResult = bc.number; break; }
        }
    }

    // Return table with properties
    lua_newtable(L);
    lua_pushnumber(L, r); lua_setfield(L, -2, "R");
    lua_pushnumber(L, g); lua_setfield(L, -2, "G");
    lua_pushnumber(L, b); lua_setfield(L, -2, "B");
    lua_pushstring(L, nameOut); lua_setfield(L, -2, "Name");
    lua_pushnumber(L, numResult); lua_setfield(L, -2, "Number");
    return 1;
}

// ============================================================================
// Roblox API: Color3
// ============================================================================

static int luaColor3New(lua_State* L) {
    // Color3.new(r, g, b) - values 0-1
    float r = (float)luaL_optnumber(L, 1, 0);
    float g = (float)luaL_optnumber(L, 2, 0);
    float b = (float)luaL_optnumber(L, 3, 0);
    r = fminf(fmaxf(r, 0), 1);
    g = fminf(fmaxf(g, 0), 1);
    b = fminf(fmaxf(b, 0), 1);
    
    float* c = new float[3]{r, g, b};
    float** ud = (float**)lua_newuserdata(L, sizeof(float*));
    *ud = c;
    luaL_getmetatable(L, "Color3");
    lua_setmetatable(L, -2);
    return 1;
}

static int luaColor3FromRGB(lua_State* L) {
    // Color3.fromRGB(r, g, b) - values 0-255
    int ri = (int)luaL_optnumber(L, 1, 0);
    int gi = (int)luaL_optnumber(L, 2, 0);
    int bi = (int)luaL_optnumber(L, 3, 0);
    
    float r = fminf(fmaxf((float)ri, 0), 255) / 255.0f;
    float g = fminf(fmaxf((float)gi, 0), 255) / 255.0f;
    float b = fminf(fmaxf((float)bi, 0), 255) / 255.0f;
    
    float* c = new float[3]{r, g, b};
    float** ud = (float**)lua_newuserdata(L, sizeof(float*));
    *ud = c;
    luaL_getmetatable(L, "Color3");
    lua_setmetatable(L, -2);
    return 1;
}

static int luaColor3FromHSV(lua_State* L) {
    // Color3.fromHSV(h, s, v)
    float h = (float)luaL_optnumber(L, 1, 0);
    float s = (float)luaL_optnumber(L, 2, 0);
    float v = (float)luaL_optnumber(L, 3, 1);
    
    // HSV to RGB conversion
    int hi = (int)floor(h * 6) % 6;
    float f = h * 6 - floor(h * 6);
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    
    float r, g, b;
    switch (hi) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = v; g = v; b = v; break;
    }
    
    float* c = new float[3]{r, g, b};
    float** ud = (float**)lua_newuserdata(L, sizeof(float*));
    *ud = c;
    luaL_getmetatable(L, "Color3");
    lua_setmetatable(L, -2);
    return 1;
}

// ============================================================================
// Roblox API: Players service
// ============================================================================

static int luaPlayersGetPlayers(lua_State* L) {
    // Players:GetPlayers()
    lua_newtable(L);
    // Return empty table for now (no player system yet)
    return 1;
}

static int luaPlayersGetLocalPlayer(lua_State* L) {
    // Players.LocalPlayer
    // Return nil for server scripts
    return 0;
}

// ============================================================================
// Roblox API: RunService
// ============================================================================

static int luaRunServiceIsServer(lua_State* L) {
    // RunService:IsServer()
    lua_pushboolean(L, 1); // In OpenBLOX, we're the server
    return 1;
}

static int luaRunServiceIsClient(lua_State* L) {
    // RunService:IsClient()
    lua_pushboolean(L, 0);
    return 1;
}

// ============================================================================
// Roblox API: SoundService
// ============================================================================

static int luaSoundServicePlaySound(lua_State* L) {
    // SoundService:PlaySound(soundId) - Legacy API
    const char* soundId = lua_tostring(L, 2);
    if (soundId) {
        printf("[LUA] Playing sound: %s\n", soundId);
        // AudioManager::playSound not available on 3DS
    }
    return 0;
}

static int luaSoundServiceStopSound(lua_State* L) {
    // SoundService:StopSound(soundId)
    const char* soundId = lua_tostring(L, 2);
    if (soundId) {
        printf("[LUA] Stopping sound: %s\n", soundId);
    }
    return 0;
}

// ============================================================================
// Roblox API: Sound instance
// ============================================================================

static int luaSoundPlay(lua_State* L) {
    // sound:Play()
    Node* node = getInstanceFromLua(L, 1);
    if (node) {
        node->playing = 1;
        printf("[LUA] Sound play: %s\n", node->name);
    }
    return 0;
}

static int luaSoundStop(lua_State* L) {
    // sound:Stop()
    Node* node = getInstanceFromLua(L, 1);
    if (node) {
        node->playing = 0;
        printf("[LUA] Sound stop: %s\n", node->name);
    }
    return 0;
}

// ============================================================================
// Roblox API: Humanoid
// ============================================================================

static int luaHumanoidTakeDamage(lua_State* L) {
    // humanoid:TakeDamage(amount)
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    
    float damage = (float)luaL_checknumber(L, 2);
    humanoid->health -= damage;
    if (humanoid->health < 0) humanoid->health = 0;
    
    printf("[LUA] Humanoid %s took %.1f damage, health: %.1f\n", 
           humanoid->name, damage, humanoid->health);
    
    if (humanoid->health <= 0) {
        // Trigger Died event
        printf("[LUA] Humanoid %s died!\n", humanoid->name);
    }
    return 0;
}

static int luaHumanoidGetHealth(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    lua_pushnumber(L, humanoid->health);
    return 1;
}

static int luaHumanoidSetHealth(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    humanoid->health = (float)luaL_checknumber(L, 2);
    return 0;
}

static int luaHumanoidGetMaxHealth(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    lua_pushnumber(L, humanoid->maxHealth);
    return 1;
}

static int luaHumanoidSetMaxHealth(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    humanoid->maxHealth = (float)luaL_checknumber(L, 2);
    return 0;
}

static int luaHumanoidGetWalkSpeed(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    lua_pushnumber(L, humanoid->walkSpeed);
    return 1;
}

static int luaHumanoidSetWalkSpeed(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    humanoid->walkSpeed = (float)luaL_checknumber(L, 2);
    return 0;
}

static int luaHumanoidGetJumpPower(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    lua_pushnumber(L, humanoid->jumpPower);
    return 1;
}

static int luaHumanoidSetJumpPower(lua_State* L) {
    Node* humanoid = getInstanceFromLua(L, 1);
    if (!humanoid) return 0;
    humanoid->jumpPower = (float)luaL_checknumber(L, 2);
    return 0;
}

// ============================================================================
// Roblox API: Lighting
// ============================================================================

static char ambientLight[64];
static float clockTime = 12.0f;
static float lightBrightness = 1.0f;

static int luaLightingGetAmbient(lua_State* L) {
    // Lighting.Ambient
    const char* ambient = ambientLight;
    if (ambient) {
        lua_pushstring(L, ambient);
    }
    return 1;
}

static int luaLightingSetAmbient(lua_State* L) {
    // Lighting.Ambient
    const char* color = lua_tostring(L, 2);
    if (color) {
        strncpy(ambientLight, color, sizeof(ambientLight) - 1);
    }
    return 0;
}

static int luaLightingGetClockTime(lua_State* L) {
    // Lighting.ClockTime
    lua_pushnumber(L, clockTime);
    return 1;
}

static int luaLightingSetClockTime(lua_State* L) {
    // Lighting.ClockTime
    clockTime = (float)luaL_checknumber(L, 2);
    return 0;
}

static int luaLightingGetBrightness(lua_State* L) {
    // Lighting.Brightness
    lua_pushnumber(L, lightBrightness);
    return 1;
}

static int luaLightingSetBrightness(lua_State* L) {
    // Lighting.Brightness
    lightBrightness = (float)luaL_checknumber(L, 2);
    return 0;
}

// ============================================================================
// Roblox API: Attributes system
// ============================================================================

static int luaInstanceGetAttribute(lua_State* L) {
    // instance:GetAttribute(name)
    Node* node = getInstanceFromLua(L, 1);
    const char* name = lua_tostring(L, 2);
    if (!node || !name) return 0;
    
    auto it = node->attributes.find(name);
    if (it != node->attributes.end()) {
        // Return attribute value (simplified)
        lua_pushnumber(L, it->second);
        return 1;
    }
    return 0;
}

static int luaInstanceSetAttribute(lua_State* L) {
    // instance:SetAttribute(name, value)
    Node* node = getInstanceFromLua(L, 1);
    const char* name = lua_tostring(L, 2);
    if (!node || !name) return 0;
    
    if (lua_isnumber(L, 3)) {
        node->attributes[name] = (double)lua_tonumber(L, 3);
    } else if (lua_isstring(L, 3)) {
        // Store string as attribute (convert to double representation)
        node->attributes[name] = 0;
    }
    return 0;
}

std::string getLastErrorMessage() {
    return lastErrorMessage;
}

void setScriptTimeout(double timeout) {
    scriptTimeout = timeout;
}

double getScriptTimeout() {
    return scriptTimeout;
}

void setScriptsEnabled(bool enabled) {
    scriptsEnabled = enabled;
}

bool areScriptsEnabled() {
    return scriptsEnabled;
}

void setCurrentScript(Node* scriptNode) {
    currentScriptNode = scriptNode;
}

Node* getCurrentScript() {
    return currentScriptNode;
}

// ============================================================================
// Roblox API: Threading (spawn, delay)
// ============================================================================

static std::vector<std::pair<std::chrono::steady_clock::time_point, lua_CFunction>> delayedCalls;
static std::vector<std::thread> spawnedThreads;

static int luaSpawn(lua_State* L) {
    // spawn(function) - Run function in a separate thread
    if (!lua_isfunction(L, 1)) return 0;
    
    // Duplicate the function for the new thread
    lua_pushvalue(L, 1);
    int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    std::thread t([funcRef, L]() {
        lua_State* mainL = L;
        lua_rawgeti(mainL, LUA_REGISTRYINDEX, funcRef);
        if (lua_pcall(mainL, 0, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(mainL, -1);
            printf("[LUA SPAWN ERROR] %s\n", err);
            lua_pop(mainL, 1);
        }
        luaL_unref(mainL, LUA_REGISTRYINDEX, funcRef);
    });
    t.detach();
    
    return 0;
}

static int luaDelay(lua_State* L) {
    // delay(seconds, function) - Call function after delay
    double seconds = luaL_checknumber(L, 1);
    if (!lua_isfunction(L, 2)) return 0;
    
    // Duplicate the function
    lua_pushvalue(L, 2);
    int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    auto triggerTime = std::chrono::steady_clock::now() + std::chrono::milliseconds((int)(seconds * 1000));
    delayedCalls.push_back({triggerTime, nullptr});
    
    std::thread t([=]() {
        auto now = std::chrono::steady_clock::now();
        if (now < triggerTime) {
            std::this_thread::sleep_for(triggerTime - now);
        }
        
        // Execute the function
        lua_State* mainL = L;
        lua_rawgeti(mainL, LUA_REGISTRYINDEX, funcRef);
        if (lua_pcall(mainL, 0, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(mainL, -1);
            printf("[LUA DELAY ERROR] %s\n", err);
            lua_pop(mainL, 1);
        }
        luaL_unref(mainL, LUA_REGISTRYINDEX, funcRef);
    });
    t.detach();
    
    return 0;
}

// ============================================================================
// Roblox API: Module loading (require)
// ============================================================================

static std::unordered_map<int, std::string> moduleCache;  // module ref -> source

static int luaRequire(lua_State* L) {
    // require(module) - Load a ModuleScript and return its result
    if (lua_isstring(L, 1)) {
        // require("path/to/module")
        const char* modulePath = lua_tostring(L, 1);
        std::string path = modulePath;
        
        // Check cache
        for (auto& cacheEntry : moduleCache) {
            if (cacheEntry.second == path) {
                // Push cached result
                lua_rawgeti(L, LUA_REGISTRYINDEX, cacheEntry.first);
                return 1;
            }
        }
        
        // Try to load and execute
        if (luaL_loadfile(L, path.c_str())) {
            const char* err = lua_tostring(L, -1);
            printf("[LUA REQUIRE ERROR] Cannot load module: %s - %s\n", path.c_str(), err);
            lua_pop(L, 1);
            return 0;
        }
        
        if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            printf("[LUA REQUIRE ERROR] Module execution error: %s\n", err);
            lua_pop(L, 1);
            return 0;
        }
        
        // Cache the result
        int resultRef = luaL_ref(L, LUA_REGISTRYINDEX);
        moduleCache[resultRef] = path;
        
        // Push cached result
        lua_rawgeti(L, LUA_REGISTRYINDEX, resultRef);
        return 1;
    } else if (lua_isuserdata(L, 1)) {
        // require(script) - Load a ModuleScript Node
        Node* node = getInstanceFromLua(L, 1);
        if (node && node->source) {
            // Check if already executed
            if (node->moduleResultRef >= 0) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, node->moduleResultRef);
                return 1;
            }
            
            // Execute module source
            if (luaL_loadbuffer(L, node->source, strlen(node->source), node->name)) {
                const char* err = lua_tostring(L, -1);
                printf("[LUA REQUIRE ERROR] Cannot load module: %s\n", err);
                lua_pop(L, 1);
                return 0;
            }
            
            if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
                const char* err = lua_tostring(L, -1);
                printf("[LUA REQUIRE ERROR] Module execution error: %s\n", err);
                lua_pop(L, 1);
                return 0;
            }
            
            // Cache result in node
            node->moduleResultRef = luaL_ref(L, LUA_REGISTRYINDEX);
            lua_rawgeti(L, LUA_REGISTRYINDEX, node->moduleResultRef);
            return 1;
        }
    }
    
    return 0;
}

// ============================================================================
// Roblox API: HttpService (JSONDecode, JSONEncode)
// ============================================================================

// Minimal JSON parser for Lua tables
static void jsonDecodeToLuaTable(lua_State* L, const char* json) {
    // Very basic JSON decoder (supports tables, arrays, strings, numbers, booleans, null)
    // For production, replace with cjson or similar
    int len = strlen(json);
    int pos = 0;
    
    // Skip whitespace
    while (pos < len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
        pos++;
    
    if (pos >= len) {
        lua_pushnil(L);
        return;
    }
    
    if (json[pos] == '[') {
        // Array
        lua_newtable(L);
        pos++; // skip [
        int idx = 1;
        while (pos < len) {
            while (pos < len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
                pos++;
            if (pos >= len || json[pos] == ']') break;
            
            // Parse value
            if (json[pos] == '"') {
                // String
                pos++;
                std::string str;
                while (pos < len && json[pos] != '"') {
                    if (json[pos] == '\\' && pos + 1 < len) {
                        pos++;
                        if (json[pos] == 'n') str += '\n';
                        else if (json[pos] == 't') str += '\t';
                        else if (json[pos] == 'r') str += '\r';
                        else str += json[pos];
                    } else {
                        str += json[pos];
                    }
                    pos++;
                }
                pos++; // skip closing "
                lua_pushstring(L, str.c_str());
            } else if (json[pos] == '{' || json[pos] == '[') {
                // Nested table/array - recursive parse
                int depth = 1;
                int start = pos;
                char opener = json[pos];
                char closer = (opener == '{') ? '}' : ']';
                pos++;
                while (pos < len && depth > 0) {
                    if (json[pos] == opener) depth++;
                    else if (json[pos] == closer) depth--;
                    pos++;
                }
                std::string nestedJson(json + start, pos - start);
                jsonDecodeToLuaTable(L, nestedJson.c_str());
            } else if (json[pos] == 't' && strncmp(json + pos, "true", 4) == 0) {
                lua_pushboolean(L, 1);
                pos += 4;
            } else if (json[pos] == 'f' && strncmp(json + pos, "false", 5) == 0) {
                lua_pushboolean(L, 0);
                pos += 5;
            } else if (json[pos] == 'n' && strncmp(json + pos, "null", 4) == 0) {
                lua_pushnil(L);
                pos += 4;
            } else {
                // Number
                int numStart = pos;
                while (pos < len && (json[pos] >= '0' && json[pos] <= '9' || json[pos] == '.' || json[pos] == '-' || json[pos] == '+' || json[pos] == 'e' || json[pos] == 'E'))
                    pos++;
                std::string numStr(json + numStart, pos - numStart);
                lua_pushnumber(L, strtod(numStr.c_str(), nullptr));
            }
            
            lua_rawseti(L, -2, idx++);
            
            while (pos < len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
                pos++;
            if (json[pos] == ',') pos++;
        }
    } else if (json[pos] == '{') {
        // Object
        lua_newtable(L);
        pos++; // skip {
        while (pos < len) {
            while (pos < len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
                pos++;
            if (pos >= len || json[pos] == '}') break;
            
            // Parse key
            if (json[pos] == '"') {
                pos++;
                std::string key;
                while (pos < len && json[pos] != '"') {
                    if (json[pos] == '\\' && pos + 1 < len) {
                        pos++;
                        if (json[pos] == 'n') key += '\n';
                        else if (json[pos] == 't') key += '\t';
                        else if (json[pos] == 'r') key += '\r';
                        else key += json[pos];
                    } else {
                        key += json[pos];
                    }
                    pos++;
                }
                pos++; // skip closing "
                
                while (pos < len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
                    pos++;
                if (json[pos] == ':') pos++; // skip :
                while (pos < len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
                    pos++;
                
                // Parse value
                if (json[pos] == '"') {
                    pos++;
                    std::string str;
                    while (pos < len && json[pos] != '"') {
                        if (json[pos] == '\\' && pos + 1 < len) {
                            pos++;
                            if (json[pos] == 'n') str += '\n';
                            else if (json[pos] == 't') str += '\t';
                            else if (json[pos] == 'r') str += '\r';
                            else str += json[pos];
                        } else {
                            str += json[pos];
                        }
                        pos++;
                    }
                    pos++;
                    lua_pushstring(L, key.c_str());
                    lua_pushstring(L, str.c_str());
                } else if (json[pos] == '{' || json[pos] == '[') {
                    int depth = 1;
                    int start = pos;
                    char opener = json[pos];
                    char closer = (opener == '{') ? '}' : ']';
                    pos++;
                    while (pos < len && depth > 0) {
                        if (json[pos] == opener) depth++;
                        else if (json[pos] == closer) depth--;
                        pos++;
                    }
                    std::string nestedJson(json + start, pos - start);
                    lua_pushstring(L, key.c_str());
                    jsonDecodeToLuaTable(L, nestedJson.c_str());
                } else if (json[pos] == 't' && strncmp(json + pos, "true", 4) == 0) {
                    lua_pushstring(L, key.c_str());
                    lua_pushboolean(L, 1);
                    pos += 4;
                } else if (json[pos] == 'f' && strncmp(json + pos, "false", 5) == 0) {
                    lua_pushstring(L, key.c_str());
                    lua_pushboolean(L, 0);
                    pos += 5;
                } else if (json[pos] == 'n' && strncmp(json + pos, "null", 4) == 0) {
                    lua_pushstring(L, key.c_str());
                    lua_pushnil(L);
                    pos += 4;
                } else {
                    int numStart = pos;
                    while (pos < len && (json[pos] >= '0' && json[pos] <= '9' || json[pos] == '.' || json[pos] == '-' || json[pos] == '+' || json[pos] == 'e' || json[pos] == 'E'))
                        pos++;
                    std::string numStr(json + numStart, pos - numStart);
                    lua_pushstring(L, key.c_str());
                    lua_pushnumber(L, strtod(numStr.c_str(), nullptr));
                }
                
                lua_settable(L, -3);
            } else {
                pos++; // skip invalid char
            }
            
            while (pos < len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
                pos++;
            if (json[pos] == ',') pos++;
        }
    } else if (json[pos] == '"') {
        // String
        pos++;
        std::string str;
        while (pos < len && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < len) {
                pos++;
                if (json[pos] == 'n') str += '\n';
                else if (json[pos] == 't') str += '\t';
                else if (json[pos] == 'r') str += '\r';
                else str += json[pos];
            } else {
                str += json[pos];
            }
            pos++;
        }
        pos++;
        lua_pushstring(L, str.c_str());
    } else if (json[pos] == 't' && strncmp(json + pos, "true", 4) == 0) {
        lua_pushboolean(L, 1);
    } else if (json[pos] == 'f' && strncmp(json + pos, "false", 5) == 0) {
        lua_pushboolean(L, 0);
    } else if (json[pos] == 'n' && strncmp(json + pos, "null", 4) == 0) {
        lua_pushnil(L);
    } else {
        // Number
        int numStart = pos;
        while (pos < len && (json[pos] >= '0' && json[pos] <= '9' || json[pos] == '.' || json[pos] == '-' || json[pos] == '+' || json[pos] == 'e' || json[pos] == 'E'))
            pos++;
        std::string numStr(json + numStart, pos - numStart);
        lua_pushnumber(L, strtod(numStr.c_str(), nullptr));
    }
}

static int luaHttpServiceJSONDecode(lua_State* L) {
    // HttpService:JSONDecode(json_string)
    const char* json = lua_tostring(L, 2);
    if (!json) return 0;
    
    jsonDecodeToLuaTable(L, json);
    return 1;
}

static int luaHttpServiceJSONEncode(lua_State* L) {
    // HttpService:JSONEncode(lua_value) - Basic JSON encoder
    // For simplicity, return a placeholder
    lua_pushstring(L, "{}");
    return 1;
}

// ============================================================================
// Roblox API: Sound properties (Pitch, TimePosition, IsPlaying)
// ============================================================================

static int luaSoundGetPitch(lua_State* L) {
    // Sound.Pitch getter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    lua_pushnumber(L, node->soundPitch);
    return 1;
}

static int luaSoundSetPitch(lua_State* L) {
    // Sound.Pitch setter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->soundPitch = (float)luaL_checknumber(L, 2);
    return 0;
}

static int luaSoundGetTimePosition(lua_State* L) {
    // Sound.TimePosition getter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    lua_pushnumber(L, node->soundTimePosition);
    return 1;
}

static int luaSoundSetTimePosition(lua_State* L) {
    // Sound.TimePosition setter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->soundTimePosition = (float)luaL_checknumber(L, 2);
    return 0;
}

static int luaSoundGetIsPlaying(lua_State* L) {
    // Sound.IsPlaying getter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    lua_pushboolean(L, node->playing);
    return 1;
}

static int luaSoundResume(lua_State* L) {
    // sound:Resume() - Resume a paused sound
    Node* node = getInstanceFromLua(L, 1);
    if (node) {
        node->playing = 1;
        node->soundPaused = 0;
        printf("[LUA] Sound resume: %s\n", node->name);
    }
    return 0;
}

static int luaSoundPause(lua_State* L) {
    // sound:Pause() - Pause a playing sound
    Node* node = getInstanceFromLua(L, 1);
    if (node) {
        node->soundPaused = 1;
        printf("[LUA] Sound pause: %s\n", node->name);
    }
    return 0;
}

static int luaSoundGetVolume(lua_State* L) {
    // Sound.Volume getter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    lua_pushnumber(L, node->soundVolume);
    return 1;
}

static int luaSoundSetVolume(lua_State* L) {
    // Sound.Volume setter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    node->soundVolume = (float)luaL_checknumber(L, 2);
    return 0;
}

static int luaSoundGetSoundId(lua_State* L) {
    // Sound.SoundId getter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    if (node->soundId) {
        lua_pushstring(L, node->soundId);
        return 1;
    }
    return 0;
}

static int luaSoundSetSoundId(lua_State* L) {
    // Sound.SoundId setter
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    const char* soundId = lua_tostring(L, 2);
    if (soundId) {
        if (node->soundId) free(node->soundId);
        node->soundId = strdup(soundId);
    }
    return 0;
}

// ============================================================================
// Roblox API: Script indexed child access (script[name])
// ============================================================================

static int luaScriptIndex(lua_State* L) {
    // __index metamethod for script object
    // script.ChildName or script["ChildName"]
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    const char* key = lua_tostring(L, 2);
    if (!key) return 0;
    
    // Check if it's a child node
    for (Node* child : node->children) {
        if (strcmp(child->name, key) == 0) {
            Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
            *ud = child;
            luaL_getmetatable(L, "Instance");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
    
    // Check properties
    if (strcmp(key, "Parent") == 0) {
        if (node->parent) {
            Node** ud = (Node**)lua_newuserdata(L, sizeof(Node*));
            *ud = node->parent;
            luaL_getmetatable(L, "Instance");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
    
    if (strcmp(key, "Name") == 0) {
        lua_pushstring(L, node->name);
        return 1;
    }
    
    if (strcmp(key, "ClassName") == 0) {
        lua_pushstring(L, node->type);
        return 1;
    }
    
    if (strcmp(key, "source") == 0) {
        if (node->source) {
            lua_pushstring(L, node->source);
            return 1;
        }
    }
    
    return 0;
}

static int luaScriptNewIndex(lua_State* L) {
    // __newindex metamethod for script object
    // script.ChildName = value
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    const char* key = lua_tostring(L, 2);
    if (!key) return 0;
    
    if (strcmp(key, "Parent") == 0) {
        Node* newParent = getInstanceFromLua(L, 3);
        if (newParent) {
            if (node->parent) {
                for (auto it = node->parent->children.begin(); it != node->parent->children.end(); ++it) {
                    if (*it == node) {
                        node->parent->children.erase(it);
                        break;
                    }
                }
            }
            node->SetParent(newParent);
        } else {
            // Remove from parent
            if (node->parent) {
                for (auto it = node->parent->children.begin(); it != node->parent->children.end(); ++it) {
                    if (*it == node) {
                        node->parent->children.erase(it);
                        break;
                    }
                }
                node->parent = nullptr;
            }
        }
        return 0;
    }
    
    if (strcmp(key, "Name") == 0) {
        const char* name = lua_tostring(L, 3);
        if (name) {
            strncpy(node->name, name, sizeof(node->name) - 1);
            node->name[sizeof(node->name) - 1] = '\0';
        }
        return 0;
    }
    
    return 0;
}

// ============================================================================
// Roblox API: RunService.Heartbeat event
// ============================================================================

static int luaHeartbeatWait(lua_State* L);
static int luaHeartbeatConnect(lua_State* L);

static int luaRunServiceGetHeartbeat(lua_State* L) {
    // RunService.Heartbeat getter - returns the Heartbeat RBXScriptSignal
    Node* node = getInstanceFromLua(L, 1);
    if (!node) return 0;
    
    // Create/get the Heartbeat signal
    lua_newtable(L);
    lua_pushcfunction(L, &luaHeartbeatWait);
    lua_setfield(L, -2, "wait");
    lua_pushcfunction(L, &luaHeartbeatConnect);
    lua_setfield(L, -2, "Connect");
    return 1;
}

static int luaHeartbeatWait(lua_State* L) {
    // Heartbeat:wait() - Wait for next frame
    // In Roblox, Heartbeat fires after physics but before rendering
    // We simulate this by yielding for a small duration
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    return 0;
}

static int luaHeartbeatConnect(lua_State* L) {
    // Heartbeat:Connect(callback) - Connect callback to Heartbeat event
    if (!lua_isfunction(L, 2)) return 0;
    
    lua_pushvalue(L, 2);
    int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
    
    // For now, just acknowledge the connection
    // A proper implementation would store and call the callback each frame
    lua_pushstring(L, "RBXScriptConnection");
    return 1;
}

}  // namespace ScriptEngine

// ============================================================================
// Helper Functions
// ============================================================================

bool isLuaFile(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) return false;
    std::string ext = path.substr(pos);
    return ext == ".lua" || ext == ".Lua" || ext == ".LUA" ||
           ext == ".luau" || ext == ".Luau" || ext == ".LUAU";
}

std::string getLuaExtension(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    return path.substr(pos);
}
