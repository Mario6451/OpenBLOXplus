#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <3ds.h>

// Content System for 3DS OpenBLOX
// Manages shareddata at SD card root, platformcontent 2012/2015/2016, assetpacks

namespace ContentSystem {

    // Content type
    enum class ContentType {
        Texture,
        Sound,
        Mesh,
        Font,
        Sky,
        Music,
        Script,
        Particle,
        Model,
        Unknown
    };

    // Content source priority
    enum class ContentSource {
        PlatformContent2022,  // platformcontent2022/
        PlatformContent2019,  // platformcontent2019/
        PlatformContent2016,  // platformcontent2016/
        PlatformContent2015,  // platformcontent2015/
        PlatformContent2012,  // platformcontent2012/
        SharedData,           // shareddata/
        AssetPacks,           // shareddata/assetpacks/
        ContentFolder,        // content2007/ (built-in)
        Unknown
    };

    // Content pack info
    struct ContentPack {
        std::string id;
        std::string name;
        std::string version;
        std::string description;
        bool isEnabled;
        u64 sizeBytes;
        std::string path;
    };

    // Content entry
    struct ContentEntry {
        std::string assetId;
        std::string fileName;
        ContentType type;
        ContentSource source;
        std::string fullPath;
        std::string relativePath;
        u64 fileSize;
        bool exists;
    };

    // Initialize content system
    void init();

    // Set shareddata root path
    void setShareddataRoot(const std::string& path);

    // Get shareddata root
    const std::string& getShareddataRoot();

    // Get platformcontent path for an era
    std::string getPlatformContentPath(int era);

    // Scan for available content packs
    void scanContentPacks();

    // Get all content packs
    std::vector<ContentPack>& getContentPacks();

    // Enable/disable a content pack
    void setContentPackEnabled(const std::string& packId, bool enabled);

    // Check if a content pack is enabled
    bool isContentPackEnabled(const std::string& packId);

    // Resolve asset path using priority chain
    // Returns the best available path for an asset
    std::string resolveAsset(const std::string& assetId, ContentType type, ContentSource preferredSource = ContentSource::Unknown);

    // Get all paths for an asset (from all sources)
    std::vector<ContentEntry> getAllAssetPaths(const std::string& assetId, ContentType type);

    // Check if asset exists in any source
    bool assetExists(const std::string& assetId, ContentType type);

    // Get asset data from resolved path
    bool getAssetData(const std::string& assetId, ContentType type, std::vector<u8>& data);

    // Register custom content source
    void addContentSource(const std::string& name, const std::string& path, int priority);

    // Get content source by name
    ContentEntry* getContentSource(const std::string& name);

    // Get content source priority
    int getSourcePriority(ContentSource source);

    // Scan a directory for content files
    void scanDirectory(const std::string& path, ContentSource source, std::map<std::string, ContentEntry>& index);

    // Get indexed content entry
    const ContentEntry* getIndexEntry(const std::string& assetId, ContentType type);

    // Build full content index
    void buildContentIndex();

    // Get index size
    int getIndexSize();

    // Get index statistics
    struct IndexStats {
        int totalEntries;
        int textureCount;
        int soundCount;
        int meshCount;
        int fontCount;
        int modelCount;
        int scriptCount;
        int particleCount;
        int skyCount;
        int musicCount;
        int unknownCount;
        u64 totalSize;
    };
    IndexStats getIndexStats();

    // Shutdown
    void shutdown();
}