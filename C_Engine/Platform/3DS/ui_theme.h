#pragma once
#include <string>
#include <map>
#include <vector>
#include <3ds.h>

// Year-based UI theming system for OpenBLOX
// Changes UI appearance (colors, fonts, styles) based on selected content year

namespace ThemeManager {

    // UI Theme presets per Roblox year
    struct UITheme {
        // Panel colors
        u32 panelBg;            // Panel background
        u32 panelBorder;       // Panel border
        u32 panelTitleBg;      // Title bar background
        u32 panelTitleText;    // Title bar text

        // Button colors
        u32 buttonBg;          // Button background
        u32 buttonHover;       // Button hover state
        u32 buttonPressed;     // Button pressed state
        u32 buttonText;        // Button text color
        u32 buttonBorder;      // Button border

        // Text colors
        u32 textPrimary;       // Primary text
        u32 textSecondary;     // Secondary text
        u32 textDisabled;      // Disabled text
        u32 textHighlight;     // Highlighted/selected text

        // Health bar / progress bar
        u32 progressBarBg;     // Progress bar background
        u32 progressBarFg;     // Progress bar foreground

        // Hotbar / backpack
        u32 hotbarBg;          // Hotbar background
        u32 hotbarSlot;        // Hotbar slot border
        u32 hotbarSelected;    // Selected slot highlight

        // Chat
        u32 chatBg;            // Chat background
        u32 chatBgAlpha;       // Chat background alpha (0-255)
        u32 chatText;          // Chat text
        u32 chatName;          // Player name in chat
        u32 chatBubbleBg;      // Chat bubble background
        u32 chatBubbleBorder;  // Chat bubble border

        // Menu / settings
        u32 menuBg;            // Menu background
        u32 menuItemSelected;  // Selected menu item
        u32 menuHover;         // Hovered menu item

        // Scrollbar
        u32 scrollThumb;       // Scrollbar thumb
        u32 scrollTrack;       // Scrollbar track

        // Font settings
        const char* fontName;  // Font face name
        int fontSize;          // Default font size
        int titleFontSize;     // Title font size

        // Style settings
        bool roundedCorners;   // Use rounded corners
        bool gradients;        // Use gradient fills
        float cornerRadius;    // Corner radius in pixels
        int buttonPadding;     // Button internal padding
        int panelPadding;      // Panel internal padding
    };

    // Initialize theme manager and load all year presets
    void init();

    // Apply theme based on content year (2012, 2015, 2016, 2019, 2020, 2022)
    void applyTheme(int year);

    // Get current active theme
    UITheme& getCurrentTheme();

    // Get theme by year
    const UITheme* getThemeByYear(int year);

    // Get available years
    std::vector<int> getAvailableYears();

    // Shutdown
    void shutdown();
}