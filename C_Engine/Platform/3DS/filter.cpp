#include "filter.h"
#include <algorithm>

static int pixelateBlockSize = 4;

void Filter::init() {
    pixelateBlockSize = 4;
}

void Filter::apply(FilterType type, u32* framebuffer, int width, int height) {
    switch (type) {
        case FilterType::None:
            break;
        case FilterType::CRT:
            applyCRT(framebuffer, width, height);
            break;
        case FilterType::Pixelate:
            // Pixelate by upsampling blocks
            for (int y = 0; y < height; y += pixelateBlockSize) {
                for (int x = 0; x < width; x += pixelateBlockSize) {
                    u32 color = framebuffer[y * width + x];
                    for (int dy = 0; dy < pixelateBlockSize && y + dy < height; dy++) {
                        for (int dx = 0; dx < pixelateBlockSize && x + dx < width; dx++) {
                            framebuffer[(y + dy) * width + (x + dx)] = color;
                        }
                    }
                }
            }
            break;
        case FilterType::Sepia:
            applySepia(framebuffer, width, height);
            break;
        case FilterType::Grayscale:
            applyGrayscale(framebuffer, width, height);
            break;
        case FilterType::Brightness:
            applyBrightness(framebuffer, width, height, 50, 50);
            break;
    }
}

void Filter::applyBrightness(u32* framebuffer, int width, int height, int brightness, int gamma) {
    float brightMult = brightness / 50.0f;  // 0-2.0, 1.0 = normal
    for (int i = 0; i < width * height; i++) {
        u8 r = framebuffer[i] & 0xFF;
        u8 g = (framebuffer[i] >> 8) & 0xFF;
        u8 b = (framebuffer[i] >> 16) & 0xFF;
        
        r = std::clamp((int)(r * brightMult), 0, 255);
        g = std::clamp((int)(g * brightMult), 0, 255);
        b = std::clamp((int)(b * brightMult), 0, 255);
        
        framebuffer[i] = r | (g << 8) | (b << 16);
    }
}

void Filter::setPixelateSize(int blockSize) {
    pixelateBlockSize = std::max(1, std::min(blockSize, 16));
}

void Filter::applySepia(u32* framebuffer, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        u8 r = framebuffer[i] & 0xFF;
        u8 g = (framebuffer[i] >> 8) & 0xFF;
        u8 b = (framebuffer[i] >> 16) & 0xFF;
        
        u8 tr = std::clamp((int)(0.393f * r + 0.769f * g + 0.189f * b), 0, 255);
        u8 tg = std::clamp((int)(0.349f * r + 0.686f * g + 0.168f * b), 0, 255);
        u8 tb = std::clamp((int)(0.272f * r + 0.534f * g + 0.131f * b), 0, 255);
        
        framebuffer[i] = tr | (tg << 8) | (tb << 16);
    }
}

void Filter::applyCRT(u32* framebuffer, int width, int height) {
    // Scanlines: darken every other row
    for (int y = 0; y < height; y++) {
        if (y % 2 == 1) {
            for (int x = 0; x < width; x++) {
                u32 px = framebuffer[y * width + x];
                // Reduce brightness by 40%
                u8 r = (px & 0xFF) * 0.6f;
                u8 g = ((px >> 8) & 0xFF) * 0.6f;
                u8 b = ((px >> 16) & 0xFF) * 0.6f;
                framebuffer[y * width + x] = r | (g << 8) | (b << 16);
            }
        }
    }
}

void Filter::applyGrayscale(u32* framebuffer, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        u8 r = framebuffer[i] & 0xFF;
        u8 g = (framebuffer[i] >> 8) & 0xFF;
        u8 b = (framebuffer[i] >> 16) & 0xFF;
        
        u8 gray = (u8)(0.299f * r + 0.587f * g + 0.114f * b);
        framebuffer[i] = gray | (gray << 8) | (gray << 16);
    }
}

void Filter::shutdown() {
}