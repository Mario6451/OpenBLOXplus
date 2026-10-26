#pragma once

#include <string>
#include <vector>
#include <cstdint>

// OpenBLOX Multiplayer System
// 3DS Local Play via socket API with host/join model
// Supports up to 8 players in local multiplayer

namespace Multiplayer {

    // Maximum players in local multiplayer session
    static constexpr int MAX_PLAYERS = 8;

    // Maximum packet size for player state sync
    static constexpr int MAX_PACKET_SIZE = 1024;

    // Multiplayer session state
    enum class SessionState {
        Offline,         // No multiplayer, single player
        Lobby,           // In multiplayer lobby, waiting to join/host
        Hosting,         // Hosting a game session
        Joined,          // Joined a host's session
        Playing          // Game is in progress
    };

    // Player connection state
    enum class PlayerConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Syncing
    };

    // Player state for synchronization
    struct PlayerState {
        uint32_t playerId;
        float position[3];      // x, y, z
        float rotation[3];      // rx, ry, rz
        float velocity[3];      // vx, vy, vz
        float health;
        float maxHealth;
        bool isAlive;
        int32_t selectedTool;
        uint32_t timestamp;     // For sync lag compensation
    };

    // Remote player info
    struct RemotePlayer {
        uint32_t playerId;
        std::string playerName;
        PlayerConnectionState connectionState;
        int pingMs;            // Latency in milliseconds
        PlayerState playerState;
        bool isLocal;          // true = local player
    };

    // Packet types for network protocol
    enum class PacketType : uint8_t {
        Handshake,             // Initial connection handshake
        HandshakeReply,        // Server reply to handshake
        PlayerStateUpdate,     // Periodic state sync
        PlayerJoin,            // New player joined
        PlayerLeave,           // Player disconnected
        ChatMessage,           // Chat message sync
        SpawnRequest,          // Request spawn position
        SpawnReply,            // Reply with spawn position
        GameTick,              // Periodic game tick sync
        Acknowledge,           // Packet acknowledgment
        Error                  // Error message
    };

    // Network packet header
    struct PacketHeader {
        uint8_t type;          // PacketType
        uint32_t senderId;     // Sender player ID
        uint32_t sequence;     // Packet sequence number
        uint16_t payloadSize;  // Size of payload in bytes
    };

    // === Core API ===

    // Initialize multiplayer subsystem (socket layer)
    // Returns true if networking is available
    bool init();

    // Check if internet/local network is available
    bool isNetworkAvailable();

    // Set offline mode (disables multiplayer even if network available)
    void setOfflineMode(bool offline);

    // Check if offline mode is enabled
    bool isOfflineMode();

    // Get current session state
    SessionState getSessionState();

    // === Hosting ===

    // Start hosting a new game session
    // Returns true if hosting started successfully
    bool startHosting(const std::string& gamePath, const std::string& hostName);

    // Get host player ID (always 0)
    uint32_t getHostId();

    // === Joining ===

    // Discover available hosts on local network
    // Fills hosts vector with available session info
    bool discoverHosts(std::vector<std::string>& hosts);

    // Join a specific host by IP address
    // Returns true if join was successful
    bool joinHost(const std::string& hostIp, const std::string& playerName);

    // === Player Management ===

    // Get local player info
    RemotePlayer& getLocalPlayer();

    // Get remote player by index
    const RemotePlayer& getRemotePlayer(int index);

    // Get number of connected players (including local)
    int getPlayerCount();

    // Get player by ID
    const RemotePlayer* getPlayerById(uint32_t playerId);

    // === Networking ===

    // Process incoming network data (call once per frame)
    void processNetwork();

    // Send player state update
    void sendStateUpdate(const PlayerState& state);

    // Send chat message to all players
    void sendChatMessage(const std::string& playerName, const std::string& message);

    // Process incoming packets and update player states
    void updatePlayerStates();

    // Get average ping across all remote players
    int getAveragePing();

    // Get ping to specific player
    int getPing(uint32_t playerId);

    // === Lobby ===

    // Show multiplayer lobby UI
    // Returns selected action: "host", "join_XXX.XXX.XXX.XXX", or "" for cancel
    std::string showLobby();

    // === Shutdown ===

    // Leave current session and cleanup
    void leaveSession();

    // Full shutdown
    void shutdown();

    // Network discovery
    bool startDiscovery();
    void stopDiscovery();
    void updateDiscovery();
    // Server entry for network discovery
    struct ServerEntry {
        std::string ip;
        std::string name;
        int port;
        int players;
        int maxPlayers;
        uint64_t lastSeen;
    };

    // Get discovered servers
    std::vector<ServerEntry> getDiscoveredServers();
    bool isValidUserId(const char* userId);

    // Get local IP address
    std::string getLocalIpAddress();

    // Check if multiplayer is active (hosting or joined)
    bool isMultiplayerActive();

    // Get session name for display
    std::string getSessionName();

    // Get session game path for display
    std::string getSessionGamePath();
}