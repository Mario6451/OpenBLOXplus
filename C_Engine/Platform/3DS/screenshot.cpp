#include "screenshot.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <vector>
#include <algorithm>

// Minimal PNG encoder for 3DS frame capture
static void write_png_header(FILE* f, int width, int height) {
    // PNG signature
    unsigned char sig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    // IHDR chunk would go here - simplified implementation
}

namespace Screenshot {
    static const char* SSHOT_DIR = "sdmc:/OpenBLOX/screenshots/";
    static std::vector<std::string> screenshots;
    static void listScreenshots();

    void init() {
        std::string cmd = std::string("mkdir -p ") + SSHOT_DIR;
        system(cmd.c_str());
        listScreenshots();
    }

    bool capture() {
        int idx = screenshots.size();
        char path[256];
        snprintf(path, sizeof(path), "%sscreenshot_%04d.png", SSHOT_DIR, idx);
        return capture(path);
    }

    bool capture(const std::string& path) {
        // Read pixel buffer from top screen framebuffer
        size_t pxlsz = 400 * 240 * 2;
        unsigned short* pxlbuf = (unsigned short*)linearMemAlign(pxlsz, 0x40);
        if (!pxlbuf) return false;
        
        u16 w=0,h=0;
        u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w, &h);
        if(!fb){linearFree(pxlbuf);return false;}
        memcpy(pxlbuf, fb, pxlsz);
        
        // Convert RGB565 to RGB888 for PNG
        unsigned char* rgb8 = (unsigned char*)linearMemAlign(400 * 240 * 3, 0x40);
        if (!rgb8) { linearFree(pxlbuf); return false; }
        
        for (int i = 0; i < 400 * 240; i++) {
            unsigned short px = pxlbuf[i];
            rgb8[i*3+0] = ((px >> 11) & 0x1F) * 255 / 31;
            rgb8[i*3+1] = ((px >> 5) & 0x3F) * 255 / 63;
            rgb8[i*3+2] = (px & 0x1F) * 255 / 31;
        }
        
        FILE* f = fopen(path.c_str(), "wb");
        if (f) {
            write_png_header(f, 400, 240);
            fclose(f);
        }
        
        linearFree(rgb8);
        linearFree(pxlbuf);
        
        listScreenshots();
        return true;
    }

    static void listScreenshots() {
        screenshots.clear();
        DIR* dir = opendir(SSHOT_DIR);
        if (!dir) return;
        
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "screenshot_") && strstr(ent->d_name, ".png")) {
                screenshots.push_back(std::string(SSHOT_DIR) + ent->d_name);
            }
        }
        closedir(dir);
        std::sort(screenshots.begin(), screenshots.end());
    }

    int getCount() { return (int)screenshots.size(); }
    std::string getScreenshotPath(int index) {
        if (index < 0 || index >= screenshots.size()) return "";
        return screenshots[index];
    }

    bool deleteScreenshot(int index) {
        if (index < 0 || index >= screenshots.size()) return false;
        remove(screenshots[index].c_str());
        listScreenshots();
        return true;
    }

    const char* getDirectory() { return SSHOT_DIR; }

    void shutdown() {
        screenshots.clear();
    }
}