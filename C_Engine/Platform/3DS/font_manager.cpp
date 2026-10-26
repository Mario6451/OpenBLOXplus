#include "font_manager.h"
#include "../../ThirdParty/stb_truetype.h"
#include "texture_loader.h"
#include "citro_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

// Per-font data structure
struct FontData {
    unsigned char* ttf_buffer;
    stbtt_bakedchar cdata_12[96], cdata_16[96], cdata_20[96], cdata_24[96], cdata_32[96];
    void* texture_12, *texture_16, *texture_20, *texture_24, *texture_32;
    bool loaded;
    std::string path;
};

static FontData fonts[MAX_FONTS];
static int font_count = 0;
static const int atlas_w = 512, atlas_h = 512;

// Internal helper: find or allocate a font slot
static FontData* getFontSlot(int index) {
    if (index < 0 || index >= MAX_FONTS) return nullptr;
    return &fonts[index];
}

static int getFontHeight(FontSize size) {
    switch (size) {
        case FontSize::Size8:  return 8;
        case FontSize::Size10: return 10;
        case FontSize::Size12: return 12;
        case FontSize::Size16: return 16;
        case FontSize::Size20: return 20;
        case FontSize::Size24: return 24;
        case FontSize::Size32: return 32;
        default: return 16;
    }
}

static const stbtt_bakedchar* getCdata(const FontData* fd, FontSize size) {
    if (!fd) return nullptr;
    switch (size) {
        case FontSize::Size8:
        case FontSize::Size10:
        case FontSize::Size12: return fd->cdata_12;
        case FontSize::Size16: return fd->cdata_16;
        case FontSize::Size20: return fd->cdata_20;
        case FontSize::Size24: return fd->cdata_24;
        case FontSize::Size32: return fd->cdata_32;
        default: return fd->cdata_16;
    }
}

static void* getTexture(const FontData* fd, FontSize size) {
    if (!fd) return nullptr;
    switch (size) {
        case FontSize::Size8:
        case FontSize::Size10:
        case FontSize::Size12: return fd->texture_12;
        case FontSize::Size16: return fd->texture_16;
        case FontSize::Size20: return fd->texture_20;
        case FontSize::Size24: return fd->texture_24;
        case FontSize::Size32: return fd->texture_32;
        default: return fd->texture_16;
    }
}

// Map font face + style to TTF file path
string FontManager::getFontPath(FontFace face, FontStyle style) {
    const char* basePath = "";
    const char* filename = "";
    
    switch (face) {
        case FontFace::Arial:
            basePath = "sdmc:/OpenBLOX/content2016/fonts/";
            switch (style) {
                case FontStyle::Bold:       filename = "arialbd.ttf"; break;
                case FontStyle::Italic:     filename = "ariali.ttf"; break;
                case FontStyle::BoldItalic: filename = "arialbi.ttf"; break;
                default:                    filename = "arial.ttf"; break;
            }
            break;
            
        case FontFace::ComicSans:
            basePath = "sdmc:/OpenBLOX/content2016/fonts/";
            filename = "ComicNeue-Angular-Bold.ttf";
            break;
            
        case FontFace::SourceSansPro:
            basePath = "sdmc:/OpenBLOX/content2016/fonts/";
            switch (style) {
                case FontStyle::Light:      filename = "SourceSansPro-Light.ttf"; break;
                case FontStyle::Semibold:   filename = "SourceSansPro-Semibold.ttf"; break;
                case FontStyle::Bold:       filename = "SourceSansPro-Bold.ttf"; break;
                case FontStyle::Italic:     filename = "SourceSansPro-It.ttf"; break;
                case FontStyle::BoldItalic: filename = "SourceSansPro-BoldIt.ttf"; break;
                default:                   filename = "SourceSansPro-Regular.ttf"; break;
            }
            break;
            
        case FontFace::Legacy:
            // Legacy font is a bitmap font - return empty, handled separately
            return "";
            
        default:
            basePath = "sdmc:/OpenBLOX/content2016/fonts/";
            filename = "arial.ttf";
            break;
    }
    
    return string(basePath) + filename;
}

