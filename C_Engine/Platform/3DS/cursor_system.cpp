#include "cursor_system.h"
#include "stb_image.h"
#include "citro_renderer.h"
#include <stdio.h>
#include <string.h>

namespace CursorSystem {

    // Private state
    static int s_cursorX = 200;
    static int s_cursorY = 120;
    static bool s_enabled = false;
    static bool s_leftClick = false;
    static bool s_rightClick = false;
    static bool s_leftClickPrev = false;
    static bool s_rightClickPrev = false;

    // Cursor textures (cached per year)
    static std::map<std::string, CursorTexture> s_textureCache;

    // Year-to-cursor-folder mapping
    static struct YearCursorMap {
        Settings::ContentYear year;
        const char* cursorFolder;
    } s_yearCursorMap[] = {
        {Settings::ContentYear::Year2007, "cursor2007"},
        {Settings::ContentYear::Year2011, "cursor2012"},
        {Settings::ContentYear::Year2012, "cursor2012"},
        {Settings::ContentYear::Year2015, "cursor2015"},
        {Settings::ContentYear::Year2016, "cursor2016"},
        {Settings::ContentYear::Year2019, "cursor2019"},
        {Settings::ContentYear::Year2022, "cursor2022"}
    };
    static const int s_yearCursorMapSize = sizeof(s_yearCursorMap) / sizeof(s_yearCursorMap[0]);

    static std::string getCursorFolderForYear(Settings::ContentYear year) {
        for (int i = 0; i < s_yearCursorMapSize; i++) {
            if (s_yearCursorMap[i].year == year) {
                return std::string("assets/") + s_yearCursorMap[i].cursorFolder + "/";
            }
        }
        return "assets/cursor2012/";  // Fallback
    }

    void init() {
        printf("[CursorSystem] Initializing cursor system\n");
        s_cursorX = 200;
        s_cursorY = 120;
        s_enabled = false;
        reloadCursorForYear();
    }

    void shutdown() {
        // Free all cached pixel data
        for (auto& pair : s_textureCache) {
            if (pair.second.pixels) {
                stbi_image_free(pair.second.pixels);
                pair.second.pixels = nullptr;
            }
        }
        s_textureCache.clear();
        printf("[CursorSystem] Shutdown complete\n");
    }

    void setEnabled(bool enabled) {
        s_enabled = enabled;
        if (enabled) {
            printf("[CursorSystem] Cursor enabled\n");
        } else {
            printf("[CursorSystem] Cursor disabled\n");
        }
    }

    bool isEnabled() {
        return s_enabled;
    }

    int getCursorX() {
        return s_cursorX;
    }

    int getCursorY() {
        return s_cursorY;
    }

    void setCursorPosition(int x, int y) {
        s_cursorX = x;
        s_cursorY = y;
        clampCursor();
    }

    void trackpadUpdate(int dx, int dy) {
        if (!s_enabled) return;

        // Move cursor by trackpad delta
        // Scale factor: adjust for sensitivity
        float sensitivity = 1.5f;
        s_cursorX += (int)(dx * sensitivity);
        s_cursorY += (int)(dy * sensitivity);

        clampCursor();
    }

    void setLeftClick(bool pressed) {
        s_leftClick = pressed;
    }

    void setRightClick(bool pressed) {
        s_rightClick = pressed;
    }

    bool isLeftClick() {
        bool clicked = s_leftClick && !s_leftClickPrev;
        s_leftClickPrev = s_leftClick;
        return clicked;
    }

    bool isRightClick() {
        bool clicked = s_rightClick && !s_rightClickPrev;
        s_rightClickPrev = s_rightClick;
        return clicked;
    }

    static void renderFallbackCursor();

