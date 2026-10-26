#pragma once
#include <string>
#include <vector>
#include <map>
#include <3ds.h>
#include "ndtypes.h"

// Map metadata storage and display for main menu previews

namespace MapMetadata {

    struct MapInfo {
        std::string filePath;
        std::string name;
        std::string creator;
        u64 fileSize;            // bytes
        u64 creationDate;        // seconds since epoch
        u64 lastModifiedDate;
        int starRating;          // 1-5, 0 = unrated
        float avgRating;         // average from reviews (0.0-5.0)
        int reviewCount;
        std::string thumbnailPath;
        bool hasPreview;
        std::vector<std::string> tags;
    };

    // Initialize metadata system
    void init();

    // Scan and extract metadata from a .rbxl file
    MapInfo scanMap(const std::string& filePath);

    // Get cached metadata
    MapInfo* getMetadata(const std::string& filePath);

    // Update metadata cache
    void updateCache(const std::string& filePath, const MapInfo& info);

    // Get human-readable file size
    std::string formatFileSize(u64 bytes);

    // Get human-readable date
    std::string formatDate(u64 timestamp);

    // Draw map info card on bottom screen
    void drawInfoCard(const MapInfo& info, s32 x, s32 y);

    // Draw star rating graphic
    void drawStars(int rating, s32 x, s32 y, float scale);

    // Draw thumbnail image from cached data
    void drawThumbnail(const MapInfo& info, s32 x, s32 y, s32 w, s32 h);

    // Save/load metadata cache
    bool saveCache();
    bool loadCache();

    // Shutdown
    void shutdown();

} // namespace MapMetadata