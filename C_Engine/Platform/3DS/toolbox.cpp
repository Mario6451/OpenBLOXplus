// OpenBLOX Studio Toolbox Implementation
// Fetches assets from the configured Toolbox Proxy

#include "toolbox.h"
#include "settings.h"
#include "asset_loader.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>

namespace Toolbox {

    static std::string proxyUrl;
    static std::vector<ToolboxAsset> searchResults;
    static int scrollOffset = 0;
    static std::string searchQuery;
    static AssetType currentCategory = AssetType::Model;
    static bool searching = false;

    // Forward declarations for static functions used before their definitions
    static std::string urlEncode(const std::string& s);
    static bool httpGet(const std::string& url, std::string& response);
    static void parseSearchResults(const std::string& json);

    // Stub socket functions - 3DS uses httpc module, not POSIX sockets
    static int socketConnect(const char* host, int port) {
        (void)host; (void)port;
        printf("Toolbox: socketConnect not available on 3DS\n");
        return -1;
    }
    static int socketSend(int sock, const char* data, size_t len) {
        (void)sock; (void)data; (void)len;
        return -1;
    }
    static int socketRecv(int sock, char* buffer, int len) {
        (void)sock; (void)buffer; (void)len;
        return -1;
    }
    static void socketClose(int sock) {
        (void)sock;
    }

    void init() {
        // Load proxy URL from settings
        proxyUrl = Settings::get().general.toolboxProxy;
        scrollOffset = 0;
        searchQuery = "";
        currentCategory = AssetType::Model;
        searching = false;
        printf("Toolbox initialized with proxy: %s\n", proxyUrl.c_str());
    }

    void setProxyUrl(const std::string& url) {
        proxyUrl = url;
        Settings::get().general.toolboxProxy = url;
        Settings::save();
    }

    const std::string& getProxyUrl() {
        return proxyUrl;
    }

    bool searchAssets(const std::string& query, AssetType type) {
        if (proxyUrl.empty()) {
            printf("Toolbox: No proxy URL configured\n");
            return false;
        }

        searching = true;
        searchQuery = query;
        currentCategory = type;
        scrollOffset = 0;
        searchResults.clear();

        // Build proxy search URL
        // Format: <proxy_url>/search?type=<type>&query=<query>
        std::string url = proxyUrl + "/search?type=" + std::string(getAssetTypeString(type));
        url += "&query=" + urlEncode(query);

        printf("Toolbox: Searching proxy for '%s' (type: %s)\n", query.c_str(), getAssetTypeString(type));

        // Use AssetLoader to fetch search results
        std::string response;
        bool success = httpGet(url, response);

        if (!success) {
            printf("Toolbox: Failed to fetch search results from proxy\n");
            searching = false;
            return false;
        }

        // Parse JSON response - simplified parser for [ { "name":"...", "id":"...", ...} ]
        parseSearchResults(response);

        searching = false;
        printf("Toolbox: Found %d results\n", searchResults.size());
        return true;
    }

    // Simple HTTP GET (currently disabled on 3DS - socket stubs return -1)
    static bool httpGet(const std::string& url_in, std::string& response) {
        // Parse URL
        std::string host, path;
        int port = 80;

        // Create mutable copy of url for parsing
        std::string url = url_in;

        // Remove "http://" prefix
        size_t schemeEnd = url.find("://");
        if (schemeEnd != std::string::npos) {
            url = url.substr(schemeEnd + 3);
        }

        // Extract host
        size_t pathStart = url.find('/');
        if (pathStart != std::string::npos) {
            host = url.substr(0, pathStart);
            path = url.substr(pathStart);
        } else {
            host = url;
            path = "/";
        }

        // Build HTTP request
        std::string request = "GET " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";
        request += "User-Agent: OpenBLOX-Toolbox/0.1.0\r\n";
        request += "Accept: application/json\r\n";
        response += "Connection: close\r\n\r\n";

        // Connect to proxy
        int sock = socketConnect(host.c_str(), port);
        if (sock < 0) return false;

        // Send request
        if (socketSend(sock, request.c_str(), request.length()) < 0) {
            socketClose(sock);
            return false;
        }

        // Receive response
        bool headersDone = false;
        const int bufferSize = 4096;
        char buffer[bufferSize];

        while (true) {
            int ret = socketRecv(sock, buffer, bufferSize - 1);
            if (ret <= 0) break;
            buffer[ret] = '\0';

            if (!headersDone) {
                size_t headerEnd = strstr(buffer, "\r\n\r\n") - buffer;
                if (headerEnd != 0 && headerEnd < (size_t)ret) {
                    headersDone = true;
                    response += (buffer + headerEnd + 4);
                }
            } else {
                response += buffer;
            }
        }

        socketClose(sock);
        return !response.empty();
    }