    void render() {
        if (!s_enabled) return;

        // Get or load cursor texture for current year
        Settings::ContentYear year = Settings::get().general.contentYear;
        std::string folder = getCursorFolderForYear(year);

        auto it = s_textureCache.find(folder);
        if (it == s_textureCache.end() || !it->second.pixels) {
            // Load cursor texture using stbi_load (like icon_loader.cpp)
            std::string path = folder + "ArrowCursor.png";
            
            // Try multiple path formats
            uint8_t* data = nullptr;
            int w, h, channels;
            std::string tryPaths[] = {
                path,                           // "assets/cursor2015/ArrowCursor.png"
                "sdmc:/OpenBLOX/" + path,      // Full SD card path
                path.substr(7),                // "cursor2015/ArrowCursor.png"
                "sdmc:/OpenBLOX/assets/" + path.substr(7)
            };

            for (const auto& tryPath : tryPaths) {
                data = stbi_load(tryPath.c_str(), &w, &h, &channels, 4); // Force RGBA
                if (data) {
                    printf("[CursorSystem] Loaded cursor from %s (%dx%d)\n", tryPath.c_str(), w, h);
                    break;
                }
            }

            if (data) {
                CursorTexture ct;
                ct.name = "ArrowCursor";
                ct.pixels = data;
                ct.width = w;
                ct.height = h;
                s_textureCache[folder] = ct;
                it = s_textureCache.find(folder);
            } else {
                printf("[CursorSystem] Warning: Could not load cursor, using fallback\n");
                renderFallbackCursor();
                return;
            }
        }

        // Draw cursor pixel-by-pixel (like icon_loader.cpp)
        int drawX = s_cursorX - 2;
        int drawY = s_cursorY - 2;
        int w = it->second.width;
        int h = it->second.height;

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                uint8_t* pixel = &it->second.pixels[(i * w + j) * 4];
                uint8_t r = pixel[0];
                uint8_t g = pixel[1];
                uint8_t b = pixel[2];
                uint8_t a = pixel[3];

                if (a > 10) { // Skip transparent pixels
                    DrawPixel(drawX + j, drawY + i, r, g, b);
                }
            }
        }
    }

    static void renderFallbackCursor() {
        // Draw a simple crosshair if texture loading fails
        int x = s_cursorX;
        int y = s_cursorY;
        int size = 8;

        // Simple white crosshair
        for (int i = 0; i < size; i++) {
            DrawPixel(x - i, y, 255, 255, 255);
            DrawPixel(x + i, y, 255, 255, 255);
            DrawPixel(x, y - i, 255, 255, 255);
            DrawPixel(x, y + i, 255, 255, 255);
        }
    }

    void reloadCursorForYear() {
        // Free all cached pixel data
        for (auto& pair : s_textureCache) {
            if (pair.second.pixels) {
                stbi_image_free(pair.second.pixels);
                pair.second.pixels = nullptr;
            }
        }
        s_textureCache.clear();

        Settings::ContentYear year = Settings::get().general.contentYear;
        std::string folder = getCursorFolderForYear(year);
        std::string path = folder + "ArrowCursor.png";

        // Try multiple path formats
        uint8_t* data = nullptr;
        int w = 32, h = 32;
        std::string tryPaths[] = {
            path,                           // "assets/cursor2015/ArrowCursor.png"
            "sdmc:/OpenBLOX/" + path,      // Full SD card path
            path.substr(7),                // "cursor2015/ArrowCursor.png"
            "sdmc:/OpenBLOX/assets/" + path.substr(7)
        };

        for (const auto& tryPath : tryPaths) {
            int channels;
            data = stbi_load(tryPath.c_str(), &w, &h, &channels, 4); // Force RGBA
            if (data) {
                printf("[CursorSystem] Loaded %s (%dx%d) for year %d\n", tryPath.c_str(), w, h, (int)year);
                break;
            }
        }

        if (data) {
            CursorTexture ct;
            ct.name = "ArrowCursor";
            ct.pixels = data;
            ct.width = w;
            ct.height = h;
            s_textureCache[folder] = ct;
        } else {
            printf("[CursorSystem] Warning: Could not load cursor for year %d\n", (int)year);
        }
    }

    std::string getCursorFolder() {
        Settings::ContentYear year = Settings::get().general.contentYear;
        return getCursorFolderForYear(year);
    }

    std::string getCursorPath(Settings::ContentYear year, const std::string& cursorName) {
        std::string folder = getCursorFolderForYear(year);
        return folder + cursorName;
    }

    void clampCursor() {
        // Top screen is 400x240
        // Keep cursor within bounds
        int cursorSize = 32;  // Approximate cursor size

        if (s_cursorX < 0) s_cursorX = 0;
        if (s_cursorY < 0) s_cursorY = 0;
        if (s_cursorX > 400 - cursorSize) s_cursorX = 400 - cursorSize;
        if (s_cursorY > 240 - cursorSize) s_cursorY = 240 - cursorSize;
    }
}