#include "ui_theme.h"
#include <vector>
#include <algorithm>

namespace ThemeManager {

    static UITheme currentTheme;
    static std::map<int, UITheme> yearThemes;
    static std::vector<int> availableYears = {2012, 2015, 2016, 2019, 2022};

    // Helper to create a UITheme
    static UITheme createTheme(
        u32 panelBg, u32 panelBorder, u32 panelTitleBg, u32 panelTitleText,
        u32 buttonBg, u32 buttonHover, u32 buttonPressed, u32 buttonText, u32 buttonBorder,
        u32 textPrimary, u32 textSecondary, u32 textDisabled, u32 textHighlight,
        u32 progressBarBg, u32 progressBarFg,
        u32 hotbarBg, u32 hotbarSlot, u32 hotbarSelected,
        u32 chatBg, u32 chatBgAlpha, u32 chatText, u32 chatName, u32 chatBubbleBg, u32 chatBubbleBorder,
        u32 menuBg, u32 menuItemSelected, u32 menuHover,
        u32 scrollThumb, u32 scrollTrack,
        const char* fontName, int fontSize, int titleFontSize,
        bool roundedCorners, bool gradients, float cornerRadius, int buttonPadding, int panelPadding)
    {
        UITheme theme;
        theme.panelBg = panelBg;
        theme.panelBorder = panelBorder;
        theme.panelTitleBg = panelTitleBg;
        theme.panelTitleText = panelTitleText;
        theme.buttonBg = buttonBg;
        theme.buttonHover = buttonHover;
        theme.buttonPressed = buttonPressed;
        theme.buttonText = buttonText;
        theme.buttonBorder = buttonBorder;
        theme.textPrimary = textPrimary;
        theme.textSecondary = textSecondary;
        theme.textDisabled = textDisabled;
        theme.textHighlight = textHighlight;
        theme.progressBarBg = progressBarBg;
        theme.progressBarFg = progressBarFg;
        theme.hotbarBg = hotbarBg;
        theme.hotbarSlot = hotbarSlot;
        theme.hotbarSelected = hotbarSelected;
        theme.chatBg = chatBg;
        theme.chatBgAlpha = chatBgAlpha;
        theme.chatText = chatText;
        theme.chatName = chatName;
        theme.chatBubbleBg = chatBubbleBg;
        theme.chatBubbleBorder = chatBubbleBorder;
        theme.menuBg = menuBg;
        theme.menuItemSelected = menuItemSelected;
        theme.menuHover = menuHover;
        theme.scrollThumb = scrollThumb;
        theme.scrollTrack = scrollTrack;
        theme.fontName = fontName;
        theme.fontSize = fontSize;
        theme.titleFontSize = titleFontSize;
        theme.roundedCorners = roundedCorners;
        theme.gradients = gradients;
        theme.cornerRadius = cornerRadius;
        theme.buttonPadding = buttonPadding;
        theme.panelPadding = panelPadding;
        return theme;
    }