    // Simple URL encoding
    static std::string urlEncode(const std::string& s) {
        std::string encoded;
        for (char c : s) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else {
                char hex[4];
                snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                encoded += hex;
            }
        }
        return encoded;
    }

    // Simplified JSON parser for search results
    // Expects: [{"name":"...","id":"...","creator":"...","downloads":123}, ...]
    static void parseSearchResults(const std::string& json) {
        size_t pos = 0;
        while ((pos = json.find("{", pos)) != std::string::npos) {
            size_t end = json.find("}", pos);
            if (end == std::string::npos) break;

            std::string obj = json.substr(pos, end - pos + 1);
            ToolboxAsset asset;
            asset.type = currentCategory;
            asset.isFavorite = false;
            asset.downloads = 0;

            // Parse name
            size_t namePos = obj.find("\"name\"");
            if (namePos != std::string::npos) {
                size_t valStart = obj.find('"', namePos + 8);
                if (valStart != std::string::npos) {
                    size_t valEnd = obj.find('"', valStart + 1);
                    if (valEnd != std::string::npos) {
                        asset.name = obj.substr(valStart + 1, valEnd - valStart - 1);
                    }
                }
            }

            // Parse id
            size_t idPos = obj.find("\"id\"");
            if (idPos != std::string::npos) {
                size_t valStart = obj.find('"', idPos + 6);
                if (valStart != std::string::npos) {
                    size_t valEnd = obj.find('"', valStart + 1);
                    if (valEnd != std::string::npos) {
                        asset.assetId = obj.substr(valStart + 1, valEnd - valStart - 1);
                    }
                }
            }

            // Parse creator
            size_t creatorPos = obj.find("\"creator\"");
            if (creatorPos != std::string::npos) {
                size_t valStart = obj.find('"', creatorPos + 11);
                if (valStart != std::string::npos) {
                    size_t valEnd = obj.find('"', valStart + 1);
                    if (valEnd != std::string::npos) {
                        asset.creator = obj.substr(valStart + 1, valEnd - valStart - 1);
                    }
                }
            }

            // Parse downloads
            size_t downloadsPos = obj.find("\"downloads\"");
            if (downloadsPos != std::string::npos) {
                size_t valStart = obj.find(':', downloadsPos);
                if (valStart != std::string::npos) {
                    valStart++;
                    while (valStart < obj.size() && obj[valStart] == ' ') valStart++;
                    std::string numStr;
                    while (valStart < obj.size() && isdigit(obj[valStart])) {
                        numStr += obj[valStart++];
                    }
                    if (!numStr.empty()) asset.downloads = atoi(numStr.c_str());
                }
            }

            if (!asset.name.empty()) {
                searchResults.push_back(asset);
            }

            pos = end + 1;
        }
    }

    const std::vector<ToolboxAsset>& getSearchResults() {
        return searchResults;
    }

    int getScrollOffset() {
        return scrollOffset;
    }

    void setScrollOffset(int offset) {
        scrollOffset = offset;
    }

    bool fetchAsset(const std::string& assetId, AssetType type) {
        if (proxyUrl.empty()) {
            printf("Toolbox: No proxy URL configured\n");
            return false;
        }

        // Build asset fetch URL: <proxy_url>/asset?id=<id>&type=<type>
        std::string url = proxyUrl + "/asset?id=" + assetId;
        url += "&type=" + std::string(getAssetTypeString(type));

        printf("Toolbox: Fetching asset %s from proxy\n", assetId.c_str());

        // Use AssetLoader to download the asset
        return AssetLoader::startDownload(assetId, 
            (AssetLoader::AssetType)(int)type, url);
    }

    bool insertAsset(const std::string& assetId, AssetType type) {
        // Fetch and insert the asset into the current workspace
        bool fetched = fetchAsset(assetId, type);
        if (!fetched) {
            printf("Toolbox: Failed to fetch asset %s\n", assetId.c_str());
            return false;
        }

        // Once downloaded, the asset is automatically available in the workspace
        // via the AssetLoader's local path
        std::string localPath = AssetLoader::getAssetLocalPath(assetId, 
            (AssetLoader::AssetType)(int)type);

        printf("Toolbox: Asset %s available at %s\n", assetId.c_str(), localPath.c_str());
        return true;
    }

    const std::string& getSearchQuery() {
        return searchQuery;
    }

    void setSearchQuery(const std::string& query) {
        searchQuery = query;
    }

    AssetType getCurrentCategory() {
        return currentCategory;
    }

    void setCurrentCategory(AssetType type) {
        currentCategory = type;
    }

    bool isSearching() {
        return searching;
    }

    const char* getAssetTypeString(AssetType type) {
        switch (type) {
            case AssetType::Model: return "model";
            case AssetType::Mesh: return "mesh";
            case AssetType::Decal: return "decal";
            case AssetType::Audio: return "audio";
            case AssetType::Plugin: return "plugin";
            default: return "unknown";
        }
    }

    const char* getCategoryLabel(AssetType type) {
        switch (type) {
            case AssetType::Model: return "Models";
            case AssetType::Mesh: return "Meshes";
            case AssetType::Decal: return "Decals";
            case AssetType::Audio: return "Audio";
            case AssetType::Plugin: return "Plugins";
            default: return "Unknown";
        }
    }

    void shutdown() {
        searchResults.clear();
        proxyUrl.clear();
        searchQuery.clear();
    }

};