#pragma once
#include <3ds.h>

namespace ShaderEffects {
    enum class EffectType {
        None,
        CRT,           // Scanlines + curvature
        Pixelate,      // Blocky/pixelated look
        Sepia,         // Vintage brownish tint
        Grayscale,     // Black and white
        Bloom,         // Glow effect
        Vignette       // Dark corners
    };

    void init();

    // Set active effect
    void setEffect(EffectType type);
    EffectType getCurrentEffect();

    // Get effect parameter
    float getEffectStrength();
    void setEffectStrength(float strength);

    // Apply effect to pixel (RGBA8888)
    u32 applyEffect(u32 pixel, u32 x, u32 y, u32 width, u32 height);

    // Get effect name
    const char* getEffectName(EffectType type);

    // Get effect count
    int getEffectCount();

    void shutdown();
}