    void init() {
        // 2012 - Classic Roblox UI
        // Gray panels, blocky buttons, Eras Demi font
        yearThemes[2012] = createTheme(
            0xFFBEBEBE, 0xFF444444, 0xFF666666, 0xFFFFFFFF,  // panel colors
            0xFFBEBEBE, 0xFFD0D0D0, 0xFFA0A0A0, 0xFF000000, 0xFF444444,  // button colors
            0xFF000000, 0xFF666666, 0xFF999999, 0xFF0000FF,  // text colors
            0xFF666666, 0xFFFF0000,  // progress bar
            0xFFAAAAAA, 0xFF666666, 0xFF00FF00,  // hotbar
            0x80FFFFFF, 0xCC, 0xFF000000, 0xFF0000FF, 0xFFBEBEBE, 0xFF444444,  // chat
            0xFFBEBEBE, 0xFF888888, 0xFFAAAAAA,  // menu
            0xFF888888, 0xFFCCCCCC,  // scrollbar
            "ErasDemi", 16, 20,
            false, false, 0.0f, 2, 4
        );

        // 2015 - Slightly modernized
        // Lighter panels, Comic Sans for chat
        yearThemes[2015] = createTheme(
            0xFFD0D0D0, 0xFF888888, 0xFF999999, 0xFFFFFFFF,
            0xFFD0D0D0, 0xFFE8E8E8, 0xFFB0B0B0, 0xFF000000, 0xFF666666,
            0xFF000000, 0xFF555555, 0xFF888888, 0xFF0066CC,
            0xFF888888, 0xFF00AA00,
            0xFFCCCCCC, 0xFF888888, 0xFF44CC44,
            0x90FFFFFF, 0xAA, 0xFF000000, 0xFF0066CC, 0xFFE0E0E0, 0xFFAAAAAA,
            0xFFD0D0D0, 0xFF999999, 0xFFBBBBBB,
            0xFF999999, 0xFFDDDDDD,
            "ComicSans", 14, 18,
            false, false, 0.0f, 3, 4
        );

        // 2016 - Modern Roblox UI (Source Sans era)
        // Blue accents, smoother panels, semi-transparent chat
        yearThemes[2016] = createTheme(
            0xFFE0E0E0, 0xFFAAAAAA, 0xFF0066CC, 0xFFFFFFFF,
            0xFFE0E0E0, 0xFFF0F0F0, 0xFFC0C0C0, 0xFF000000, 0xFF888888,
            0xFF000000, 0xFF444444, 0xFF777777, 0xFF0066CC,
            0xFFAAAAAA, 0xFF00AA00,
            0xFFCCCCCC, 0xFF999999, 0xFF44AA44,
            0xA02D2D2D, 0x80, 0xFFFFFFFF, 0xFF4488FF, 0xFF333333, 0xFF555555,
            0xFFE0E0E0, 0xFF0066CC, 0xFFDDDDDD,
            0xFFAAAAAA, 0xFFDDDDDD,
            "SourceSans", 14, 18,
            true, false, 3.0f, 4, 6
        );

        // 2019 - Modern dark UI
        // Dark gray panels, blue highlights, fully rounded corners
        yearThemes[2019] = createTheme(
            0xFF3A3A3A, 0xFF222222, 0xFF222222, 0xFFFFFFFF,
            0xFF4A4A4A, 0xFF5A5A5A, 0xFF3A3A3A, 0xFFFFFFFF, 0xFF222222,
            0xFFFFFFFF, 0xFFAAAAAA, 0xFF666666, 0xFF0096FF,
            0xFF222222, 0xFF00FF00,
            0xFF333333, 0xFF555555, 0xFF00CC00,
            0xC01A1A1A, 0x90, 0xFFFFFFFF, 0xFF66AAFF, 0xFF2A2A2A, 0xFF444444,
            0xFF3A3A3A, 0xFF0096FF, 0xFF4A4A4A,
            0xFF666666, 0xFF444444,
            "SourceSans", 14, 18,
            true, false, 6.0f, 6, 8
        );

        // 2020 - Dark mode with new typography
        yearThemes[2020] = createTheme(
            0xFF3D3D3D, 0xFF2A2A2A, 0xFF2A2A2A, 0xFFFFFFFF,
            0xFF4D4D4D, 0xFF5D5D5D, 0xFF3D3D3D, 0xFFFFFFFF, 0xFF2A2A2A,
            0xFFFFFFFF, 0xFFBBBBBB, 0xFF777777, 0xFF1A8CFF,
            0xFF2A2A2A, 0xFF00CC00,
            0xFF333333, 0xFF555555, 0xFF00DD00,
            0xD0222222, 0xA0, 0xFFFFFFFF, 0xFF77BBFF, 0xFF2D2D2D, 0xFF4A4A4A,
            0xFF3D3D3D, 0xFF1A8CFF, 0xFF4D4D4D,
            0xFF777777, 0xFF4D4D4D,
            "SourceSans", 13, 17,
            true, false, 8.0f, 6, 8
        );

        // 2022 - Latest Roblox UI style
        // Very clean, lots of white space on light mode, smooth animations
        yearThemes[2022] = createTheme(
            0xFF2B2B2B, 0xFF1A1A1A, 0xFF1A1A1A, 0xFFFFFFFF,
            0xFF3F3F3F, 0xFF4F4F4F, 0xFF2F2F2F, 0xFFFFFFFF, 0xFF1A1A1A,
            0xFFFFFFFF, 0xFFCCCCCC, 0xFF888888, 0xFF00A2FF,
            0xFF1A1A1A, 0xFF00DD00,
            0xFF2B2B2B, 0xFF4A4A4A, 0xFF00EE00,
            0xE01E1E1E, 0xB0, 0xFFFFFFFF, 0xFF88CCFF, 0xFF252525, 0xFF3F3F3F,
            0xFF2B2B2B, 0xFF00A2FF, 0xFF3F3F3F,
            0xFF888888, 0xFF3F3F3F,
            "SourceSans", 13, 17,
            true, false, 10.0f, 8, 10
        );

        // Default to 2012 theme
        applyTheme(2012);
    }

    void applyTheme(int year) {
        auto it = yearThemes.find(year);
        if (it != yearThemes.end()) {
            currentTheme = it->second;
        } else {
            // Fallback to 2012
            applyTheme(2012);
        }
    }

    UITheme& getCurrentTheme() {
        return currentTheme;
    }

    const UITheme* getThemeByYear(int year) {
        auto it = yearThemes.find(year);
        if (it != yearThemes.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::vector<int> getAvailableYears() {
        return availableYears;
    }

    void shutdown() {
        yearThemes.clear();
    }
}