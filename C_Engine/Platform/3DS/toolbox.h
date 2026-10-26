#pragma once

#include <string>
#include <vector>
#include <map>
#include <3ds.h>

// OpenBLOX Studio Toolbox
// Fetches assets from the configured Toolbox Proxy

namespace Toolbox {

    enum class AssetType {
        Model,
        Mesh,
        Decal,
        Audio,
        Plugin,
        Count
    };

    struct ToolboxAsset {
        std::string name;
        std::string assetId;
        std::string proxyId;
        std::string creator;
        int downloads;
        bool isFavorite;
        AssetType type;
    };

    // === Core API ===
    void init();
    void setProxyUrl(const std::string& url);
    const std::string& getProxyUrl();
    bool searchAssets(const std::string& query, AssetType type);
    const std::vector<ToolboxAsset>& getSearchResults();
    int getScrollOffset();
    void setScrollOffset(int offset);
    bool fetchAsset(const std::string& assetId, AssetType type);
    bool insertAsset(const std::string& assetId, AssetType type);
    const std::string& getSearchQuery();
    void setSearchQuery(const std::string& query);
    AssetType getCurrentCategory();
    void setCurrentCategory(AssetType type);
    bool isSearching();
    const char* getAssetTypeString(AssetType type);
    const char* getCategoryLabel(AssetType type);
    void shutdown();

};