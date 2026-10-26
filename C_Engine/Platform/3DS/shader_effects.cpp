#include "shader_effects.h"
#include <math.h>

namespace ShaderEffects {
    static EffectType currentEffect = EffectType::None;
    static float strength = 0.5f;

    void init() {}

    void setEffect(EffectType type) { currentEffect = type; }
    EffectType getCurrentEffect() { return currentEffect; }
    float getEffectStrength() { return strength; }
    void setEffectStrength(float s) { strength = s < 0.0f ? 0.0f : (s > 1.0f ? 1.0f : s); }

    u32 applyEffect(u32 pixel, u32 x, u32 y, u32 width, u32 height) {
        if (currentEffect == EffectType::None) return pixel;

        u8 r = (pixel >> 16) & 0xFF;
        u8 g = (pixel >> 8) & 0xFF;
        u8 b = pixel & 0xFF;
        u8 a = (pixel >> 24) & 0xFF;

        float f = strength;

        switch (currentEffect) {
            case EffectType::CRT: {
                // Scanlines: darken every other row
                if (y % 2 == 0) {
                    r = (u8)(r * (1.0f - f * 0.5f));
                    g = (u8)(g * (1.0f - f * 0.5f));
                    b = (u8)(b * (1.0f - f * 0.5f));
                }
                // Vignette
                float cx = width / 2, cy = height / 2;
                float dist = (float)hypot(x - cx, y - cy) / (float)hypot(cx, cy);
                float vig = 1.0f - (dist * f * 0.8f);
                r = (u8)(r * vig);
                g = (u8)(g * vig);
                b = (u8)(b * vig);
                break;
            }
            case EffectType::Pixelate: {
                // Block pixels
                u32 blockSize = (u32)(4.0f + f * 16.0f);
                u32 bx = (x / blockSize) * blockSize;
                u32 by = (y / blockSize) * blockSize;
                // Would need neighbor sampling - simplified as color quantize
                r = (u8)((r / blockSize) * blockSize);
                g = (u8)((g / blockSize) * blockSize);
                b = (u8)((b / blockSize) * blockSize);
                break;
            }
            case EffectType::Sepia: {
                u32 tr = (u32)(0.393f * r + 0.769f * g + 0.189f * b);
                u32 tg = (u32)(0.349f * r + 0.686f * g + 0.168f * b);
                u32 tb = (u32)(0.272f * r + 0.534f * g + 0.131f * b);
                r = (u8)(r + (tr - r) * f);
                g = (u8)(g + (tg - g) * f);
                b = (u8)(b + (tb - b) * f);
                break;
            }
            case EffectType::Grayscale: {
                u32 gray = (u32)(0.299f * r + 0.587f * g + 0.114f * b);
                r = (u8)(r + (gray - r) * f);
                g = (u8)(g + (gray - g) * f);
                b = (u8)(b + (gray - b) * f);
                break;
            }
            case EffectType::Bloom: {
                u32 brightness = (u32)(r * 0.299f + g * 0.587f + b * 0.114f);
                if (brightness > 128) {
                    u32 glow = (brightness - 128) * f;
                    r = (u8)(r + glow);
                    g = (u8)(g + glow);
                    b = (u8)(b + glow);
                }
                break;
            }
            case EffectType::Vignette: {
                float cx = width / 2, cy = height / 2;
                float dist = (float)hypot(x - cx, y - cy) / (float)hypot(cx, cy);
                float vig = 1.0f - (dist * dist * f);
                r = (u8)(r * vig);
                g = (u8)(g * vig);
                b = (u8)(b * vig);
                break;
            }
            default: break;
        }

        return (a << 24) | ((u8)r << 16) | ((u8)g << 8) | (u8)b;
    }

    const char* getEffectName(EffectType type) {
        switch (type) {
            case EffectType::None:     return "None";
            case EffectType::CRT:      return "CRT";
            case EffectType::Pixelate: return "Pixelate";
            case EffectType::Sepia:    return "Sepia";
            case EffectType::Grayscale:return "Grayscale";
            case EffectType::Bloom:    return "Bloom";
            case EffectType::Vignette: return "Vignette";
            default: return "Unknown";
        }
    }

    int getEffectCount() { return 7; }
    void shutdown() {}
}