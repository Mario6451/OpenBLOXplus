#pragma once
#include <string>
#include <map>
#include <vector>
#include <3ds.h>

// Year-based UI theme system for OpenBLOX
// Each content year (2012, 2015, 2016, 2019, 2022) has its own UI style

namespace Theme {

    enum class ThemeType {
        Default,    // White/light
        Dark,       // Black/dark gray
        Blue,       // Blue gradient
        Green,      // Green gradient
        Purple,     // Purple gradient
        Sunset,     // Orange/pink
        Custom      // User-defined colors
    };

    // Content year enum for year-based theming
    enum class ContentYear {
        Year2012,
        Year2015,
        Year2016,
        Year2019,
        Year2022,
        Unknown
    };

    // Button style
    enum class ButtonStyle {
        Blocky,      // 2012: sharp rectangles
        Smooth,      // 2015/2016: slightly rounded
        Rounded,     // 2019+: fully rounded corners
        Modern       // 2020+: flat with shadows
    };

    struct ThemeColors {
        // Top screen (game UI overlay)
        u32 healthBarBg;
        u32 healthBarFg;
        u32 hotbarBg;
        u32 hotbarBorder;
        u32 hotbarSelected;
        
        // Bottom screen
        u32 backgroundColor;
        u32 buttonBg;
        u32 buttonSelected;
        u32 textPrimary;
        u32 textSecondary;
        u32 border;
        
        // Menu
        u32 menuBg;
        u32 menuItemBg;
        u32 menuItemSelected;
        u32 menuText;
    };

    // Full theme definition including fonts and styles
    struct YearTheme {
        ContentYear year;
        std::string name;
        ThemeColors colors;
        std::string fontName;          // Font to use (ErasDemi, ComicSans, Arial, SourceSans)
        int fontSize;                   // Default font size
        ButtonStyle buttonStyle;
        float panelTransparency;        // 0.0-1.0
        bool useRoundedCorners;
        bool useDropShadows;
        float borderThickness;          // Border thickness for panels
        u32 panelBorderColor;
        u32 chatBubbleColor;
        u32 backpackSlotColor;
    };

    void init();
    void setTheme(ThemeType type);
    ThemeType getCurrentTheme();
    ThemeColors& getColors();
    
    // Year-based theming
    void applyContentYearTheme(ContentYear year);
    ContentYear getCurrentContentYear();
    const YearTheme* getCurrentYearTheme();
    std::vector<YearTheme>& getAllYearThemes();
    
    // Get theme name string for display
    const char* getYearName(ContentYear year);
    
    // Custom color setting
    void setCustomColor(int slot, u32 color);
    
    // Load/save
    bool load();
    bool save();
    
    // Get available theme count
    int getThemeCount();
    
    // Get theme name for display
    const char* getThemeName(ThemeType type);
    
    void shutdown();
}