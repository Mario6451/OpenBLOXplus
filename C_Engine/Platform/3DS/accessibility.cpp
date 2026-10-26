#include "accessibility.h"
#include <cstdio>

namespace Accessibility {
    static ColorblindMode colorblindMode = ColorblindMode::None;
    static float textScale = 1.0f;
    static int holdDelay = 200; // ms
    static bool highContrast = false;
    static bool screenShake = false;

    void init() {
        load();
    }

    void setColorblindMode(ColorblindMode mode) {
        colorblindMode = mode;
        save();
    }

    ColorblindMode getColorblindMode() { return colorblindMode; }

    u32 applyColorblindFilter(u32 color) {
        if (colorblindMode == ColorblindMode::None) return color;

        u8 a = (color >> 24) & 0xFF;
        u8 r = (color >> 16) & 0xFF;
        u8 g = (color >> 8) & 0xFF;
        u8 b = color & 0xFF;

        // Simplified colorblind simulation matrices
        float nr, ng, nb;
        
        switch (colorblindMode) {
            case ColorblindMode::Deuteranopia:
                nr = 0.625f * r + 0.375f * g + 0.0f * b;
                ng = 0.7f * r + 0.3f * g + 0.0f * b;
                nb = 0.0f * r + 0.3f * g + 0.7f * b;
                break;
            case ColorblindMode::Protanopia:
                nr = 0.567f * r + 0.433f * g + 0.0f * b;
                ng = 0.558f * r + 0.442f * g + 0.0f * b;
                nb = 0.0f * r + 0.242f * g + 0.758f * b;
                break;
            case ColorblindMode::Tritanopia:
                nr = 0.95f * r + 0.05f * g + 0.0f * b;
                ng = 0.0f * r + 0.433f * g + 0.567f * b;
                nb = 0.0f * r + 0.475f * g + 0.525f * b;
                break;
            default:
                nr = r; ng = g; nb = b;
                break;
        }

        return (a << 24) | ((u8)nr << 16) | ((u8)ng << 8) | (u8)nb;
    }

    void setTextScale(float scale) {
        textScale = scale < 0.5f ? 0.5f : (scale > 2.0f ? 2.0f : scale);
        save();
    }

    float getTextScale() { return textScale; }

    void setHoldDelay(int ms) {
        holdDelay = ms < 50 ? 50 : (ms > 2000 ? 2000 : ms);
        save();
    }

    int getHoldDelay() { return holdDelay; }

    void setHighContrast(bool enabled) {
        highContrast = enabled;
        save();
    }

    bool getHighContrast() { return highContrast; }

    void setScreenShake(bool enabled) {
        screenShake = enabled;
        save();
    }

    bool getScreenShake() { return screenShake; }

    bool load() {
        FILE* f = fopen("sdmc:/OpenBLOX/accessibility.dat", "rb");
        if (!f) return true;
        fread(&colorblindMode, sizeof(ColorblindMode), 1, f);
        fread(&textScale, sizeof(float), 1, f);
        fread(&holdDelay, sizeof(int), 1, f);
        fread(&highContrast, sizeof(bool), 1, f);
        fread(&screenShake, sizeof(bool), 1, f);
        fclose(f);
        return true;
    }

    bool save() {
        FILE* f = fopen("sdmc:/OpenBLOX/accessibility.dat", "wb");
        if (!f) return false;
        fwrite(&colorblindMode, sizeof(ColorblindMode), 1, f);
        fwrite(&textScale, sizeof(float), 1, f);
        fwrite(&holdDelay, sizeof(int), 1, f);
        fwrite(&highContrast, sizeof(bool), 1, f);
        fwrite(&screenShake, sizeof(bool), 1, f);
        fclose(f);
        return true;
    }

    void shutdown() {
        save();
    }
}