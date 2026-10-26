#include "map_metadata.h"
#include <map>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

namespace MapMetadata {
    static std::map<std::string, MapInfo> cache;

    void init() {
        loadCache();
    }

    MapInfo scanMap(const std::string& filePath) {
        MapInfo info;
        info.filePath = filePath;

        struct stat st;
        if (stat(filePath.c_str(), &st) == 0) {
            info.fileSize = st.st_size;
            info.creationDate = (u64)st.st_ctime;
            info.lastModifiedDate = (u64)st.st_mtime;
        }

        info.name = filePath.substr(filePath.find_last_of("/\\") + 1);
        size_t dot = info.name.find_last_of('.');
        if (dot != std::string::npos) info.name = info.name.substr(0, dot);

        info.creator = "";
        info.starRating = 0;
        info.avgRating = 0.0f;
        info.reviewCount = 0;
        info.thumbnailPath = "";
        info.hasPreview = false;

        return info;
    }

    MapInfo* getMetadata(const std::string& filePath) {
        auto it = cache.find(filePath);
        if (it != cache.end()) return &it->second;
        return nullptr;
    }

    void updateCache(const std::string& filePath, const MapInfo& info) {
        cache[filePath] = info;
    }

    std::string formatFileSize(u64 bytes) {
        char buf[64];
        if (bytes < 1024) snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
        else if (bytes < 1024 * 1024) snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
        else snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
        return buf;
    }

    std::string formatDate(u64 timestamp) {
        time_t t = (time_t)timestamp;
        struct tm* tm = localtime(&t);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
        return buf;
    }

    void drawInfoCard(const MapInfo& info, s32 x, s32 y) {
        // Draw info card using GX/Citro2D
        (void)info; (void)x; (void)y;
    }

    void drawStars(int rating, s32 x, s32 y, float scale) {
        (void)rating; (void)x; (void)y; (void)scale;
    }

    void drawThumbnail(const MapInfo& info, s32 x, s32 y, s32 w, s32 h) {
        (void)info; (void)x; (void)y; (void)w; (void)h;
    }

    bool saveCache() {
        FILE* f = fopen("sdmc:/OpenBLOX/metadata_cache.dat", "wb");
        if (!f) return false;

        u32 count = (u32)cache.size();
        fwrite(&count, sizeof(u32), 1, f);

        for (auto& p : cache) {
            char path[256], name[64];
            strncpy(path, p.first.c_str(), 255); path[255] = 0;
            strncpy(name, p.second.name.c_str(), 63); name[63] = 0;
            fwrite(path, 256, 1, f);
            fwrite(name, 64, 1, f);
            fwrite(&p.second.fileSize, sizeof(u64), 1, f);
            fwrite(&p.second.lastModifiedDate, sizeof(u64), 1, f);
            fwrite(&p.second.starRating, sizeof(int), 1, f);
        }
        fclose(f);
        return true;
    }

    bool loadCache() {
        FILE* f = fopen("sdmc:/OpenBLOX/metadata_cache.dat", "rb");
        if (!f) return true;

        u32 count = 0;
        if (fread(&count, sizeof(u32), 1, f) != 1) { fclose(f); return false; }

        for (u32 i = 0; i < count; i++) {
            MapInfo info;
            char path[256], name[64];
            fread(path, 256, 1, f);
            fread(name, 64, 1, f);
            info.filePath = path;
            info.name = name;
            fread(&info.fileSize, sizeof(u64), 1, f);
            fread(&info.lastModifiedDate, sizeof(u64), 1, f);
            fread(&info.starRating, sizeof(int), 1, f);
            cache[path] = info;
        }
        fclose(f);
        return true;
    }

    void shutdown() { saveCache(); }
}