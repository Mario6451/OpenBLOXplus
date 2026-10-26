#pragma once
#include <3ds.h>

namespace Accessibility {
    enum class ColorblindMode {
        None,
        Deuteranopia,   // Red-green weakness (green)
        Protanopia,     // Red-green weakness (red)
        Tritanopia,     // Blue-yellow weakness
    };

    void init();

    // Colorblind mode
    void setColorblindMode(ColorblindMode mode);
    ColorblindMode getColorblindMode();

    // Apply colorblind filter to color
    u32 applyColorblindFilter(u32 color);

    // Text size scaling (0.5 to 2.0)
    void setTextScale(float scale);
    float getTextScale();

    // Button hold delay in milliseconds
    void setHoldDelay(int ms);
    int getHoldDelay();

    // High contrast mode
    void setHighContrast(bool enabled);
    bool getHighContrast();

    // Screen shake toggle (for vestibular sensitivity)
    void setScreenShake(bool enabled);
    bool getScreenShake();

    // Load/save
    bool load();
    bool save();

    void shutdown();
}