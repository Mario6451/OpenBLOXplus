#include "ui_asset_loader.h"
#include "font_manager.h"
#include "citro_renderer.h"
#include "stb_image.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

namespace UIAssetLoader {
    using Settings::ContentYear;

    static ContentYear g_currentYear = ContentYear::Year2012;
    static std::map<std::string, UIAsset*> g_assetCache;

    // Year-to-folder mapping
    struct YearMapping {
        ContentYear year;
        const char* uiFolder;      // assets/ui{YEAR}/
        const char* loadingFolder; // assets/loading{YEAR}/
        const char* particleFolder;// assets/particles{YEAR}/
        const char* gameFolder;    // content{YEAR}/
        bool hasUI;                // true if UI folder exists
    };

    static const YearMapping s_yearMap[] = {
        {ContentYear::Year2007, "", "", "", "content2007", false},
        {ContentYear::Year2011, "", "", "", "content2012", false},
        {ContentYear::Year2012, "", "", "", "content2012", false},
        {ContentYear::Year2015, "ui2015", "loading2015", "particles2015", "content2015", true},
        {ContentYear::Year2016, "ui2016", "loading2016", "particles2016", "content2016", true},
        {ContentYear::Year2019, "ui2019", "loading2019", "particles2019", "content2019", true},
        {ContentYear::Year2022, "ui2022", "loading2022", "particles2022", "content2022", true},
    };
    static const int s_yearMapSize = sizeof(s_yearMap) / sizeof(s_yearMap[0]);

    void init() {
        setContentYear(ContentYear::Year2012);
    }

    void shutdown() {
        for (auto& pair : g_assetCache) {
            if (pair.second && pair.second->data) {
                free(pair.second->data);
            }
            delete pair.second;
        }
        g_assetCache.clear();
    }

    void setContentYear(ContentYear year) {
        g_currentYear = year;
        printf("[UIAssetLoader] Content year set to %d\n", (int)year);
    }

    ContentYear getContentYear() {
        return g_currentYear;
    }

    void reloadAllAssets() {
        // Clear cache
        for (auto& pair : g_assetCache) {
            if (pair.second && pair.second->data) {
                free(pair.second->data);
            }
            delete pair.second;
        }
        g_assetCache.clear();

        // Reload loading screen
        loadLoadingScreen();
        loadParticles();

        printf("[UIAssetLoader] All assets reloaded for year %d\n", (int)g_currentYear);
    }

    UIAsset* loadUIAsset(const std::string& name) {
        // Check cache
        auto it = g_assetCache.find(name);
        if (it != g_assetCache.end()) {
            return it->second;
        }

        // No UI assets for 2007/2012
        if (!hasUIAssets()) {
            return nullptr;
        }

        std::string path = getUIPath(name + ".png");

        // Check if file exists
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            return nullptr;
        }
        fclose(f);

        // Load using stb_image for PNG decoding
        // Integrated with ThirdParty/stb_image.h
        UIAsset* asset = new UIAsset();
        asset->name = name;
        asset->path = path;

        int w = 0, h = 0, channels = 0;
        uint8_t* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
        if (pixels) {
            asset->width = w;
            asset->height = h;
            asset->data = pixels; // stbi_load allocates; caller must free
        } else {
            // Fallback: mark as loaded but without pixel data
            asset->width = 0;
            asset->height = 0;
            asset->data = nullptr;
        }

