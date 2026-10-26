#include "theme_system.h"
#include "lighting.h"
#include <cstdio>

namespace Theme {
    static ThemeType currentTheme = ThemeType::Default;
    static ThemeColors colors;
    static ContentYear currentContentYear = ContentYear::Year2012;
    static std::vector<YearTheme> yearThemes;

    static ThemeColors themePresets[] = {
        // Default (light)
        {
            0xFF000000, 0xFFFF0000,  // health bar bg, fg
            0x80000000, 0xFF888888, 0xFF00FF00, // hotbar bg, border, selected
            0xFFFFFFFF, 0x80000000, 0xFF000000, // bg, btn bg, btn sel
            0xFF000000, 0xFF888888, 0xFF000000, // text primary, secondary, border
            0xF0FFFFFF, 0xFFE0E0E0, 0xFF0078D4, 0xFF000000 // menu bg, item, sel, text
        },
        // Dark
        {
            0xFF000000, 0xFFFF0000,
            0x80202020, 0xFF444444, 0xFF00FF00,
            0xFF111111, 0x80333333, 0xFF000000,
            0xFFEEEEEE, 0xFF888888, 0xFF444444,
            0xFF1A1A1A, 0xFF2A2A2A, 0xFF0078D4, 0xFFFFFFFF
        },
        // Blue
        {
            0xFF000000, 0xFFFF0000,
            0x80001A6C, 0xFF0078D4, 0xFF00FF00,
            0xFF001A6C, 0x80003399, 0xFFFFFFFF,
            0xFFFFFFFF, 0xFF99CCFF, 0xFF0078D4,
            0xFF001A6C, 0xFF003399, 0xFF00AAFF, 0xFFFFFFFF
        },
        // Green
        {
            0xFF000000, 0xFFFF0000,
            0x80004400, 0xFF008800, 0xFF00FF00,
            0xFF003300, 0x80006600, 0xFFFFFFFF,
            0xFFFFFFFF, 0xFF99FF99, 0xFF008800,
            0xFF003300, 0xFF006600, 0xFF00AA00, 0xFFFFFFFF
        },
        // Purple
        {
            0xFF000000, 0xFFFF0000,
            0x80440044, 0xFF880088, 0xFF00FF00,
            0xFF220044, 0x80660066, 0xFFFFFFFF,
            0xFFFFFFFF, 0xFFCC99CC, 0xFF880088,
            0xFF220044, 0xFF440066, 0xFFAA00AA, 0xFFFFFFFF
        },
        // Sunset
        {
            0xFF000000, 0xFFFF0000,
            0x80884400, 0xFFFF6600, 0xFF00FF00,
            0xFF442200, 0x80884400, 0xFFFFFFFF,
            0xFFFFFFFF, 0xFFCCAA88, 0xFFFF6600,
            0xFF442200, 0xFF663300, 0xFFFF8800, 0xFFFFFFFF
        },
    };

    static YearTheme defaultYearThemes[] = {
        // 2012: Classic Roblox UI — gray panels, Eras Demi, blocky buttons
        {
            ContentYear::Year2012, "2012 Classic",
            {
                0xFF000000, 0xFFFF0000,
                0x80000000, 0xFF888888, 0xFF00FF00,
                0xFFAAAAAA, 0xFF666666, 0xFF000000,
                0xFF000000, 0xFF999999, 0xFF000000,
                0xFFD0D0D0, 0xFFE0E0E0, 0xFF0078D4, 0xFF000000
            },
            "ErasDemi", 18, ButtonStyle::Blocky,
            0.85f, false, false, 2.0f, 0xFF000000, 0xFF333333, 0xFFCCCCCC
        },
        // 2015: Slightly modernized
        {
            ContentYear::Year2015, "2015 Updated",
            {
                0xFF000000, 0xFFFF0000,
                0x90000000, 0xFF777777, 0xFF00FF00,
                0xFFB0B0B0, 0xFF555555, 0xFF000000,
                0xFFAAAAAA, 0xFF888888, 0xFF000000,
                0xFFD8D8D8, 0xFFE5E5E5, 0xFF0066CC, 0xFF000000
            },
            "ErasDemi", 16, ButtonStyle::Smooth,
            0.80f, true, false, 1.5f, 0xFF000000, 0xFF444444, 0xFFDDDDDD
        },
        // 2016: Source Sans, smoother panels, new color scheme
        {
            ContentYear::Year2016, "2016 Modern",
            {
                0xFF000000, 0xFFFF0000,
                0x95000000, 0xFF666666, 0xFF00FF00,
                0xFFB8B8B8, 0xFF444444, 0xFF000000,
                0xFF000000, 0xFF888888, 0xFF000000,
                0xFFE0E0E0, 0xFFEEEEEE, 0xFF0055CC, 0xFF000000
            },
            "SourceSans", 16, ButtonStyle::Smooth,
            0.75f, true, false, 1.0f, 0xFF000000, 0xFF555555, 0xFFEEEEEE
        },
        // 2019: Modern Roblox UI (blue accents, rounded corners)
        {
            ContentYear::Year2019, "2019 Roblox",
            {
                0xFF000000, 0xFFFF0000,
                0xA0000000, 0xFF555555, 0xFF00FF00,
                0xFF303030, 0xFF222222, 0xFFFFFFFF,
                0xFFFFFFFF, 0xFFAAAAAA, 0xFFFFFFFF,
                0xFFD4D9DF, 0xFFE4E8EC, 0xFF00A2FF, 0xFF111111
            },
            "SourceSans", 15, ButtonStyle::Rounded,
            0.70f, true, true, 0.8f, 0xFFBDBDBD, 0xFF1A1A1A, 0xFFF0F0F0
        },
        // 2022: Latest Roblox UI style
        {
            ContentYear::Year2022, "2022 Latest",
            {
                0xFF000000, 0xFFFF0000,
                0xB0111111, 0xFF404040, 0xFF00FF00,
                0xFF181818, 0xFF101010, 0xFFFFFFFF,
                0xFFFFFFFF, 0xFF777777, 0xFFFFFFFF,
                0xFF212326, 0xFF2B2D30, 0xFF0099FF, 0xFFFFFFFF
            },
            "SourceSans", 14, ButtonStyle::Modern,
            0.65f, true, true, 0.3f, 0xFF36383B, 0xFF000000, 0xFF26282B
        },
    };