// Load a specific font by face and style
bool FontManager::loadFont(FontFace face, FontStyle style) {
    if (face == FontFace::Legacy) {
        // Legacy bitmap font - handled by content2012 fonts
        iprintf("Legacy font uses bitmap font from content2012, skipping TTF load.\n");
        return true;
    }
    
    // Check if already loaded
    for (int i = 0; i < font_count; i++) {
        if (fonts[i].loaded) {
            string path = getFontPath(face, style);
            if (fonts[i].path == path) return true;
        }
    }
    
    if (font_count >= MAX_FONTS) return false;
    
    string ttfPath = getFontPath(face, style);
    FILE* f = fopen(ttfPath.c_str(), "rb");
    if (!f) {
        iprintf("Font load failed: %s\n", ttfPath.c_str());
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char* ttf_buffer = (unsigned char*)malloc(size);
    if (!ttf_buffer) { fclose(f); return false; }
    fread(ttf_buffer, 1, size, f);
    fclose(f);
    
    FontData* fd = &fonts[font_count];
    fd->ttf_buffer = ttf_buffer;
    fd->path = ttfPath;
    fd->loaded = false;
    fd->texture_12 = fd->texture_16 = fd->texture_20 = fd->texture_24 = fd->texture_32 = nullptr;
    
    // Bake all sizes
    bool success = true;
    for (int s = 12; s <= 32; s += 4) {
        unsigned char* baked = (unsigned char*)malloc(atlas_w * atlas_h);
        if (!baked) { success = false; break; }
        
        stbtt_bakedchar* chars = nullptr;
        if (s == 12) chars = fd->cdata_12;
        else if (s == 16) chars = fd->cdata_16;
        else if (s == 20) chars = fd->cdata_20;
        else if (s == 24) chars = fd->cdata_24;
        else if (s == 32) chars = fd->cdata_32;
        
        if (!chars) { free(baked); break; }
        
        int ret = stbtt_BakeFontBitmap(ttf_buffer, 0, (float)s, baked, atlas_w, atlas_h, 32, 96, chars);
        if (ret <= 0) { free(baked); success = false; break; }
        
        // Convert to RGBA
        unsigned char* rgba = (unsigned char*)malloc(atlas_w * atlas_h * 4);
        if (!rgba) { free(baked); success = false; break; }
        
        for (int i = 0; i < atlas_w * atlas_h; ++i) {
            unsigned char v = baked[i];
            rgba[i*4+0] = 255;
            rgba[i*4+1] = 255;
            rgba[i*4+2] = 255;
            rgba[i*4+3] = v;
        }
        
        TextureData td;
        td.width = atlas_w; td.height = atlas_h; td.channels = 4; td.pixels = rgba;
        void* tex = TextureLoader::createC3DTexture(td);
        
        if (s == 12) fd->texture_12 = tex;
        else if (s == 16) fd->texture_16 = tex;
        else if (s == 20) fd->texture_20 = tex;
        else if (s == 24) fd->texture_24 = tex;
        else if (s == 32) fd->texture_32 = tex;
        
        free(baked);
        free(rgba);
    }
    
    if (success) {
        fd->loaded = true;
        font_count++;
        iprintf("Font loaded: %s (slot %d, total: %d)\n", ttfPath.c_str(), font_count - 1, font_count);
    }
    
    return success;
}

// Initialize all classic Roblox fonts
bool FontManager::initClassicFonts() {
    bool all_ok = true;
    
    // Arial variants
    all_ok &= loadFont(FontFace::Arial, FontStyle::Regular);
    all_ok &= loadFont(FontFace::Arial, FontStyle::Bold);
    all_ok &= loadFont(FontFace::Arial, FontStyle::Italic);
    all_ok &= loadFont(FontFace::Arial, FontStyle::BoldItalic);
    
    // Comic Sans
    all_ok &= loadFont(FontFace::ComicSans, FontStyle::Regular);
    
    // Source Sans Pro variants
    all_ok &= loadFont(FontFace::SourceSansPro, FontStyle::Regular);
    all_ok &= loadFont(FontFace::SourceSansPro, FontStyle::Bold);
    all_ok &= loadFont(FontFace::SourceSansPro, FontStyle::Italic);
    all_ok &= loadFont(FontFace::SourceSansPro, FontStyle::Light);
    all_ok &= loadFont(FontFace::SourceSansPro, FontStyle::Semibold);
    
    // Legacy (bitmap - already available in content2012)
    all_ok &= loadFont(FontFace::Legacy, FontStyle::Regular);
    
    iprintf("Classic fonts initialized: %s\n", all_ok ? "OK" : "PARTIAL");
    return all_ok;
}

// Legacy initFont for backward compatibility
bool FontManager::initFont(const std::string &ttfPath, int fontHeight) {
    // Find existing font or load new one
    if (font_count >= MAX_FONTS) return false;
    
    FILE* f = fopen(ttfPath.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char* ttf_buffer = (unsigned char*)malloc(size);
    if (!ttf_buffer) { fclose(f); return false; }
    fread(ttf_buffer, 1, size, f);
    fclose(f);
    
    FontData* fd = &fonts[font_count];
    fd->ttf_buffer = ttf_buffer;
    fd->path = ttfPath;
    fd->loaded = false;
    fd->texture_12 = fd->texture_16 = fd->texture_20 = fd->texture_24 = fd->texture_32 = nullptr;
    
    bool success = true;
    int heights[] = {12, 16, 20, 24, 32};
    for (int h : heights) {
        unsigned char* baked = (unsigned char*)malloc(atlas_w * atlas_h);
        if (!baked) { success = false; break; }
        
        stbtt_bakedchar* chars = nullptr;
        if (h == 12) chars = fd->cdata_12;
        else if (h == 16) chars = fd->cdata_16;
        else if (h == 20) chars = fd->cdata_20;
        else if (h == 24) chars = fd->cdata_24;
        else if (h == 32) chars = fd->cdata_32;
        
        if (!chars) { free(baked); break; }
        
        int ret = stbtt_BakeFontBitmap(ttf_buffer, 0, (float)h, baked, atlas_w, atlas_h, 32, 96, chars);
        if (ret <= 0) { free(baked); success = false; break; }
        
        unsigned char* rgba = (unsigned char*)malloc(atlas_w * atlas_h * 4);
        if (!rgba) { free(baked); success = false; break; }
        
        for (int i = 0; i < atlas_w * atlas_h; ++i) {
            unsigned char v = baked[i];
            rgba[i*4+0] = 255;
            rgba[i*4+1] = 255;
            rgba[i*4+2] = 255;
            rgba[i*4+3] = v;
        }
        
        TextureData td;
        td.width = atlas_w; td.height = atlas_h; td.channels = 4; td.pixels = rgba;
        void* tex = TextureLoader::createC3DTexture(td);
        
        if (h == 12) fd->texture_12 = tex;
        else if (h == 16) fd->texture_16 = tex;
        else if (h == 20) fd->texture_20 = tex;
        else if (h == 24) fd->texture_24 = tex;
        else if (h == 32) fd->texture_32 = tex;
        
        free(baked);
        free(rgba);
    }
    
    if (success) {
        fd->loaded = true;
        font_count++;
    }
    
    return success;
}

void FontManager::shutdownFont() {
    for (int i = 0; i < font_count; i++) {
        if (fonts[i].ttf_buffer) {
            free(fonts[i].ttf_buffer);
            fonts[i].ttf_buffer = nullptr;
        }
        fonts[i].loaded = false;
        fonts[i].texture_12 = fonts[i].texture_16 = fonts[i].texture_20 = fonts[i].texture_24 = fonts[i].texture_32 = nullptr;
    }
    font_count = 0;
}

// Find font by face and style
static FontData* findFont(FontFace face, FontStyle style) {
    string target = FontManager::getFontPath(face, style);
    if (face == FontFace::Legacy) return nullptr; // Legacy uses bitmap
    
    for (int i = 0; i < font_count; i++) {
        if (fonts[i].loaded && fonts[i].path == target) {
            return &fonts[i];
        }
    }
    return nullptr;
}

int FontManager::drawText(int x, int y, const std::string &text, uint32_t color) {
    iprintf("%s", text.c_str());
    return (int)text.size() * 8;
}

int FontManager::drawText(const char* text, int x, int y, int r, int g, int b, FontSize size) {
    return drawTextWithFont(text, x, y, r, g, b, FontFace::Arial, FontStyle::Regular, size);
}

int FontManager::drawTextWithFont(const char* text, int x, int y, int r, int g, int b,
                                   FontFace face, FontStyle style, FontSize size) {
    if (!text) return 0;
    
    FontData* fd = findFont(face, style);
    if (!fd) {
        // Fallback to any loaded font
        for (int i = 0; i < font_count; i++) {
            if (fonts[i].loaded) { fd = &fonts[i]; break; }
        }
    }
    
    if (!fd) {
        iprintf("%s", text);
        return (int)strlen(text) * 8;
    }
    
    const stbtt_bakedchar* chars = getCdata(fd, size);
    if (!chars) return 0;
    
    consoleSelect(NULL);
    iprintf("\x1b[%d;%dH%s", (y / 8) + 1, (x / 8), text);
    
    int width = 0;
    for (const char* c = text; *c; c++) {
        unsigned int ch = (unsigned char)*c;
        if (ch >= 32 && ch < 128) {
            width += chars[ch - 32].xadvance;
        }
    }
    return width;
}

int FontManager::getTextWidth(const std::string &text, FontSize size) {
    return getTextWidthWithFont(text, FontFace::Arial, FontStyle::Regular, size);
}

int FontManager::getTextWidthWithFont(const std::string &text, FontFace face, FontStyle style, FontSize size) {
    FontData* fd = findFont(face, style);
    if (!fd) {
        for (int i = 0; i < font_count; i++) {
            if (fonts[i].loaded) { fd = &fonts[i]; break; }
        }
    }
    
    if (!fd) return (int)text.size() * 8;
    
    const stbtt_bakedchar* chars = getCdata(fd, size);
    if (!chars) return (int)text.size() * 8;
    
    int width = 0;
    for (char c : text) {
        unsigned int ch = (unsigned char)c;
        if (ch >= 32 && ch < 128) {
            width += chars[ch - 32].xadvance;
        }
    }
    return width;
}

int FontManager::drawTextTop(const char* text, int x, int y, int r, int g, int b, FontSize size) {
    return drawTextTopWithFont(text, x, y, r, g, b, FontFace::Arial, FontStyle::Regular, size);
}

int FontManager::drawTextTopWithFont(const char* text, int x, int y, int r, int g, int b,
                                     FontFace face, FontStyle style, FontSize size) {
    if (!text) return 0;
    
    FontData* fd = findFont(face, style);
    if (!fd) {
        for (int i = 0; i < font_count; i++) {
            if (fonts[i].loaded) { fd = &fonts[i]; break; }
        }
    }
    
    if (!fd) {
        consoleSelect(NULL);
        iprintf("\x1b[%d;%dH%s", (y / 8) + 1, (x / 8), text);
        return (int)strlen(text) * 8;
    }
    
    const stbtt_bakedchar* chars = getCdata(fd, size);
    if (!chars) return 0;
    
    consoleSelect(NULL);
    iprintf("\x1b[%d;%dH%s", (y / 8) + 1, (x / 8), text);
    
    int width = 0;
    for (const char* c = text; *c; c++) {
        unsigned int ch = (unsigned char)*c;
        if (ch >= 32 && ch < 128) {
            width += chars[ch - 32].xadvance;
        }
    }
    return width;
}

bool FontManager::isFontLoaded(FontFace face, FontStyle style) {
    if (face == FontFace::Legacy) return true;
    return findFont(face, style) != nullptr;
}
