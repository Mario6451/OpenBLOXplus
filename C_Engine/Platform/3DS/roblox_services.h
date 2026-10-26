#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdint.h>

// Vast Roblox Services layer for OpenBLOX 3DS
// Provides game:GetService() access to all major Roblox services.
// Accessible via: game:GetService("HttpService"), game:GetService("TeamsService"), etc.

typedef struct lua_State lua_State;

namespace RobloxServices {

    // HTTP request methods
    enum class HttpMethod {
        GET,
        POST,
        PUT,
        DELETE,
        HEAD
    };

    // HTTP request result
    struct HttpRequestResult {
        int statusCode;
        std::string body;
        std::map<std::string, std::string> headers;
        bool success;
        std::string errorMsg;
    };

    // Team definition (TeamsService)
    struct Team {
        std::string name;
        uint32_t color;      // RGBA color
        std::string shirt;   // Shirt asset ID
        std::string pants;   // Pants asset ID
        std::string spawnLocation;
    };

    // Player data store entry (DataStoreService)
    struct DataStoreEntry {
        std::string key;
        std::string value;   // JSON-encoded value
        uint64_t version;
        bool exists;
    };

    // Asset metadata (AssetService)
    struct AssetInfo {
        std::string assetId;
        std::string contentType;
        std::string location;
        bool isPublic;
        int64_t size;
    };

    // Tween definition (TweenService)
    struct TweenInfo {
        float time;              // Duration in seconds
        int easingStyle;         // 0=Linear, 1=Sine, 2=Quad, 3=Cubic, 4=Quart, 5=Quint, 6=Expo, 7=Back, 8=ELastic, 9=Bounce
        int easingDirection;     // 0=InOut, 1=In, 2=Out
        float repeatCount;       // -1 = infinite
        bool reverse;
        float delayTime;
    };

    // Active tween instance
    struct TweenInstance {
        int id;
        std::string targetRef;   // Reference to target Instance
        TweenInfo info;
        std::map<std::string, std::string> startProperties;
        std::map<std::string, std::string> goalProperties;
        float elapsed;
        bool playing;
        bool completed;
    };

    // ========================================================================
    // HttpService
    // ========================================================================
    void initHttpService();
    std::string jsonEncode(const std::string& input);
    std::string jsonDecode(const std::string& input);
    uint64_t generateGUID(bool withCasing = false);
    std::string urlEncode(const std::string& input);
    std::string urlDecode(const std::string& input);
    HttpRequestResult httpRequest(const std::string& url, HttpMethod method,
                                   const std::string& body = "",
                                   const std::map<std::string, std::string>& headers = {});
    bool isHttpsEnabled();
    void setHttpsEnabled(bool enabled);

    // ========================================================================
    // TeamsService
    // ========================================================================
    void initTeamService();
    void shutdownTeamService();
    void addTeam(const std::string& name, uint32_t color = 0xFF0000FF);
    void removeTeam(const std::string& name);
    Team* getTeam(const std::string& name);
    std::vector<Team>& getAllTeams();
    void setPlayerTeam(const std::string& playerName, const std::string& teamName);
    std::string getPlayerTeam(const std::string& playerName);

    // ========================================================================
    // AssetService
    // ========================================================================
    void initAssetService();
    void shutdownAssetService();
    AssetInfo getAssetInfo(const std::string& assetId);
    std::string getAssetThumbnailUrl(const std::string& assetId, int width = 150, int height = 150);
    bool downloadAsset(const std::string& assetId, const std::string& outputPath);

    // ========================================================================
    // Lighting
    // ========================================================================
    void initLightingService();
    void setClockTime(float hours);         // 0-24
    float getClockTime();
    void setBrightness(float brightness);   // 0-1
    float getBrightness();
    void setAmbientColor(uint32_t color);
    uint32_t getAmbientColor();
    void setOutdoorAmbient(uint32_t color);
    void setFogEnd(float distance);
    float getFogEnd();
    void setFogColor(uint32_t color);
    void setGlobalShadows(bool enabled);
    void setTechnology(int tech);           // 0=Bloom, 1=Future
    void setGeographicLatitude(float deg);
    void setAmbient(uint32_t color);

    // ========================================================================
    // InsertService - Download sets/models from Pizzaboy Sets API
    // Download endpoint: https://sets.pizzaboxer.xyz/assetversions/{versionId}/download
    // ========================================================================
    void initInsertService();
    void shutdownInsertService();

    // Configure URLs for Pizzaboy Sets API
    void SetAssetUrl(const char* url);
    void SetPlaceUrl(const char* url);
    void SetAvatarUrl(const char* url);
    void SetModelUrl(const char* url);
    void SetDecalUrl(const char* url);
    void SetSoundUrl(const char* url);
    void SetScriptUrl(const char* url);
    void SetBadgeUrl(const char* url);
    void SetAvatarPackageUrl(const char* url);
    void SetMonthsOfFreePremiumUrl(const char* url);
    void SetClothingPackageUrl(const char* url);
    void SetGemUrl(const char* url);
    void SetMusicUrl(const char* url);
    void setInsertCallback(const std::function<void(const std::string&)>& callback);

