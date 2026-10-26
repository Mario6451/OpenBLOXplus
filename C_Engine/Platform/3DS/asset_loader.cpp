#include "asset_loader.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <curl/curl.h>

namespace AssetLoader {

    // Internal state
    static std::string baseUrl = "https://assetdelivery.roblox.com";
    static bool offlineMode = false;
    static std::map<std::string, DownloadEntry> downloads;
    static std::vector<std::string> downloadLog;
    static const std::string cacheDir = "sdmc:/OpenBLOX/shareddata/assets/";
    static const std::string logDir = "sdmc:/OpenBLOX/shareddata/logs/";
    static const std::string logFile = logDir + "downloads.log";
    static bool curlInitialized = false;

    // Helper to get file extension for asset type
    static const char* getTypeExtension(AssetType type) {
        switch (type) {
        case AssetType::Texture: return ".png";
        case AssetType::Sound:   return ".ogg";
        case AssetType::Mesh:    return ".mesh";
        case AssetType::Decal:   return ".png";
        case AssetType::Font:    return ".fontdef";
        case AssetType::Script:  return ".lua";
        case AssetType::Place:   return ".rbxl";
        default:                 return ".bin";
        }
    }

    // Helper to get directory for asset type
    static const char* getTypeDir(AssetType type) {
        switch (type) {
        case AssetType::Texture: return "textures";
        case AssetType::Sound:   return "sounds";
        case AssetType::Mesh:    return "meshes";
        case AssetType::Decal:   return "decals";
        case AssetType::Font:    return "fonts";
        case AssetType::Script:  return "scripts";
        case AssetType::Place:   return "places";
        default:                 return "other";
        }
    }

    // Helper to create directory
    static bool createDirectory(const std::string& path) {
        mkdir(path.c_str(), 0755);
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }

