#include "roblox_services.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <math.h>

#if defined(BUILD_3DS) || defined(_3DS)
#include <3ds.h>
#include <3ds/services/soc.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <malloc.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#ifndef SO_RCVTIMEO
#define SO_RCVTIMEO 1005
#endif
#ifndef SO_SNDTIMEO
#define SO_SNDTIMEO 1006
#endif
#endif // BUILD_3DS

// ========================================================================
// Internal state
// ========================================================================
namespace RobloxServices {

    // HttpService state
    static bool gHttpsEnabled = false;

    // TeamsService state
    static std::vector<Team> gTeams;
    static std::map<std::string, std::string> gPlayerTeams;

    // AssetService state
    static std::map<std::string, AssetInfo> gAssetCache;

    // Lighting state
    static float gClockTime = 14.0f;
    static float gBrightness = 1.0f;
    static uint32_t gAmbientColor = 0xFFFFFFFF;
    static uint32_t gOutdoorAmbient = 0xFFFFFFFF;
    static float gFogEnd = 1000.0f;
    static uint32_t gFogColor = 0xFF80C0FF;
    static bool gGlobalShadows = false;
    static int gTechnology = 0;
    static float gGeographicLatitude = 0.0f;

    // CollectionService state
    static std::map<std::string, std::vector<std::string>> gTagsMap;

    // DataStoreService state
    static const std::string gDataStorePath = "sdmc:/OpenBLOX/datastores/";

    // LocalizationService state
    static std::string gCurrentLocale = "en";

    // Statistics state
    static std::map<std::string, int> gStatistics;

    // TweenService state
    static std::vector<TweenInstance> gTweens;
    static int gNextTweenId = 1;

    // PhysicsService state
    static std::map<std::string, std::string> gCollisionGroups;
    static std::map<std::string, std::map<std::string, bool>> gCollisionGroupMatrix;

    // ContentProvider state
    static float gPreloadProgress = 0.0f;

    // ========================================================================
    // InsertService state (Pizzaboy Sets API)
    // ========================================================================
    static std::string gInsertAssetUrl = "https://sets.pizzaboxer.xyz";
    static std::string gInsertPlaceUrl = "";
    static std::string gInsertAvatarUrl = "";
    static std::string gInsertModelUrl = "https://sets.pizzaboxer.xyz";
    static std::string gInsertDecalUrl = "";
    static std::string gInsertSoundUrl = "";
    static std::string gInsertScriptUrl = "";
    static std::string gInsertBadgeUrl = "";
    static std::string gInsertAvatarPackageUrl = "";
    static std::string gInsertMonthsOfFreePremiumUrl = "";
    static std::string gInsertClothingPackageUrl = "";
    static std::string gInsertGemUrl = "";
    static std::string gInsertMusicUrl = "";
    static bool gInsertCallbackCalled = false;

    static const std::string gPizzaboySetsBaseUrl = "https://sets.pizzaboxer.xyz";

    // HTTP socket state (shared with socket infrastructure)
    static bool gHttpSocketsInitialized = false;
    static u32* gSocBuffer = NULL;
    static const int SOC_ALIGN_VAL = 0x1000;
    static const int SOC_BUFFERSIZE_VAL = 0x100000; // 1MB

    // Initialize 3DS socket subsystem
    static bool initHttpSockets() {
        if (gHttpSocketsInitialized) return true;
        gSocBuffer = (u32*)memalign(SOC_ALIGN_VAL, SOC_BUFFERSIZE_VAL);
        if (!gSocBuffer) { printf("[HttpService] Failed to allocate SOC buffer\n"); return false; }
        Result rc = socInit(gSocBuffer, SOC_BUFFERSIZE_VAL);
        if (R_FAILED(rc)) {
            printf("[HttpService] socInit failed: 0x%08X\n", (unsigned int)rc);
            free(gSocBuffer); gSocBuffer = NULL; return false;
        }
        gHttpSocketsInitialized = true;
        printf("[HttpService] SOC initialized\n");
        return true;
    }

    // Shutdown socket subsystem
    static void shutdownHttpSockets() {
        if (gSocBuffer) { socExit(); free(gSocBuffer); gSocBuffer = NULL; }
        gHttpSocketsInitialized = false;
    }

    // Parse URL into components
    static bool parseUrl(const std::string& url, std::string& scheme, std::string& host, int& port, std::string& path) {
        size_t protoPos = url.find("://");
        if (protoPos == std::string::npos) { scheme = "http"; }
        else { scheme = url.substr(0, protoPos); }
        size_t hostStart = (protoPos != std::string::npos) ? protoPos + 3 : 0;
        size_t pathPos = url.find('/', hostStart);
        if (pathPos == std::string::npos) { host = url.substr(hostStart); path = "/"; }
        else { host = url.substr(hostStart, pathPos - hostStart); path = url.substr(pathPos); }
        size_t colonPos = host.find(':');
        if (colonPos != std::string::npos) {
            std::string portStr = host.substr(colonPos + 1);
            host = host.substr(0, colonPos);
            port = std::stoi(portStr);
        } else { port = (scheme == "https") ? 443 : 80; }
        return true;
    }