        g_assetCache[name] = asset;
        return asset;
    }

    void loadLoadingScreen() {
        // Loading screen assets are in assets/loading{YEAR}/
        // Key files: loadingTexture.png, loadingvignette.png, robloxlogo.png
        // Optional: loadingCircle.png, cancelButton.png, robloxTilt.png
        if (!hasLoadingAssets()) {
            printf("[UIAssetLoader] No loading screen assets for this year\n");
            return;
        }

        const char* files[] = {
            "loadingTexture.png", "loadingvignette.png", "robloxlogo.png",
            "loadingCircle.png", "cancelButton.png"
        };

        for (const char* file : files) {
            std::string path = getLoadingPath(file);
            FILE* f = fopen(path.c_str(), "rb");
            if (f) {
                fclose(f);
                printf("[UIAssetLoader] Loading screen asset: %s\n", file);
            }
        }
    }

    std::string getLoadingScreenPath() {
        if (!hasLoadingAssets()) {
            return "";
        }
        return getLoadingPath("loadingTexture.png");
    }

    void loadParticles() {
        if (!hasParticles()) {
            printf("[UIAssetLoader] No particle assets for this year\n");
            return;
        }

        std::string path = getParticlesPath();
        printf("[UIAssetLoader] Particle path: %s\n", path.c_str());
    }

    std::string getParticlesPath() {
        if (!hasParticles()) {
            return "";
        }
        return getParticlePath("");
    }

    bool hasParticles() {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == g_currentYear) {
                return strlen(s_yearMap[i].particleFolder) > 0;
            }
        }
        return false;
    }

    bool hasUIAssets() {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == g_currentYear) {
                return s_yearMap[i].hasUI;
            }
        }
        return false;
    }

    bool hasLoadingAssets() {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == g_currentYear) {
                return strlen(s_yearMap[i].loadingFolder) > 0;
            }
        }
        return false;
    }

    void drawTextButton(const std::string& text, int x, int y, int w, int h, bool hovered) {
        // Classic ROBLOX 2007 style: blue button with white border
        int r = hovered ? 80 : 40, g = hovered ? 120 : 80, b = hovered ? 200 : 180;

        // Fill button
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                DrawPixel(x + dx, y + dy, r, g, b);
            }
        }

        // White border
        for (int dx = 0; dx < w; dx++) {
            DrawPixel(x + dx, y, 255, 255, 255);
            DrawPixel(x + dx, y + h - 1, 255, 255, 255);
        }
        for (int dy = 0; dy < h; dy++) {
            DrawPixel(x, y + dy, 255, 255, 255);
            DrawPixel(x + w - 1, y + dy, 255, 255, 255);
        }

        // Text (centered)
        FontManager::drawText(text.c_str(), x + w / 2 - (int)text.length() * 3, y + h / 2 - 4,
                             255, 255, 255, FontSize::Size10);
    }

    void drawTextField(const std::string& label, const std::string& value, int x, int y, int w, bool active) {
        // Label
        FontManager::drawText(label.c_str(), x, y, 180, 180, 180, FontSize::Size10);

        // Input box
        int boxY = y + 12;
        for (int dy = 0; dy < 16; dy++) {
            for (int dx = 0; dx < w; dx++) {
                DrawPixel(x + dx, boxY + dy, 30, 30, 40);
            }
        }

        // Border
        for (int dx = 0; dx < w; dx++) {
            DrawPixel(x + dx, boxY, active ? 100 : 80, active ? 150 : 80, 200);
            DrawPixel(x + dx, boxY + 15, active ? 100 : 80, active ? 150 : 80, 200);
        }
        for (int dy = 0; dy < 16; dy++) {
            DrawPixel(x, boxY + dy, active ? 100 : 80, active ? 150 : 80, 200);
            DrawPixel(x + w - 1, boxY + dy, active ? 100 : 80, active ? 150 : 80, 200);
        }

        // Value
        FontManager::drawText(value.c_str(), x + 2, boxY + 2, 255, 255, 255, FontSize::Size10);
    }

    void drawPanel(int x, int y, int w, int h) {
        // Semi-transparent background panel
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                DrawPixel(x + dx, y + dy, 20, 20, 30);
            }
        }

        // Border
        for (int dx = 0; dx < w; dx++) {
            DrawPixel(x + dx, y, 60, 60, 80);
            DrawPixel(x + dx, y + h - 1, 60, 60, 80);
        }
        for (int dy = 0; dy < h; dy++) {
            DrawPixel(x, y + dy, 60, 60, 80);
            DrawPixel(x + w - 1, y + dy, 60, 60, 80);
        }
    }

    std::string getUIPath(const std::string& assetName) {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == g_currentYear && strlen(s_yearMap[i].uiFolder) > 0) {
                return std::string("assets/") + s_yearMap[i].uiFolder + "/" + assetName;
            }
        }
        return assetName;
    }

    std::string getLoadingPath(const std::string& assetName) {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == g_currentYear && strlen(s_yearMap[i].loadingFolder) > 0) {
                return std::string("assets/") + s_yearMap[i].loadingFolder + "/" + assetName;
            }
        }
        return assetName;
    }

    std::string getParticlePath(const std::string& assetName) {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == g_currentYear && strlen(s_yearMap[i].particleFolder) > 0) {
                std::string result = std::string("assets/") + s_yearMap[i].particleFolder + "/";
                if (!assetName.empty()) result += assetName;
                return result;
            }
        }
        return "";
    }

    std::string getGameTexturePath(const std::string& assetName) {
        std::string base = getGameContentPath(g_currentYear);
        return base + "/textures/" + assetName;
    }

    std::string yearToFolderName(ContentYear year) {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == year) {
                return s_yearMap[i].gameFolder;
            }
        }
        return "content2012";
    }

    std::string getGameContentPath(ContentYear year) {
        for (int i = 0; i < s_yearMapSize; i++) {
            if (s_yearMap[i].year == year) {
                return std::string("sdmc:/OpenBLOX/") + s_yearMap[i].gameFolder;
            }
        }
        return "sdmc:/OpenBLOX/content2012";
    }
}
