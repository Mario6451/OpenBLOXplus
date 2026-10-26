#pragma once
#include <string>
#include <vector>
#include <map>
#include "settings.h"

// Year-specific UI asset loading system
// Loads UI textures, loading screens, and particles based on content year
// Falls back to text-based UI for years without UI folders (2007, 2012)

namespace UIAssetLoader {

    struct UIAsset {
        std::string name;
        std::string path;
        int width = 0, height = 0;
        uint8_t* data = nullptr; // RGBA pixel data
    };

    // Initialize asset loader
    void init();

    // Shutdown and free all assets
    void shutdown();

    // Set current content year
    void setContentYear(Settings::ContentYear year);

    // Get current content year
    Settings::ContentYear getContentYear();

    // Reload all assets for current year
    void reloadAllAssets();

    // Load UI asset by name from year-specific folder
    // e.g., "SettingsButton" -> assets/ui2015/SettingsButton.png
    UIAsset* loadUIAsset(const std::string& name);

    // Load loading screen assets
    void loadLoadingScreen();

    // Get loading screen path
    std::string getLoadingScreenPath();

    // Load particle assets
    void loadParticles();

    // Get particles path
    std::string getParticlesPath();

    // Check if current year has UI assets
    bool hasUIAssets();

    // Check if current year has loading assets
    bool hasLoadingAssets();

    // Check if current year has particle assets
    bool hasParticles();
    
    // Draw text-based UI element (fallback for 2007/2012)
    void drawTextButton(const std::string& text, int x, int y, int w, int h, bool hovered = false);
    void drawTextField(const std::string& label, const std::string& value, int x, int y, int w, bool active = false);
    void drawPanel(int x, int y, int w, int h);

    // Get year-specific path
    std::string getUIPath(const std::string& assetName);
    std::string getLoadingPath(const std::string& assetName);
    std::string getParticlePath(const std::string& assetName);
    std::string getGameTexturePath(const std::string& assetName);

    // Convert Settings::ContentYear to folder name string
    std::string yearToFolderName(Settings::ContentYear year);

    // Get game content path for year (e.g., content2015/textures/)
    std::string getGameContentPath(Settings::ContentYear year);
};
