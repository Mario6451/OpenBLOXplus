#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <3ds.h>

// Web asset loader with offline caching for 3DS OpenBLOX
// Downloads assets from Roblox assetdelivery, caches to shareddata/assets
// Logs downloads to shareddata/logs

namespace AssetLoader {

    // Asset type
    enum class AssetType {
        Unknown,
        Texture,
        Sound,
        Mesh,
        Decal,
        Font,
        Script,
        Place
    };

    // Download status
    enum class DownloadStatus {
        Pending,
        InProgress,
        Completed,
        Failed,
        Cached,
        Offline
    };

    // Download entry
    struct DownloadEntry {
        std::string assetId;
        std::string assetUrl;
        std::string localPath;
        AssetType type;
        DownloadStatus status;
        int progress;     // 0-100
        u32 fileSize;
        u32 downloaded;
        std::string error;
    };

    // Initialize asset loader
    void init();

    // Set base assetdelivery URL (default: https://assetdelivery.roblox.com)
    void setAssetBaseUrl(const std::string& url);

    // Get base URL
    const std::string& getAssetBaseUrl();

    // Check if asset is available (cached or downloadable)
    bool isAssetAvailable(const std::string& assetId, AssetType type);

    // Get local path for an asset (returns cached path if available)
    std::string getAssetLocalPath(const std::string& assetId, AssetType type);

    // Start downloading an asset
    // Returns true if download started, false if already cached or error
    bool startDownload(const std::string& assetId, AssetType type, const std::string& url = "");

    // Get download status
    DownloadEntry& getDownload(const std::string& assetId);

    // Update download progress (call once per frame)
    void updateDownloads();

    // Cancel a download
    void cancelDownload(const std::string& assetId);

    // Cancel all downloads
    void cancelAllDownloads();

    // Get all active downloads
    std::vector<DownloadEntry*> getActiveDownloads();

    // Get asset from cache (returns file contents)
    bool getAssetFromCache(const std::string& assetId, AssetType type, std::vector<u8>& data);

    // Cache asset to local storage
    bool cacheAsset(const std::string& assetId, AssetType type, const u8* data, u32 size);

    // Clean cache (remove assets older than maxAgeDays)
    void cleanCache(int maxAgeDays = 30);

    // Clear all cached assets
    void clearAllCache();

    // Get cache size in bytes
    u64 getCacheSize();

    // Get cache entry count
    int getCacheCount();

    // Set offline mode (no new downloads, use cache only)
    void setOfflineMode(bool offline);

    // Get offline mode
    bool isOfflineMode();

    // Log download to shareddata/logs
    void logDownload(const std::string& assetId, const std::string& url, bool success, const std::string& error = "");

    // Get download log entries
    std::vector<std::string> getDownloadLog();

    // Shutdown
    void shutdown();
}