#pragma once
#include <string>
#include <vector>
#include <dirent.h>

// GameEntry defined in ui_main_menu.h — forward declare to avoid duplication
struct GameEntry;

struct ServerEntry {
    std::string ip;
    std::string name;
    std::string map;
    int players = 0;
    int maxPlayers = 8;
    int port = 27016;
};

enum class MultiTab { Host, Join };

class UIMultiplayer {
public:
    void init();
    std::string run();
    std::string getSelectedMap() const { return mapName; }
    int getPort() const { return portNum; }

private:
    // Drawing
    void drawMenu();
    void drawHostTab();
    void drawJoinTab();
    void drawTextInput(const char* label, int x, int y, int w);
    void drawServerList();
    void drawGameList();
    void drawScrollbar();
    void drawIcon(const char* name, int x, int y, int color);

    // Input
    void processInput();
    void showKeyboard();
    void hideKeyboard();

    // Game list helpers
    std::vector<GameEntry> scanDirectory(const std::string& path, const std::string& search);
    std::vector<GameEntry> scanRecursive(const std::string& path, const std::string& search, int depth);
    std::string getBaseName(const std::string& path);

    // State
    MultiTab activeTab = MultiTab::Host;

    // Host tab
    std::string ipAddress;
    std::string portStr;
    int portNum = 27016;
    std::string mapName;
    std::string userId;

    // Join tab
    std::string username;
    std::vector<ServerEntry> serverList;
    int selectedServerIndex = 0;

    // Game list (shared between tabs)
    std::vector<GameEntry> gameEntries;
    int selectedGameIndex = 0;
    int gameScrollOffset = 0;
    std::string gameSearch;
    std::string gameBrowsePath = "sdmc:/OpenBLOX/";
    std::vector<std::string> gamePathHistory;

    // Common
    int serverScrollOffset = 0;
    int activeInput = 0; // 0-3=fields, 4=game_list, 5=server_list
    bool keyboardShow = false;
    bool hostPressed = false;
    std::string selectedServer;
};

extern UIMultiplayer g_uiMultiplayer;