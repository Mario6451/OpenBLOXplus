#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Icon loading system for OpenBLOX
// Loads PNG icons from assets/MainMenuIcons/ folder
// Auto-resizes icons that are too big for their slot
// Falls back to colored squares if icons not found

namespace IconLoader {

    struct Icon {
        uint8_t* data;      // RGBA pixel data
        int width;
        int height;
        std::string path;
    };

    // Initialize icon loader
    void init();

    // Shutdown and free all icons
    void shutdown();

    // Load icon by name
    // Checks assets/MainMenuIcons/ first, then existing texture paths
    // If icon is larger than maxWidth/maxHeight, it will be scaled down
    Icon* loadIcon(const std::string& name, int maxWidth = 48, int maxHeight = 48);

    // Free a loaded icon
    void freeIcon(Icon* icon);

    // Check if icon exists
    bool iconExists(const std::string& name);

    // Get icon path (without loading)
    std::string getIconPath(const std::string& name);

    // Draw icon at position
    // If icon is null, draws colored square fallback
    void drawIcon(const Icon* icon, int x, int y, int fallbackColor = 100);

    // Draw icon with auto-resize to fit slot
    void drawIconScaled(const Icon* icon, int x, int y, int slotWidth, int slotHeight, int fallbackColor = 100);

    // Load all default menu icons
    // Returns number of icons loaded
    int loadDefaultIcons();

    // Get icon by name (cached)
    Icon* getIcon(const std::string& name);

    // Clear icon cache
    void clearCache();

    // Predefined icon names for main menu
    static const char* ICON_SETTINGS = "settings";
    static const char* ICON_AVATAR_EDITOR = "avatar_editor";
    static const char* ICON_STUDIO = "studio";
    static const char* ICON_MULTIPLAYER = "multiplayer";
    static const char* ICON_QUIT = "quit";
    static const char* ICON_VOICE_CHAT = "microphone";
    static const char* ICON_SPEAKER = "speaker";
}