    // Perform HTTP request using 3DS sockets
    static HttpRequestResult performHttpRequest(const std::string& url, HttpMethod method,
                                                  const std::string& body,
                                                  const std::map<std::string, std::string>& headers) {
        HttpRequestResult result;
        result.success = false; result.statusCode = 0;

        if (!initHttpSockets()) { result.errorMsg = "Failed to initialize sockets"; return result; }

        std::string scheme, host, path; int port;
        if (!parseUrl(url, scheme, host, port, path)) { result.errorMsg = "Invalid URL"; return result; }

        // Resolve hostname
        struct hostent* ent = gethostbyname(host.c_str());
        if (!ent) { result.errorMsg = std::string("DNS resolution failed for ") + host; return result; }
        struct in_addr addr;
        memcpy(&addr, ent->h_addr, sizeof(addr));

        // Create TCP socket
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) { result.errorMsg = "socket() failed"; return result; }

        // Set receive timeout (10 seconds)
        struct timeval timeout; timeout.tv_sec = 10; timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr = addr;
        server.sin_port = htons(port);

        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
            result.errorMsg = std::string("connect() failed: ") + strerror(errno);
            close(sock); return result;
        }

        // Build HTTP request
        std::string methodStr = "GET";
        if (method == HttpMethod::POST) methodStr = "POST";
        else if (method == HttpMethod::PUT) methodStr = "PUT";
        else if (method == HttpMethod::DELETE) methodStr = "DELETE";

        std::string request = methodStr + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";
        request += "User-Agent: OpenBLOX/1.0\r\n";
        request += "Accept: */*\r\n";
        request += "Connection: close\r\n";
        if (!body.empty()) {
            request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
            request += "Content-Type: application/json\r\n";
        }
        for (const auto& h : headers) { request += h.first + ": " + h.second + "\r\n"; }
        request += "\r\n";
        if (!body.empty()) request += body;

        if (send(sock, request.c_str(), request.size(), MSG_NOSIGNAL) < 0) {
            result.errorMsg = "send() failed"; close(sock); return result;
        }

        // Receive response
        char buf[4096]; std::string response;
        int bytes;
        while ((bytes = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[bytes] = '\0'; response += buf;
        }
        close(sock);

        if (response.empty()) { result.errorMsg = "Empty response"; return result; }

        // Parse HTTP status code
        size_t statusEnd = response.find("\r\n");
        if (statusEnd != std::string::npos) {
            std::string statusLine = response.substr(0, statusEnd);
            // "HTTP/1.1 XXX"
            size_t codeStart = statusLine.find(' ') + 1;
            size_t codeEnd = statusLine.find(' ', codeStart);
            if (codeEnd != std::string::npos) {
                std::string codeStr = statusLine.substr(codeStart, codeEnd - codeStart);
                result.statusCode = std::stoi(codeStr);
            }
        }

        // Extract body after headers
        size_t bodyStart = response.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            result.body = response.substr(bodyStart + 4);
        }
        result.success = (result.statusCode >= 200 && result.statusCode < 300);
        return result;
    }

    // ========================================================================
    // HttpService
    // ========================================================================

    void initHttpService() {
        gHttpsEnabled = false;
        printf("[RobloxServices] HttpService initialized\n");
    }

    std::string jsonEncode(const std::string& input) {
        std::string result = "\"";
        for (char c : input) {
            switch (c) {
                case '\"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        result += "\"";
        return result;
    }

    std::string jsonDecode(const std::string& input) {
        // Strip surrounding quotes if present
        std::string trimmed = input;
        if (trimmed.length() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
            trimmed = trimmed.substr(1, trimmed.length() - 2);
            std::string result;
            for (size_t i = 0; i < trimmed.length(); i++) {
                if (trimmed[i] == '\\' && i + 1 < trimmed.length()) {
                    i++;
                    switch (trimmed[i]) {
                        case '\"': result += '\"'; break;
                        case '\\': result += '\\'; break;
                        case 'n': result += '\n'; break;
                        case 'r': result += '\r'; break;
                        case 't': result += '\t'; break;
                        default: result += trimmed[i];
                    }
                } else {
                    result += trimmed[i];
                }
            }
            return result;
        }
        return trimmed;
    }

    uint64_t generateGUID(bool withCasing) {
        uint64_t guid = (uint64_t)time(NULL) ^ ((uint64_t)rand() << 32);
        return guid;
    }

    std::string urlEncode(const std::string& input) {
        std::string result;
        for (char c : input) {
            if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
                result += c;
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
                result += buf;
            }
        }
        return result;
    }

    std::string urlDecode(const std::string& input) {
        std::string result;
        for (size_t i = 0; i < input.length(); i++) {
            if (input[i] == '%' && i + 2 < input.length()) {
                char buf[3] = {input[i+1], input[i+2], '\0'};
                result += (char)strtol(buf, nullptr, 16);
                i += 2;
            } else if (input[i] == '+') {
                result += ' ';
            } else {
                result += input[i];
            }
        }
        return result;
    }

    HttpRequestResult httpRequest(const std::string& url, HttpMethod method,
                                   const std::string& body,
                                   const std::map<std::string, std::string>& headers) {
        HttpRequestResult result;
        result.statusCode = 0;
        result.success = false;

        if (!gHttpsEnabled) {
            result.errorMsg = "HTTP requests are not enabled (set HttpService.HttpsEnabled = true)";
            return result;
        }

#if defined(BUILD_3DS) || defined(_3DS)
        // Parse URL into components
        std::string scheme = "http";
        std::string host;
        int port = 80;
        std::string path = "/";

        // Extract scheme
        size_t schemeEnd = url.find("://");
        if (schemeEnd != std::string::npos) {
            scheme = url.substr(0, schemeEnd);
        }
        size_t hostStart = (schemeEnd != std::string::npos) ? schemeEnd + 3 : 0;

        // Extract host and path
        size_t hostEnd = url.find('/', hostStart);
        if (hostEnd != std::string::npos) {
            host = url.substr(hostStart, hostEnd - hostStart);
            path = url.substr(hostEnd);
        } else {
            host = url.substr(hostStart);
        }

        // Set port based on scheme
        if (scheme == "https") port = 443;
        else port = 80;

        // DNS resolution
        struct addrinfo hints, *res = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        char portStr[8];
        snprintf(portStr, sizeof(portStr), "%d", port);

        int ret = getaddrinfo(host.c_str(), portStr, &hints, &res);
        if (ret != 0 || res == NULL) {
            result.errorMsg = "DNS resolution failed: " + host;
            result.statusCode = 0;
            return result;
        }

        // Create socket
        int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        freeaddrinfo(res);
        if (sock < 0) {
            result.errorMsg = "Socket creation failed";
            return result;
        }

        // Set receive timeout (5 seconds)
        time_t timeout = 5;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Connect
        res = NULL;
        ret = getaddrinfo(host.c_str(), portStr, &hints, &res);
        if (ret == 0 && res) {
            ret = connect(sock, res->ai_addr, res->ai_addrlen);
            freeaddrinfo(res);
        }
        if (ret < 0) {
            result.errorMsg = "Connection failed to " + host;
            close(sock);
            return result;
        }

        // Build HTTP request
        std::string methodStr = "GET";
        if (method == HttpMethod::POST) methodStr = "POST";
        else if (method == HttpMethod::PUT) methodStr = "PUT";
        else if (method == HttpMethod::DELETE) methodStr = "DELETE";

        std::string request = methodStr + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";
        request += "User-Agent: OpenBLOX/0.1.0\r\n";
        request += "Accept: */*\r\n";
        request += "Connection: close\r\n";

        // Add custom headers
        if (!headers.empty()) {
            for (auto& h : headers) {
                request += h.first + ": " + h.second + "\r\n";
            }
        }

        // Add content length for POST/PUT
        if (!body.empty()) {
            request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
            request += "Content-Type: application/x-www-form-urlencoded\r\n";
        }
        request += "\r\n";

        // Send request
        const char* reqPtr = request.c_str();
        size_t reqLen = request.size();
        size_t sent = 0;
        while (sent < reqLen) {
            int s = send(sock, reqPtr + sent, reqLen - sent, MSG_NOSIGNAL);
            if (s < 0) {
                result.errorMsg = "Failed to send request";
                close(sock);
                return result;
            }
            sent += s;
        }

        // Send body for POST/PUT
        if (!body.empty()) {
            const char* bodyPtr = body.c_str();
            size_t bodySent = 0;
            while (bodySent < body.size()) {
                int s = send(sock, bodyPtr + bodySent, body.size() - bodySent, MSG_NOSIGNAL);
                if (s < 0) {
                    result.errorMsg = "Failed to send request body";
                    close(sock);
                    return result;
                }
                bodySent += s;
            }
        }

        // Receive response
        const int bufferSize = 8192;
        char buffer[bufferSize];
        std::string responseStr;
        bool headersDone = false;

        while (true) {
            int rc = recv(sock, buffer, bufferSize - 1, 0);
            if (rc <= 0) break;
            buffer[rc] = '\0';
            responseStr += buffer;
        }
        close(sock);

        if (responseStr.empty()) {
            result.errorMsg = "Empty response from server";
            return result;
        }

        // Parse HTTP status line
        size_t firstLine = responseStr.find("\r\n");
        if (firstLine != std::string::npos) {
            std::string statusLine = responseStr.substr(0, firstLine);
            result.statusCode = 0;
            size_t space1 = statusLine.find(' ');
            if (space1 != std::string::npos) {
                size_t space2 = statusLine.find(' ', space1 + 1);
                if (space2 != std::string::npos) {
                    result.statusCode = atoi(statusLine.substr(space1 + 1, space2 - space1 - 1).c_str());
                }
            }
        }

        // Parse headers and extract body
        size_t headerEnd = responseStr.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            result.body = responseStr.substr(headerEnd + 4);
        } else {
            result.body = "";
        }

        result.success = (result.statusCode >= 200 && result.statusCode < 300);
        if (!result.success) {
            result.errorMsg = "HTTP error: " + std::to_string(result.statusCode);
        }
#else
        result.errorMsg = "HTTP request not supported on this platform";
#endif
        return result;
    }

    bool isHttpsEnabled() { return gHttpsEnabled; }
    void setHttpsEnabled(bool enabled) { gHttpsEnabled = enabled; }

    // ========================================================================
    // TeamsService
    // ========================================================================

    void initTeamService() {
        gTeams.clear();
        gPlayerTeams.clear();
        printf("[RobloxServices] TeamsService initialized\n");
    }

    void shutdownTeamService() {
        gTeams.clear();
        gPlayerTeams.clear();
    }

    void addTeam(const std::string& name, uint32_t color) {
        removeTeam(name);
        Team team;
        team.name = name;
        team.color = color;
        gTeams.push_back(team);
        printf("[RobloxServices] Team added: %s\n", name.c_str());
    }

    void removeTeam(const std::string& name) {
        for (auto it = gTeams.begin(); it != gTeams.end(); ++it) {
            if (it->name == name) { gTeams.erase(it); break; }
        }
        for (auto& p : gPlayerTeams) {
            if (p.second == name) p.second = "";
        }
    }

    Team* getTeam(const std::string& name) {
        for (auto& t : gTeams) {
            if (t.name == name) return &t;
        }
        return nullptr;
    }

    std::vector<Team>& getAllTeams() { return gTeams; }

    void setPlayerTeam(const std::string& playerName, const std::string& teamName) {
        gPlayerTeams[playerName] = teamName;
    }

    std::string getPlayerTeam(const std::string& playerName) {
        auto it = gPlayerTeams.find(playerName);
        return (it != gPlayerTeams.end()) ? it->second : "";
    }

    // ========================================================================
    // AssetService
    // ========================================================================

    void initAssetService() {
        gAssetCache.clear();
        printf("[RobloxServices] AssetService initialized\n");
    }

    void shutdownAssetService() { gAssetCache.clear(); }

    AssetInfo getAssetInfo(const std::string& assetId) {
        auto it = gAssetCache.find(assetId);
        if (it != gAssetCache.end()) return it->second;
        AssetInfo info;
        info.assetId = assetId;
        info.contentType = "application/octet-stream";
        info.isPublic = true;
        info.size = 0;
        gAssetCache[assetId] = info;
        return info;
    }

    std::string getAssetThumbnailUrl(const std::string& assetId, int width, int height) {
        char buf[256];
        snprintf(buf, sizeof(buf), "https://thumb.assetdelivery.roblox.com/v1/assets/%s-thumb-%dx%d.webp",
                 assetId.c_str(), width, height);
        return std::string(buf);
    }

    bool downloadAsset(const std::string& assetId, const std::string& outputPath) {
        printf("[RobloxServices] Asset download: %s -> %s\n", assetId.c_str(), outputPath.c_str());
        return false;
    }

    // ========================================================================
    // Lighting
    // ========================================================================

    void initLightingService() {
        gClockTime = 14.0f;
        gBrightness = 1.0f;
        gAmbientColor = 0xFFFFFFFF;
        gFogEnd = 1000.0f;
        gGlobalShadows = false;
        gTechnology = 0;
        printf("[RobloxServices] Lighting initialized\n");
    }

    void setClockTime(float h) { gClockTime = h; }
    float getClockTime() { return gClockTime; }
    void setBrightness(float b) { gBrightness = b; if (gBrightness < 0) gBrightness = 0; if (gBrightness > 1) gBrightness = 1; }
    float getBrightness() { return gBrightness; }
    void setAmbientColor(uint32_t c) { gAmbientColor = c; }
    uint32_t getAmbientColor() { return gAmbientColor; }
    void setOutdoorAmbient(uint32_t c) { gOutdoorAmbient = c; }
    void setFogEnd(float d) { gFogEnd = d; }
    float getFogEnd() { return gFogEnd; }
    void setFogColor(uint32_t c) { gFogColor = c; }
    void setGlobalShadows(bool e) { gGlobalShadows = e; }
    void setTechnology(int t) { gTechnology = t; }
    void setGeographicLatitude(float d) { gGeographicLatitude = d; }
    void setAmbient(uint32_t c) { gAmbientColor = c; }

    // ========================================================================
    // CollectionService
    // ========================================================================

    void initCollectionService() {
        gTagsMap.clear();
        printf("[RobloxServices] CollectionService initialized\n");
    }

    void addTag(const std::string& ref, const std::string& tag) {
        auto& tags = gTagsMap[ref];
        if (std::find(tags.begin(), tags.end(), tag) == tags.end()) tags.push_back(tag);
    }

    void removeTag(const std::string& ref, const std::string& tag) {
        auto it = gTagsMap.find(ref);
        if (it != gTagsMap.end())
            it->second.erase(std::remove(it->second.begin(), it->second.end(), tag), it->second.end());
    }

    bool hasTag(const std::string& ref, const std::string& tag) {
        auto it = gTagsMap.find(ref);
        if (it == gTagsMap.end()) return false;
        return std::find(it->second.begin(), it->second.end(), tag) != it->second.end();
    }

    std::vector<std::string> getTags(const std::string& ref) {
        auto it = gTagsMap.find(ref);
        return (it != gTagsMap.end()) ? it->second : std::vector<std::string>();
    }

    std::vector<std::string> getInstancesByTag(const std::string& tag) {
        std::vector<std::string> result;
        for (auto& p : gTagsMap)
            if (std::find(p.second.begin(), p.second.end(), tag) != p.second.end())
                result.push_back(p.first);
        return result;
    }

    // ========================================================================
    // DataStoreService (file-based)
    // ========================================================================

    void initDataStoreService() {
        mkdir("sdmc:/OpenBLOX/datastores", 0755);
        printf("[RobloxServices] DataStoreService initialized (file-based)\n");
    }

    void shutdownDataStoreService() {}

    DataStoreEntry getAsync(const std::string& storeName, const std::string& key) {
        DataStoreEntry entry;
        entry.key = key;
        entry.exists = false;
        char path[256];
        snprintf(path, sizeof(path), "%s%s_%s.dat", gDataStorePath.c_str(), storeName.c_str(), key.c_str());
        FILE* f = fopen(path, "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (size > 0 && size < 1048576) {
                entry.value.resize(size, '\0');
                fread(&entry.value[0], 1, size, f);
                entry.exists = true;
            }
            fclose(f);
        }
        return entry;
    }

    void setAsync(const std::string& storeName, const std::string& key, const std::string& value) {
        char path[256];
        snprintf(path, sizeof(path), "%s%s_%s.dat", gDataStorePath.c_str(), storeName.c_str(), key.c_str());
        FILE* f = fopen(path, "w");
        if (f) { fwrite(value.c_str(), 1, value.length(), f); fclose(f); }
    }

    void removeAsync(const std::string& storeName, const std::string& key) {
        char path[256];
        snprintf(path, sizeof(path), "%s%s_%s.dat", gDataStorePath.c_str(), storeName.c_str(), key.c_str());
        remove(path);
    }

    std::vector<std::string> getStoreNames() { return {}; }

    // ========================================================================
    // LocalizationService
    // ========================================================================

    void initLocalizationService() {
        gCurrentLocale = "en";
        printf("[RobloxServices] LocalizationService initialized\n");
    }

    void setLocale(const std::string& l) { gCurrentLocale = l; }
    std::string getLocale() { return gCurrentLocale; }
    std::string getStringAsync(const std::string&, const std::string& key) { return key; }

    // ========================================================================
    // Statistics
    // ========================================================================

    void initStatisticsService() {
        gStatistics["NetworkIncomingBytes"] = 0;
        gStatistics["NetworkOutgoingBytes"] = 0;
        gStatistics["MemoryUsage"] = 0;
        gStatistics["ServerTickRate"] = 30;
        printf("[RobloxServices] Statistics initialized\n");
    }

    void setStat(const std::string& name, int value) { gStatistics[name] = value; }
    int getStat(const std::string& name) {
        auto it = gStatistics.find(name);
        return (it != gStatistics.end()) ? it->second : 0;
    }

    // ========================================================================
    // TweenService
    // ========================================================================

    static float applyEasing(float t, int style, int direction) {
        float eased = t; // Linear default
        if (style == 1) eased = 1.0f - cosf(t * 3.14159f / 2.0f); // Sine
        else if (style == 2) eased = t * t; // Quad

        if (direction == 2) return 1.0f - eased; // Out
        return eased;
    }

    void initTweenService() {
        gTweens.clear();
        gNextTweenId = 1;
        printf("[RobloxServices] TweenService initialized\n");
    }

    int createTween(const std::string& targetRef, const TweenInfo& tweenInfo,
                    const std::map<std::string, std::string>& goalProperties) {
        TweenInstance tween;
        tween.id = gNextTweenId++;
        tween.targetRef = targetRef;
        tween.info = tweenInfo;
        tween.goalProperties = goalProperties;
        tween.elapsed = 0.0f;
        tween.playing = false;
        tween.completed = false;
        gTweens.push_back(tween);
        return tween.id;
    }

    void playTween(int tweenId) {
        for (auto& t : gTweens)
            if (t.id == tweenId) { t.playing = true; t.completed = false; t.elapsed = 0.0f; break; }
    }
    void cancelTween(int tweenId) {
        for (auto& t : gTweens)
            if (t.id == tweenId) { t.playing = false; t.completed = true; break; }
    }
    void pauseTween(int tweenId) {
        for (auto& t : gTweens)
            if (t.id == tweenId) { t.playing = false; break; }
    }
    void resumeTween(int tweenId) {
        for (auto& t : gTweens)
            if (t.id == tweenId) { t.playing = true; break; }
    }

    void updateTweens(float dt) {
        for (auto& tween : gTweens) {
            if (!tween.playing || tween.completed) continue;
            tween.elapsed += dt;
            float progress = tween.elapsed / tween.info.time;
            if (progress >= 1.0f) { progress = 1.0f; tween.completed = true; tween.playing = false; }
            applyEasing(progress, tween.info.easingStyle, tween.info.easingDirection);
        }
    }

    // ========================================================================
    // PhysicsService
    // ========================================================================

    void initPhysicsService() {
        gCollisionGroups.clear();
        gCollisionGroupMatrix.clear();
        printf("[RobloxServices] PhysicsService initialized\n");
    }

    void setCollisionGroup(const std::string& ref, const std::string& group) {
        gCollisionGroups[ref] = group;
    }

    void collisionGroupCollides(const std::string& g1, const std::string& g2, bool collides) {
        gCollisionGroupMatrix[g1][g2] = collides;
        gCollisionGroupMatrix[g2][g1] = collides;
    }

    // ========================================================================
    // ContentProvider
    // ========================================================================

    void initContentProvider() {
        gPreloadProgress = 0.0f;
        printf("[RobloxServices] ContentProvider initialized\n");
    }

    void preAsync(const std::vector<std::string>& assetIds) {
        printf("[RobloxServices] ContentProvider::PreAsync: %d assets\n", (int)assetIds.size());
    }
    float getPreloadProgress() { return gPreloadProgress; }

    // ========================================================================
    // MarketPlaceService (stub)
    // ========================================================================

    void initMarketPlaceService() {
        printf("[RobloxServices] MarketPlaceService initialized (stub)\n");
    }

    // ========================================================================
    // InsertService - Load sets/models from Pizzaboy Sets API
    // Download endpoint: https://sets.pizzaboxer.xyz/assetversions/{versionId}/download
    // ========================================================================

    // Pizzaboy Sets API endpoints
    static std::string gPizzaboyAssetUrl = "https://sets.pizzaboxer.xyz/v1/asset/";
    static std::string gPizzaboyPlaceUrl = "https://sets.pizzaboxer.xyz/v1/place/";
    static std::string gPizzaboyAvatarUrl = "https://sets.pizzaboxer.xyz/v1/avatar/";
    static std::string gPizzaboyModelUrl = "https://sets.pizzaboxer.xyz/v1/model/";
    static std::string gPizzaboyDecalUrl = "https://sets.pizzaboxer.xyz/v1/asset/";
    static std::string gPizzaboySoundUrl = "https://sets.pizzaboxer.xyz/v1/asset/";
    static std::string gPizzaboyScriptUrl = "https://sets.pizzaboxer.xyz/v1/script/";
    static std::string gPizzaboyBadgeUrl = "https://sets.pizzaboxer.xyz/v1/badge/";
    static std::string gPizzaboyAvatarPackageUrl = "https://sets.pizzaboxer.xyz/v1/avatar/";
    static std::string gPizzaboyMonthsOfFreePremiumUrl = "https://sets.pizzaboxer.xyz/v1/pas/";
    static std::string gPizzaboyClothingPackageUrl = "https://sets.pizzaboxer.xyz/v1/clothing/";
    static std::string gPizzaboyGemUrl = "https://sets.pizzaboxer.xyz/v1/pas/";
    static std::string gPizzaboyMusicUrl = "https://sets.pizzaboxer.xyz/v1/asset/";
 

    void initInsertService() {
        printf("[InsertService] Initialized (Pizzaboy Sets API)\n");
    }

    void shutdownInsertService() {
        printf("[InsertService] Shut down\n");
    }

    // Configure Pizzaboy Sets API endpoints
    void SetAssetUrl(const char* url) { gInsertAssetUrl = url ? url : gInsertAssetUrl; }
    void SetPlaceUrl(const char* url) { gInsertPlaceUrl = url ? url : gInsertPlaceUrl; }
    void SetAvatarUrl(const char* url) { gInsertAvatarUrl = url ? url : gInsertAvatarUrl; }
    void SetModelUrl(const char* url) { gInsertModelUrl = url ? url : gInsertModelUrl; }
    void SetDecalUrl(const char* url) { gInsertDecalUrl = url ? url : gInsertDecalUrl; }
    void SetSoundUrl(const char* url) { gInsertSoundUrl = url ? url : gInsertSoundUrl; }
    void SetScriptUrl(const char* url) { gInsertScriptUrl = url ? url : gInsertScriptUrl; }
    void SetBadgeUrl(const char* url) { gInsertBadgeUrl = url ? url : gInsertBadgeUrl; }
    void SetAvatarPackageUrl(const char* url) { gInsertAvatarPackageUrl = url ? url : gInsertAvatarPackageUrl; }
    void SetMonthsOfFreePremiumUrl(const char* url) { gInsertMonthsOfFreePremiumUrl = url ? url : gInsertMonthsOfFreePremiumUrl; }
    void SetClothingPackageUrl(const char* url) { gInsertClothingPackageUrl = url ? url : gInsertClothingPackageUrl; }
    void SetGemUrl(const char* url) { gInsertGemUrl = url ? url : gInsertGemUrl; }
    void SetMusicUrl(const char* url) { gInsertMusicUrl = url ? url : gInsertMusicUrl; }
    void setInsertCallback(const std::function<void(const std::string&)>&) {
        gInsertCallbackCalled = true;
    }

    // Build Pizzaboy Sets download URL: /assetversions/{versionId}/download
    static std::string buildDownloadUrl(const std::string& versionId) {
        return "https://sets.pizzaboxer.xyz/assetversions/" + versionId + "/download";
    }

    // Build collection list URL
    static std::string buildCollectionUrl(const std::string& collectionId) {
        return "https://sets.pizzaboxer.xyz/v1/collection/" + collectionId;
    }

    // Load a model from Pizzaboy Sets API
    std::string LoadAsset(const std::string& assetId, int assetType) {
        printf("[InsertService] LoadAsset: %s (type=%d)\n", assetId.c_str(), assetType);

        std::string url;
        // For models/sets, use the Pizzaboy Sets download endpoint
        if (assetType == 3 || assetType == 0 || assetType == 2) {
            url = buildDownloadUrl(assetId);
        }
        // For places, return empty (not supported)
        else if (assetType == 1) {
            printf("[InsertService] Place downloading is not supported\n");
            return "";
        }
        // Default: try the download endpoint
        else {
            url = buildDownloadUrl(assetId);
        }

        HttpRequestResult result = httpRequest(url, HttpMethod::GET);
        if (!result.success) {
            printf("[InsertService] LoadAsset failed: %s\n", result.errorMsg.c_str());
            return "";
        }
        printf("[InsertService] Downloaded %zu bytes from %s\n", result.body.size(), url.c_str());
        return result.body;
    }

    // Load a local asset file (3DS filesystem)
    std::string LoadLocalAsset(const std::string& assetId) {
        std::string localPath = std::string("sdmc:/") + assetId;
        FILE* f = fopen(localPath.c_str(), "rb");
        if (!f) {
            printf("[InsertService] LoadLocalAsset: file not found: %s\n", localPath.c_str());
            return "";
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::string data(size, '\0');
        if (fread(&data[0], 1, size, f) != (size_t)size) {
            fclose(f);
            return "";
        }
        fclose(f);
        printf("[InsertService] Loaded %ld bytes from %s\n", size, localPath.c_str());
        return data;
    }

    // Get collection info from Sets API
    std::string GetCollection(const std::string& collectionId) {
        std::string url = buildCollectionUrl(collectionId);
        HttpRequestResult result = httpRequest(url, HttpMethod::GET);
        if (!result.success) {
            printf("[InsertService] GetCollection failed: %s\n", result.errorMsg.c_str());
            return "";
        }
        printf("[InsertService] Collection %s: %zu bytes\n", collectionId.c_str(), result.body.size());
        return result.body;
    }

    std::string LoadModel(const std::string& modelId) {
        return LoadAsset(modelId, 3);  // Model type
    }

    std::string LoadPlaceFile(const std::string& placeId) {
        printf("[InsertService] Place downloading is not supported\n");
        return "";
    }

    std::string LoadDecal(const std::string& decalId) {
        return LoadAsset(decalId, 4);  // Decal type
    }

    std::string LoadSound(const std::string& soundId) {
        return LoadAsset(soundId, 5);  // Sound type
    }

    std::string LoadAvatar(const std::string& avatarId) {
        return LoadAsset(avatarId, 2);  // Avatar type
    }

    std::string LoadBadge(const std::string& badgeId) {
        return LoadAsset(badgeId, 7);  // Badge type
    }

    std::string LoadClothingPackage(const std::string& packageId) {
        return LoadAsset(packageId, 8);  // Clothing type
    }

    // Replicated/Callback variants
    std::string LoadBadgeToPlayerReplicated(const std::string& badgeId,
                                             const std::string& callback) {
        return LoadBadge(badgeId);
    }

    std::string LoadCollectionToBackpackReplicated(const std::string& collectionId,
                                                    const std::string& callback) {
        return GetCollection(collectionId);
    }

    std::string LoadPlayerCreatedAsset(const std::string& assetId, const std::string& callback) {
        return LoadAsset(assetId, 0);
    }

    std::string LoadBadgeToPlayerReplicatedString(const std::string& badgeId,
                                                   const std::string& callback) {
        return LoadBadgeToPlayerReplicated(badgeId, callback);
    }

    std::string LoadCollectionToBackpackReplicatedString(const std::string& collectionId,
                                                          const std::string& callback) {
        return LoadCollectionToBackpackReplicated(collectionId, callback);
    }

    std::string LoadPlayerCreatedAssetString(const std::string& assetId,
                                              const std::string& callback) {
        return LoadPlayerCreatedAsset(assetId, callback);
    }

    // ========================================================================
    // Starter / Run / Input / Workspace / Studio / Sound / Players
    // ========================================================================

    void initStarterServices() { printf("[RobloxServices] Starter services initialized\n"); }
    void initRunService() { printf("[RobloxServices] RunService initialized\n"); }
    void setRenderSteppedCallback(const std::function<void(float)>&) {}
    void setHeartbeatCallback(const std::function<void(float)>&) {}
    void initInputServices() { printf("[RobloxServices] Input services initialized\n"); }
    void initWorkspaceServices() { printf("[RobloxServices] Workspace services initialized\n"); }
    void initStudioService() { printf("[RobloxServices] StudioService initialized\n"); }
    void initSoundService() { printf("[RobloxServices] SoundService initialized\n"); }
    void initPlayersService() { printf("[RobloxServices] Players service initialized\n"); }

    // ========================================================================
    // Init/Shutdown all
    // ========================================================================

    void initAll() {
        initHttpService();
        initTeamService();
        initAssetService();
        initLightingService();
        initCollectionService();
        initDataStoreService();
        initLocalizationService();
        initStatisticsService();
        initTweenService();
        initPhysicsService();
        initContentProvider();
        initMarketPlaceService();
        initInsertService();
        initStarterServices();
        initRunService();
        initInputServices();
        initWorkspaceServices();
        initStudioService();
        initSoundService();
        initPlayersService();
        printf("[RobloxServices] ALL services initialized\n");
    }

    void shutdownAll() {
        shutdownTeamService();
        shutdownAssetService();
        shutdownInsertService();
        shutdownDataStoreService();
        printf("[RobloxServices] ALL services shut down\n");
    }

} // namespace RobloxServices