    // Helper to write data to file
    static bool writeFile(const std::string& path, const u8* data, u32 size) {
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) return false;
        fwrite(data, 1, size, f);
        fclose(f);
        return true;
    }

    // CURL write callback - appends to vector safely
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        std::vector<u8>* data = (std::vector<u8>*)userp;
        size_t realsize = size * nmemb;
        size_t old_size = data->size();
        data->resize(old_size + realsize);
        memcpy(data->data() + old_size, contents, realsize);
        return realsize;
    }

    // Helper to download HTTP/HTTPS data using libcurl
    static bool httpDownload(const std::string& url, std::vector<u8>& data, DownloadEntry& entry) {
        if (!curlInitialized) {
            entry.error = "CURL not initialized";
            return false;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            entry.error = "Failed to initialize CURL";
            return false;
        }

        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

        // Set timeout (30 seconds)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        // Set user agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "OpenBLOX/0.1.0");

        // Accept all content
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

        // Write callback
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&data);

        // SSL options (disable peer verification for simplicity)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Perform request
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            entry.error = "CURL error: ";
            entry.error += curl_easy_strerror(res);
            printf("CURL error downloading %s: %s\n", url.c_str(), curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return false;
        }

        // Get response code
        long responseCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

        if (responseCode != 200) {
            char buf[64];
            snprintf(buf, sizeof(buf), "HTTP %ld", responseCode);
            entry.error = buf;
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_cleanup(curl);

        if (data.empty()) {
            entry.error = "Empty response from server";
            return false;
        }

        entry.fileSize = (u32)data.size();
        entry.progress = 100;
        return true;
    }

    // Initialize asset loader
    void init() {
        // Initialize libcurl
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
            printf("Failed to initialize libcurl\n");
            return;
        }
        curlInitialized = true;

        offlineMode = Settings::get().general.offlineMode;

        // Create cache directories
        createDirectory(cacheDir);
        createDirectory(logDir);

        // Create type-specific directories
        const char* dirs[] = { "textures", "sounds", "meshes", "decals", "fonts", "scripts", "places", "other" };
        for (const char* dir : dirs) {
            createDirectory(cacheDir + std::string(dir));
        }
    }

    // Set base assetdelivery URL
    void setAssetBaseUrl(const std::string& url) {
        baseUrl = url;
    }

    // Get base URL
    const std::string& getAssetBaseUrl() {
        return baseUrl;
    }

    // Get local path for an asset
    static std::string getLocalPath(const std::string& assetId, AssetType type) {
        return cacheDir + std::string(getTypeDir(type)) + "/" + assetId + getTypeExtension(type);
    }

    // Check if asset is available
    bool isAssetAvailable(const std::string& assetId, AssetType type) {
        std::string path = getLocalPath(assetId, type);
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }

    // Get local path for an asset
    std::string getAssetLocalPath(const std::string& assetId, AssetType type) {
        if (isAssetAvailable(assetId, type)) {
            return getLocalPath(assetId, type);
        }
        return "";
    }

    // Start downloading an asset
    bool startDownload(const std::string& assetId, AssetType type, const std::string& url) {
        // Check if already cached
        if (isAssetAvailable(assetId, type)) {
            return false;
        }

        // Check if already downloading
        if (downloads.find(assetId) != downloads.end()) {
            return false;
        }

        // Check offline mode
        if (offlineMode) {
            DownloadEntry entry;
            entry.assetId = assetId;
            entry.type = type;
            entry.status = DownloadStatus::Offline;
            entry.error = "Offline mode enabled";
            downloads[assetId] = entry;
            return false;
        }

        // Build URL if not provided
        std::string fullUrl = url;
        if (fullUrl.empty()) {
            fullUrl = baseUrl + "/v1/assets/" + assetId;
        }

        // Create download entry
        DownloadEntry entry;
        entry.assetId = assetId;
        entry.assetUrl = fullUrl;
        entry.localPath = getLocalPath(assetId, type);
        entry.type = type;
        entry.status = DownloadStatus::InProgress;
        entry.progress = 0;
        entry.fileSize = 0;
        entry.downloaded = 0;
        downloads[assetId] = entry;

        // Download
        std::vector<u8> data;
        bool success = httpDownload(fullUrl, data, downloads[assetId]);

        if (success) {
            // Cache the asset
            if (writeFile(downloads[assetId].localPath, data.data(), data.size())) {
                downloads[assetId].status = DownloadStatus::Completed;
                logDownload(assetId, fullUrl, true);
            }
            else {
                downloads[assetId].status = DownloadStatus::Failed;
                downloads[assetId].error = "Failed to write cache file";
                logDownload(assetId, fullUrl, false, "Failed to write cache file");
            }
        }
        else {
            downloads[assetId].status = DownloadStatus::Failed;
            logDownload(assetId, fullUrl, false, downloads[assetId].error);
        }

        return success;
    }

    // Get download status
    DownloadEntry& getDownload(const std::string& assetId) {
        static DownloadEntry empty;
        auto it = downloads.find(assetId);
        if (it != downloads.end()) {
            return it->second;
        }
        return empty;
    }

    // Update download progress (no-op with curl, downloads are synchronous)
    void updateDownloads() {
        // Downloads are synchronous with curl, so this is a no-op
    }

    // Cancel a download (no-op with synchronous curl)
    void cancelDownload(const std::string& assetId) {
        downloads.erase(assetId);
    }

    // Cancel all downloads
    void cancelAllDownloads() {
        downloads.clear();
    }

    // Get all active downloads
    std::vector<DownloadEntry*> getActiveDownloads() {
        std::vector<DownloadEntry*> result;
        for (auto& pair : downloads) {
            if (pair.second.status == DownloadStatus::InProgress) {
                result.push_back(&pair.second);
            }
        }
        return result;
    }

    // Get asset from cache
    bool getAssetFromCache(const std::string& assetId, AssetType type, std::vector<u8>& data) {
        std::string path = getLocalPath(assetId, type);
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        data.resize(size);
        fread(data.data(), 1, size, f);
        fclose(f);
        return true;
    }

    // Cache asset to local storage
    bool cacheAsset(const std::string& assetId, AssetType type, const u8* data, u32 size) {
        std::string path = getLocalPath(assetId, type);
        return writeFile(path, data, size);
    }

    // Clean cache (remove assets older than maxAgeDays)
    void cleanCache(int maxAgeDays) {
        // TODO: Implement cache cleaning based on file modification time
    }

    // Clear all cached assets
    void clearAllCache() {
        const char* dirs[] = { "textures", "sounds", "meshes", "decals", "fonts", "scripts", "places", "other" };
        for (const char* dir : dirs) {
            std::string path = cacheDir + std::string(dir);
            DIR* d = opendir(path.c_str());
            if (d) {
                struct dirent* entry;
                while ((entry = readdir(d)) != NULL) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        remove((path + "/" + entry->d_name).c_str());
                    }
                }
                closedir(d);
            }
        }
    }

    // Get cache size in bytes
    u64 getCacheSize() {
        u64 total = 0;
        const char* dirs[] = { "textures", "sounds", "meshes", "decals", "fonts", "scripts", "places", "other" };
        for (const char* dir : dirs) {
            std::string path = cacheDir + std::string(dir);
            DIR* d = opendir(path.c_str());
            if (d) {
                struct dirent* entry;
                while ((entry = readdir(d)) != NULL) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        struct stat st;
                        if (stat((path + "/" + entry->d_name).c_str(), &st) == 0) {
                            total += st.st_size;
                        }
                    }
                }
                closedir(d);
            }
        }
        return total;
    }

    // Get cache entry count
    int getCacheCount() {
        int count = 0;
        const char* dirs[] = { "textures", "sounds", "meshes", "decals", "fonts", "scripts", "places", "other" };
        for (const char* dir : dirs) {
            std::string path = cacheDir + std::string(dir);
            DIR* d = opendir(path.c_str());
            if (d) {
                struct dirent* entry;
                while ((entry = readdir(d)) != NULL) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        count++;
                    }
                }
                closedir(d);
            }
        }
        return count;
    }

    // Set offline mode
    void setOfflineMode(bool offline) {
        offlineMode = offline;
    }

    // Get offline mode
    bool isOfflineMode() {
        return offlineMode;
    }

    // Log download
    void logDownload(const std::string& assetId, const std::string& url, bool success, const std::string& error) {
        FILE* f = fopen(logFile.c_str(), "a");
        if (f) {
            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

            fprintf(f, "[%s] %s %s %s%s\n",
                timestamp,
                success ? "SUCCESS" : "FAILED",
                assetId.c_str(),
                url.c_str(),
                error.empty() ? "" : (" - " + error).c_str());
            fclose(f);
        }

        downloadLog.push_back(std::string(success ? "[SUCCESS]" : "[FAILED]") + " " + assetId);
    }

    // Get download log entries
    std::vector<std::string> getDownloadLog() {
        return downloadLog;
    }

    // Shutdown
    void shutdown() {
        if (curlInitialized) {
            curl_global_cleanup();
            curlInitialized = false;
        }
        downloads.clear();
    }
}