    // Load a set/model from Pizzaboy Sets API - returns RBXM binary data
    std::string LoadAsset(const std::string& assetId, int assetType = 0);

    // Load a local asset file (returns empty if not found)
    std::string LoadLocalAsset(const std::string& assetId);

    // Get collection info from Pizzaboy Sets API
    std::string GetCollection(const std::string& collectionId);

    // Load model from Sets API (alias for LoadAsset with model type)
    std::string LoadModel(const std::string& modelId);

    // Stub implementations for Roblox API compatibility
    std::string LoadPlaceFile(const std::string& placeId);
    std::string LoadDecal(const std::string& decalId);
    std::string LoadSound(const std::string& soundId);
    std::string LoadAvatar(const std::string& avatarId);
    std::string LoadBadge(const std::string& badgeId);
    std::string LoadClothingPackage(const std::string& packageId);
    std::string LoadBadgeToPlayerReplicated(const std::string& badgeId, const std::string& callback);
    std::string LoadCollectionToBackpackReplicated(const std::string& collectionId, const std::string& callback);
    std::string LoadPlayerCreatedAsset(const std::string& assetId, const std::string& callback);
    std::string LoadBadgeToPlayerReplicatedString(const std::string& badgeId, const std::string& callback);
    std::string LoadCollectionToBackpackReplicatedString(const std::string& collectionId, const std::string& callback);
    std::string LoadPlayerCreatedAssetString(const std::string& assetId, const std::string& callback);

    // ========================================================================
    // CollectionService
    // ========================================================================
    void initCollectionService();
    void addTag(const std::string& instanceRef, const std::string& tag);
    void removeTag(const std::string& instanceRef, const std::string& tag);
    bool hasTag(const std::string& instanceRef, const std::string& tag);
    std::vector<std::string> getTags(const std::string& instanceRef);
    std::vector<std::string> getInstancesByTag(const std::string& tag);

    // ========================================================================
    // DataStoreService (file-based for 3DS offline support)
    // ========================================================================
    void initDataStoreService();
    void shutdownDataStoreService();
    DataStoreEntry getAsync(const std::string& storeName, const std::string& key);
    void setAsync(const std::string& storeName, const std::string& key, const std::string& value);
    void removeAsync(const std::string& storeName, const std::string& key);
    std::vector<std::string> getStoreNames();

    // ========================================================================
    // LocalizationService
    // ========================================================================
    void initLocalizationService();
    void setLocale(const std::string& locale);
    std::string getLocale();
    std::string getStringAsync(const std::string& tableId, const std::string& key);

    // ========================================================================
    // Statistics
    // ========================================================================
    void initStatisticsService();
    void setStat(const std::string& name, int value);
    int getStat(const std::string& name);

    // ========================================================================
    // TweenService
    // ========================================================================
    void initTweenService();
    // Create a tween: returns tween instance ID
    int createTween(const std::string& targetRef, const TweenInfo& tweenInfo,
                    const std::map<std::string, std::string>& goalProperties);
    void playTween(int tweenId);
    void cancelTween(int tweenId);
    void pauseTween(int tweenId);
    void resumeTween(int tweenId);
    // Update all tweens (call each frame)
    void updateTweens(float dt);

    // ========================================================================
    // PhysicsService
    // ========================================================================
    void initPhysicsService();
    void setCollisionGroup(const std::string& instanceRef, const std::string& group);
    void collisionGroupCollides(const std::string& group1, const std::string& group2, bool collides);

    // ========================================================================
    // ContentProvider
    // ========================================================================
    void initContentProvider();
    void preAsync(const std::vector<std::string>& assetIds);
    float getPreloadProgress();

    // ========================================================================
    // MarketPlaceService (stub - no real Roblox marketplace access on 3DS)
    // ========================================================================
    void initMarketPlaceService();

    // ========================================================================
    // StarterPlayer / StarterGui / StarterPack / ReplicatedStorage
    // ========================================================================
    void initStarterServices();

    // ========================================================================
    // RunService (bindable events for RenderStepped, Heartbeat, etc.)
    // ========================================================================
    void initRunService();
    void setRenderSteppedCallback(const std::function<void(float)>& callback);
    void setHeartbeatCallback(const std::function<void(float)>& callback);

    // ========================================================================
    // GUI / UserInputService / ContextActionService
    // ========================================================================
    void initInputServices();

    // ========================================================================
    // Workspace / TerrainService
    // ========================================================================
    void initWorkspaceServices();

    // ========================================================================
    // StudioService (for Studio mode)
    // ========================================================================
    void initStudioService();

    // ========================================================================
    // SoundService
    // ========================================================================
    void initSoundService();

    // ========================================================================
    // Players
    // ========================================================================
    void initPlayersService();

    // ========================================================================
    // Lua bindings for all services
    // Registers all service C functions into the Lua state
    // ========================================================================
    void registerAllServices(lua_State* L);

    // ========================================================================
    // Initialize all services at once
    // ========================================================================
    void initAll();
    // Shutdown all services
    void shutdownAll();
}