// ========================================================================
// Lua bindings
// ========================================================================
#if defined(HAVE_LUA)
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace RobloxServices {

static int luaJsonEncode(lua_State* L) {
    const char* input = luaL_checkstring(L, 1);
    lua_pushstring(L, jsonEncode(input).c_str());
    return 1;
}

static int luaJsonDecode(lua_State* L) {
    const char* input = luaL_checkstring(L, 1);
    lua_pushstring(L, jsonDecode(input).c_str());
    return 1;
}

static int luaGenerateGUID(lua_State* L) {
    bool withCasing = lua_toboolean(L, 1);
    lua_pushnumber(L, (lua_Number)generateGUID(withCasing));
    return 1;
}

static int luaUrlEncode(lua_State* L) {
    const char* input = luaL_checkstring(L, 1);
    lua_pushstring(L, urlEncode(input).c_str());
    return 1;
}

static int luaUrlDecode(lua_State* L) {
    const char* input = luaL_checkstring(L, 1);
    lua_pushstring(L, urlDecode(input).c_str());
    return 1;
}

static int luaHttpRequestAsync(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    const char* methodStr = luaL_optstring(L, 2, "GET");
    HttpMethod method = HttpMethod::GET;
    if (strcmp(methodStr, "POST") == 0) method = HttpMethod::POST;
    else if (strcmp(methodStr, "PUT") == 0) method = HttpMethod::PUT;
    else if (strcmp(methodStr, "DELETE") == 0) method = HttpMethod::DELETE;
    HttpRequestResult result = httpRequest(url, method);
    if (result.success) lua_pushstring(L, result.body.c_str());
    else { lua_pushnil(L); lua_pushstring(L, result.errorMsg.c_str()); }
    return 2;
}

// ========================================================================
// InsertService Lua bindings
// ========================================================================

static int luaInsertLoadAsset(lua_State* L) {
    const char* assetId = luaL_checkstring(L, 1);
    int assetType = luaL_optinteger(L, 2, 0);
    std::string result = LoadAsset(assetId, assetType);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

static int luaInsertLoadPlaceFile(lua_State* L) {
    const char* placeId = luaL_checkstring(L, 1);
    std::string result = LoadPlaceFile(placeId);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

static int luaInsertLoadModel(lua_State* L) {
    const char* modelId = luaL_checkstring(L, 1);
    std::string result = LoadModel(modelId);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

static int luaInsertLoadDecal(lua_State* L) {
    const char* decalId = luaL_checkstring(L, 1);
    std::string result = LoadDecal(decalId);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

static int luaInsertLoadSound(lua_State* L) {
    const char* soundId = luaL_checkstring(L, 1);
    std::string result = LoadSound(soundId);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

static int luaInsertLoadAvatar(lua_State* L) {
    const char* avatarId = luaL_checkstring(L, 1);
    std::string result = LoadAvatar(avatarId);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

static int luaInsertLoadBadge(lua_State* L) {
    const char* badgeId = luaL_checkstring(L, 1);
    std::string result = LoadBadge(badgeId);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

static int luaInsertLoadClothingPackage(lua_State* L) {
    const char* packageId = luaL_checkstring(L, 1);
    std::string result = LoadClothingPackage(packageId);
    if (!result.empty()) lua_pushstring(L, result.c_str());
    else lua_pushnil(L);
    return 1;
}

void registerAllServices(lua_State* L) {
    // HttpService
    lua_newtable(L);
    lua_pushcfunction(L, &luaJsonEncode);
    lua_setfield(L, -2, "JSONEncode");
    lua_pushcfunction(L, &luaJsonDecode);
    lua_setfield(L, -2, "JSONDecode");
    lua_pushcfunction(L, &luaGenerateGUID);
    lua_setfield(L, -2, "GenerateGUID");
    lua_pushcfunction(L, &luaUrlEncode);
    lua_setfield(L, -2, "UrlEncode");
    lua_pushcfunction(L, &luaUrlDecode);
    lua_setfield(L, -2, "UrlDecode");
    lua_pushcfunction(L, &luaHttpRequestAsync);
    lua_setfield(L, -2, "RequestAsync");
    // InsertService
    lua_newtable(L);
    lua_newtable(L);  // metatable with __index
    lua_pushcfunction(L, &luaInsertLoadAsset); lua_setfield(L, -2, "LoadAsset");
    lua_pushcfunction(L, &luaInsertLoadPlaceFile); lua_setfield(L, -2, "LoadPlaceFile");
    lua_pushcfunction(L, &luaInsertLoadModel); lua_setfield(L, -2, "LoadModel");
    lua_pushcfunction(L, &luaInsertLoadDecal); lua_setfield(L, -2, "LoadDecal");
    lua_pushcfunction(L, &luaInsertLoadSound); lua_setfield(L, -2, "LoadSound");
    lua_pushcfunction(L, &luaInsertLoadAvatar); lua_setfield(L, -2, "LoadAvatar");
    lua_pushcfunction(L, &luaInsertLoadBadge); lua_setfield(L, -2, "LoadBadge");
    lua_pushcfunction(L, &luaInsertLoadClothingPackage); lua_setfield(L, -2, "LoadClothingPackage");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "InsertService");

    lua_setglobal(L, "HttpService");
}

} // namespace RobloxServices
#else
namespace RobloxServices {
void registerAllServices(lua_State*) {}
}
#endif