    void init() {
        // Initialize year themes
        for (int i = 0; i < 5; i++) {
            yearThemes.push_back(defaultYearThemes[i]);
        }
        load();
        applyContentYearTheme(currentContentYear);
    }
    
    void applyContentYearTheme(ContentYear year) {
        if (year == ContentYear::Unknown) return;
        currentContentYear = year;
        for (auto& yt : yearThemes) {
            if (yt.year == year) {
                colors = yt.colors;
                // Apply matching lighting style
                int yearIndex = (int)year; // 0=2012, 1=2015, 2=2016, 3=2019, 4=2020, 5=2022
                LightingSystem::applyContentYearLighting(yearIndex);
                save();
                return;
            }
        }
    }
    
    ContentYear getCurrentContentYear() {
        return currentContentYear;
    }
    
    const YearTheme* getCurrentYearTheme() {
        for (auto& yt : yearThemes) {
            if (yt.year == currentContentYear) return &yt;
        }
        return nullptr;
    }
    
    std::vector<YearTheme>& getAllYearThemes() {
        return yearThemes;
    }
    
    const char* getYearName(ContentYear year) {
        for (auto& yt : yearThemes) {
            if (yt.year == year) return yt.name.c_str();
        }
        return "Unknown";
    }

    void setTheme(ThemeType type) {
        currentTheme = type;
        if (type != ThemeType::Custom) {
            colors = themePresets[(int)type];
        }
        save();
    }

    ThemeType getCurrentTheme() { return currentTheme; }
    ThemeColors& getColors() { return colors; }

    void setCustomColor(int slot, u32 color) {
        currentTheme = ThemeType::Custom;
        switch (slot) {
            case 0: colors.healthBarBg = color; break;
            case 1: colors.healthBarFg = color; break;
            case 2: colors.backgroundColor = color; break;
            case 3: colors.textPrimary = color; break;
            case 4: colors.menuBg = color; break;
            default: break;
        }
        save();
    }

    bool load() {
        FILE* f = fopen("sdmc:/OpenBLOX/theme.dat", "rb");
        if (!f) {
            currentTheme = ThemeType::Default;
            currentContentYear = ContentYear::Year2012;
            colors = themePresets[0];
            return true;
        }
        fread(&currentTheme, sizeof(ThemeType), 1, f);
        fread(&currentContentYear, sizeof(ContentYear), 1, f);
        fread(&colors, sizeof(ThemeColors), 1, f);
        fclose(f);
        return true;
    }

    bool save() {
        FILE* f = fopen("sdmc:/OpenBLOX/theme.dat", "wb");
        if (!f) return false;
        fwrite(&currentTheme, sizeof(ThemeType), 1, f);
        fwrite(&currentContentYear, sizeof(ContentYear), 1, f);
        fwrite(&colors, sizeof(ThemeColors), 1, f);
        fclose(f);
        return true;
    }

    int getThemeCount() { return 6; } // Default through Sunset

    const char* getThemeName(ThemeType type) {
        switch (type) {
            case ThemeType::Default: return "Default";
            case ThemeType::Dark:    return "Dark";
            case ThemeType::Blue:    return "Blue";
            case ThemeType::Green:   return "Green";
            case ThemeType::Purple:  return "Purple";
            case ThemeType::Sunset:  return "Sunset";
            case ThemeType::Custom:  return "Custom";
            default:                return "Unknown";
        }
    }

    void shutdown() {
        save();
    }
}