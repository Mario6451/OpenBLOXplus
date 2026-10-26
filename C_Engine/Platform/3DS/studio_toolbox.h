#pragma once

#include <string>
#include <vector>
#include <3ds.h>

// Studio Toolbox with Proxy Support
// Fetches assets (Parts, Models, Audio, Decals) from the Toolbox Proxy configured in Settings

namespace StudioToolbox {

    enum class ToolboxCategory {
        Parts,
        Models,
        Audio,
        Decals,
        Scripts,
        Count
    };

    struct ToolboxAsset {
        std::string id;
        std::string name;
        std::string thumbnail;
        ToolboxCategory category;
        bool isSelected;
    };

    // Initialize toolbox (fetches proxy URL from settings)
    void init();

    // Draw Toolbox UI on bottom screen
    void drawToolbox();

    // Process input for Toolbox
    void processInput();

    // Get selected asset
    const ToolboxAsset* getSelectedAsset();

    // Insert selected asset into workspace
    void insertSelectedAsset(void* dataModel);

    // Toggle toolbox visibility
    bool isToolboxVisible();
    void toggleToolbox();

    // Shutdown
    void shutdown();

} // namespace StudioToolbox