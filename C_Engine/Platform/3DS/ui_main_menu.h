#pragma once

#include <vector>
#include <string>

// Game entry for main menu (defined outside namespace for use in cpp files)
struct GameEntry {
    std::string path;
    std::string displayName;
    bool isFolder;
};

namespace UIMainMenu {
    enum class MenuState {
        Main,
        Settings,
        Multiplayer,
        AvatarEditor,
        Studio,
        Quit
    };

    // Initialize the main menu UI
    void init();
    
    // Run main menu loop. Returns selected game path (empty if quit, "STUDIO" if studio mode)
    std::string runMenu();
    
    // Draw the main menu to top screen
    void drawMainMenu();
    
    // Draw game list on bottom screen
    void drawGameList(const std::vector<GameEntry>& entries);
    
    // Draw the redesigned main menu (320x240 bottom screen)
    void drawMainUI();
    
    // Scan directory for games
    std::vector<GameEntry> scanDirectory(const std::string& path);
    
    // Scan directory recursively for games
    std::vector<GameEntry> scanDirectoryRecursive(const std::string& path);
    
    // Filter entries by search text
    std::vector<GameEntry> filterBySearch(const std::vector<GameEntry>& entries, const std::string& search);
    
    // Run Studio editor mode. Returns "" when user exits Studio.
    void runStudio(const std::string& placePath);
    
    // Run multiplayer menu
    void runMultiplayerMenu();
}
