#pragma once

#include <string>
#include <vector>
#include <map>
#include <3ds.h>

enum class FontSize {
    Size8,
    Size10,
    Size12,
    Size16,
    Size20,
    Size24,
    Size32
};

// Classic Roblox font families
enum class FontFace {
    Arial,
    ComicSans,
    SourceSansPro,
    Legacy  // Eras Demi ITC equivalent (bitmap font from content2012)
};

// Font style variants
enum class FontStyle {
    Regular,
    Bold,
    Italic,
    BoldItalic,
    Light,
    Semibold,
    Black
};

// Maximum supported fonts
static const int MAX_FONTS = 32;

namespace FontManager {
    // Initialize font system. ttfPath may be a path under content/Fonts. fontHeight is pixel height.
    bool initFont(const std::string &ttfPath, int fontHeight = 16);
    
    // Initialize all classic Roblox fonts (Arial, ComicSans, SourceSansPro)
    bool initClassicFonts();
    
    // Load a specific font by face and style
    bool loadFont(FontFace face, FontStyle style);
    
    // Get TTF path for a font face and style
    std::string getFontPath(FontFace face, FontStyle style = FontStyle::Regular);
    
    void shutdownFont();

    // Draw text at bottom-screen coordinates (pixels). Returns width in pixels.
    int drawText(int x, int y, const std::string &text, uint32_t color = 0xFFFFFFFF);
    
    // Draw text with custom size and color (RGB)
    int drawText(const char* text, int x, int y, int r, int g, int b, FontSize size = FontSize::Size16);
    
    // Draw text with specific font face and style
    int drawTextWithFont(const char* text, int x, int y, int r, int g, int b, 
                         FontFace face, FontStyle style, FontSize size = FontSize::Size16);
    
    // Draw text to top screen with specific font
    int drawTextTopWithFont(const char* text, int x, int y, int r, int g, int b,
                            FontFace face, FontStyle style, FontSize size = FontSize::Size16);
    
    // Get text width in pixels for a given size
    int getTextWidth(const std::string &text, FontSize size = FontSize::Size16);
    
    // Get text width with specific font
    int getTextWidthWithFont(const std::string &text, FontFace face, FontStyle style, FontSize size = FontSize::Size16);
    
    // Draw text to top screen (using Citro3D coordinates)
    int drawTextTop(const char* text, int x, int y, int r, int g, int b, FontSize size = FontSize::Size16);
    
    // Check if a font is loaded
    bool isFontLoaded(FontFace face, FontStyle style);
}