#include "icon_loader.h"
#include "Platform/3DS/citro_renderer.h"
#include "stb_image.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace IconLoader {

    // Icon cache
    static std::vector<Icon*> g_icons;

    // Dynamic content folder path (set via setContentFolder)
    static std::string g_contentFolder = "content2012";

    // Fallback colors for icon placeholders
    struct IconFallback {
        const char* name;
        int r, g, b;
    };

    static const IconFallback s_fallbacks[] = {
        {ICON_SETTINGS,      150, 150, 170},   // grey gear
        {ICON_AVATAR_EDITOR, 200, 100, 200},   // purple abstract
        {ICON_STUDIO,        100, 180, 100},   // green wrench
        {ICON_MULTIPLAYER,   100, 150, 220},   // blue network
        {ICON_QUIT,          200, 60, 60},     // red power
        {ICON_VOICE_CHAT,    180, 80, 80},     // red microphone
        {ICON_SPEAKER,       80, 180, 80},     // green speaker
    };

    // Helper: try to find icon in any content folder
    static bool findIconInContentFolder(const std::string& name, std::string& outPath) {
        // Order: current year folder first, then fallback chain
        const char* fallbackFolders[] = {
            g_contentFolder.c_str(),
            "content2007", "content2012", "content2015", "content2016", "content2019", "content2022"
        };

        for (const char* folder : fallbackFolders) {
            std::string path = "sdmc:/OpenBLOX/" + std::string(folder) + "/textures/" + name + ".png";
            FILE* f = fopen(path.c_str(), "rb");
            if (f) {
                fclose(f);
                outPath = path;
                return true;
            }
        }
        return false;
    }

    void setContentFolder(const std::string& folder) {
        g_contentFolder = folder;
        printf("[IconLoader] Content folder set to %s\n", folder.c_str());
    }

    const std::string& getContentFolder() {
        return g_contentFolder;
    }

    void init() {
        // Load default icons
        loadDefaultIcons();
    }

    void shutdown() {
        clearCache();
    }

    Icon* loadIcon(const std::string& name, int maxWidth, int maxHeight) {
        // Check cache first
        for (auto* icon : g_icons) {
            if (icon->path.find(name) != std::string::npos) {
                return icon;
            }
        }

        // Check sdmc:/OpenBLOX/assets/MainMenuIcons/ (3DS runtime)
        std::string path = "sdmc:/OpenBLOX/assets/MainMenuIcons/" + name + ".png";
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            // Check local assets/MainMenuIcons/ (development build)
            path = "assets/MainMenuIcons/" + name + ".png";
            f = fopen(path.c_str(), "rb");
        }
        if (!f) {
            // Check build_pc path
            path = "C_Engine/Makefiles/build_pc/assets/MainMenuIcons/" + name + ".png";
            f = fopen(path.c_str(), "rb");
        }
        if (!f) {
            // Try content2022 VC assets
            if (name == ICON_VOICE_CHAT || name == ICON_SPEAKER) {
                path = "sdmc:/OpenBLOX/content2022/textures/ui/VoiceChat/" + name + ".png";
                f = fopen(path.c_str(), "rb");
            }
        }
        if (!f) {
            // Try existing texture paths (dynamic year-based)
            if (!findIconInContentFolder(name, path)) {
                // Icon not found - create fallback placeholder
                Icon* icon = new Icon();
                icon->data = nullptr;
                icon->width = std::min(maxWidth, maxHeight);
                icon->height = icon->width;
                icon->path = name;
                g_icons.push_back(icon);
                return icon;
            }
        }

        // Load PNG using stb_image
        int w, h, channels;
        uint8_t* data = stbi_load(path.c_str(), &w, &h, &channels, 4); // Force RGBA
        if (!data) {
            // Failed to load - create fallback
            Icon* icon = new Icon();
            icon->data = nullptr;
            icon->width = std::min(maxWidth, maxHeight);
            icon->height = icon->width;
            icon->path = name;
            g_icons.push_back(icon);
            return icon;
        }

        // Auto-resize if too big
        if (w > maxWidth || h > maxHeight) {
            // Calculate scale factor
            float scaleX = (float)maxWidth / w;
            float scaleY = (float)maxHeight / h;
            float scale = std::min(scaleX, scaleY);

            int newW = (int)(w * scale);
            int newH = (int)(h * scale);

            // Resize using stb_image_resize2
            uint8_t* resized = (uint8_t*)malloc(newW * newH * 4);
            if (resized) {
                // Use simple nearest-neighbor resize
                for (int y = 0; y < newH; y++) {
                    for (int x = 0; x < newW; x++) {
                        int srcX = (int)((float)x / newW * w);
                        int srcY = (int)((float)y / newH * h);
                        memcpy(&resized[(y * newW + x) * 4], &data[(srcY * w + srcX) * 4], 4);
                    }
                }
                stbi_image_free(data);
                data = resized;
                w = newW;
                h = newH;
            }
        }

        // Create icon
        Icon* icon = new Icon();
        icon->data = data;
        icon->width = w;
        icon->height = h;
        icon->path = path;
        g_icons.push_back(icon);

        printf("[IconLoader] Loaded %s (%dx%d)\n", name.c_str(), w, h);
        return icon;
    }

    void freeIcon(Icon* icon) {
        if (!icon) return;
        if (icon->data) {
            stbi_image_free(icon->data);
            icon->data = nullptr;
        }
        delete icon;
    }

    bool iconExists(const std::string& name) {
        std::string path = "assets/MainMenuIcons/" + name + ".png";
        FILE* f = fopen(path.c_str(), "rb");
        if (f) { fclose(f); return true; }

        // Check content folders dynamically
        std::string contentPath;
        if (findIconInContentFolder(name, contentPath)) return true;

        return false;
    }

    std::string getIconPath(const std::string& name) {
        std::string path = "assets/MainMenuIcons/" + name + ".png";
        FILE* f = fopen(path.c_str(), "rb");
        if (f) { fclose(f); return path; }

        // Check content folders dynamically
        std::string contentPath;
        if (findIconInContentFolder(name, contentPath)) return contentPath;

        return "";
    }

    void drawIcon(const Icon* icon, int x, int y, int fallbackColor) {
        if (!icon) return;

        if (icon->data) {
            // Draw loaded icon
            int w = icon->width;
            int h = icon->height;
            for (int i = 0; i < h; i++) {
                for (int j = 0; j < w; j++) {
                    uint8_t* pixel = &icon->data[(i * w + j) * 4];
                    uint8_t r = pixel[0];
                    uint8_t g = pixel[1];
                    uint8_t b = pixel[2];
                    uint8_t a = pixel[3];

                    if (a > 10) { // Skip transparent pixels
                        DrawPixel(x + j, y + i, r, g, b);
                    }
                }
            }
        } else {
            // Draw fallback colored square
            int size = std::min(icon->width, icon->height);
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    DrawPixel(x + j, y + i, fallbackColor, fallbackColor, fallbackColor);
                }
            }
        }
    }

    void drawIconScaled(const Icon* icon, int x, int y, int slotWidth, int slotHeight, int fallbackColor) {
        if (!icon) return;

        if (icon->data) {
            // Calculate scale to fit slot
            float scaleX = (float)slotWidth / icon->width;
            float scaleY = (float)slotHeight / icon->height;
            float scale = std::min(scaleX, scaleY);

            int newW = (int)(icon->width * scale);
            int newH = (int)(icon->height * scale);

            // Center in slot
            int offsetX = x + (slotWidth - newW) / 2;
            int offsetY = y + (slotHeight - newH) / 2;

            // Draw scaled icon
            for (int i = 0; i < newH; i++) {
                for (int j = 0; j < newW; j++) {
                    int srcX = (int)((float)j / newW * icon->width);
                    int srcY = (int)((float)i / newH * icon->height);
                    uint8_t* pixel = &icon->data[(srcY * icon->width + srcX) * 4];
                    uint8_t r = pixel[0];
                    uint8_t g = pixel[1];
                    uint8_t b = pixel[2];
                    uint8_t a = pixel[3];

                    if (a > 10) {
                        DrawPixel(offsetX + j, offsetY + i, r, g, b);
                    }
                }
            }
        } else {
            // Draw fallback colored square (centered in slot)
            int size = std::min(slotWidth, slotHeight) - 4;
            int centerX = x + (slotWidth - size) / 2;
            int centerY = y + (slotHeight - size) / 2;

            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    DrawPixel(centerX + j, centerY + i, fallbackColor, fallbackColor, fallbackColor);
                }
            }
        }
    }

    int loadDefaultIcons() {
        int count = 0;

        const char* names[] = {
            ICON_SETTINGS,
            ICON_AVATAR_EDITOR,
            ICON_STUDIO,
            ICON_MULTIPLAYER,
            ICON_QUIT,
            ICON_VOICE_CHAT,
            ICON_SPEAKER,
        };

        for (const char* name : names) {
            loadIcon(name, 48, 48);
            count++;
        }

        printf("[IconLoader] Loaded %d default icons\n", count);
        return count;
    }

    Icon* getIcon(const std::string& name) {
        for (auto* icon : g_icons) {
            if (icon->path.find(name) != std::string::npos) {
                return icon;
            }
        }
        return nullptr;
    }

    void clearCache() {
        for (auto* icon : g_icons) {
            freeIcon(icon);
        }
        g_icons.clear();
    }
}