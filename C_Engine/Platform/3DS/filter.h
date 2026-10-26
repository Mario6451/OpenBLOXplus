#pragma once

#include <3ds.h>
#include <cstring>

// Post-processing filters - CRT, pixelate, sepia, brightness/gamma

namespace Filter {

    enum class FilterType {
        None,
        CRT,           // CRT scanlines effect
        Pixelate,      // Pixel art / low-res look
        Sepia,         // Vintage sepia tone
        Grayscale,     // Black and white
        Brightness     // Brightness/gamma adjustment
    };

    // Initialize filter system
    void init();

    // Apply filter to framebuffer before display
    void apply(FilterType type, u32* framebuffer, int width, int height);

    // Apply brightness/gamma to framebuffer
    void applyBrightness(u32* framebuffer, int width, int height, int brightness, int gamma);

    // Set pixelate block size
    void setPixelateSize(int blockSize);

    // Apply sepia to framebuffer
    void applySepia(u32* framebuffer, int width, int height);

    // Apply CRT scanlines
    void applyCRT(u32* framebuffer, int width, int height);

    // Apply grayscale
    void applyGrayscale(u32* framebuffer, int width, int height);

    // Shutdown
    void shutdown();
}