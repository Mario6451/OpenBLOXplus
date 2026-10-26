#include "multiplayer.h"
#include "font_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>       // for memalign
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <3ds.h>
#include <3ds/services/soc.h>

// MSG_NOSIGNAL is Linux-specific; on 3DS (newlib/BSD) writes to closed
// sockets return EPIPE without raising SIGPIPE, so 0 is sufficient
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// SO_RCVTIMEO is not defined in 3DS ctru headers
#ifndef SO_RCVTIMEO
#define SO_RCVTIMEO 1005
#endif

// SO_SNDTIMEO is not defined in 3DS ctru headers
#ifndef SO_SNDTIMEO
#define SO_SNDTIMEO 1006
#endif

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000  // 1MB buffer

namespace Multiplayer {

    // SOC buffer for 3DS socket initialization
    static u32* soc_buffer = NULL;

    // === Internal State ===
    static SessionState g_sessionState = SessionState::Offline;
    static bool g_offlineMode = false;
    static bool g_networkAvailable = false;
    static RemotePlayer g_players[MAX_PLAYERS];
    static int g_playerCount = 0;
    static uint32_t g_sequenceNumber = 0;
    static int g_localPlayerIndex = -1;

    // Server socket for hosting
    static int g_serverSocket = -1;
    static int g_clientSocket = -1;
    static int g_peerSockets[MAX_PLAYERS];
    static int g_peerCount = 0;
    static struct sockaddr_in g_serverAddr;
    static struct sockaddr_in g_hostAddr;
    static std::string g_sessionName;
    static std::string g_sessionGamePath;
    static std::string g_localHostName;

    // Discovery port for host discovery
    static constexpr int DISCOVERY_PORT = 27015;
    static constexpr int GAME_PORT = 27016;
    static constexpr int SYNC_INTERVAL_MS = 20;  // 50 Hz sync rate

    // Network discovery state
    static int g_discoverySocket = -1;
    static bool g_discoveryActive = false;
    static std::vector<ServerEntry> g_discoveredServers;
    static uint64_t g_lastDiscoveryTime = 0;
    static constexpr uint64_t DISCOVERY_TIMEOUT_MS = 3000;
    static constexpr uint64_t DISCOVERY_INTERVAL_MS = 10000;

    // Forward declaration for discovery broadcast
    static void sendDiscoveryBroadcast();

    // Helper to send to all peers
    static void broadcastToPeers(const uint8_t* data, size_t size) {
        for (int i = 0; i < g_peerCount; i++) {
            if (g_peerSockets[i] >= 0) {
                send(g_peerSockets[i], data, size, MSG_NOSIGNAL);
            }
        }
    }

    // Helper to send to host (for joined clients)
    static void sendToHost(const uint8_t* data, size_t size) {
        if (g_clientSocket >= 0) {
            send(g_clientSocket, data, size, MSG_NOSIGNAL);
        }
    }

    // Helper to send to specific peer
    static void sendToPeer(int peerIndex, const uint8_t* data, size_t size) {
        if (peerIndex >= 0 && peerIndex < g_peerCount && g_peerSockets[peerIndex] >= 0) {
            send(g_peerSockets[peerIndex], data, size, MSG_NOSIGNAL);
        }
    }

    // Initialize sockets for 3DS using SOC service
    static bool initSockets() {
        soc_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
        if (soc_buffer == NULL) {
            printf("[Multiplayer] Failed to allocate SOC buffer\n");
            return false;
        }
        Result rc = socInit(soc_buffer, SOC_BUFFERSIZE);
        if (R_FAILED(rc)) {
            printf("[Multiplayer] socInit failed: 0x%08X\n", (unsigned int)rc);
            free(soc_buffer);
            soc_buffer = NULL;
            return false;
        }
        g_networkAvailable = true;
        return true;
    }

    // Cleanup sockets
    static void cleanupSockets() {
        if (g_serverSocket >= 0) {
            close(g_serverSocket);
            g_serverSocket = -1;
        }
        if (g_clientSocket >= 0) {
            close(g_clientSocket);
            g_clientSocket = -1;
        }
        for (int i = 0; i < g_peerCount; i++) {
            if (g_peerSockets[i] >= 0) {
                close(g_peerSockets[i]);
                g_peerSockets[i] = -1;
            }
        }
        g_peerCount = 0;
    }

    // Process incoming handshake from client (host side)
    static void processHandshake(int clientSocket, const uint8_t* data, size_t size) {
        if (size < sizeof(PacketHeader)) return;

        PacketHeader header;
        memcpy(&header, data, sizeof(PacketHeader));

        if (header.type == (uint8_t)PacketType::Handshake) {
            // Extract player name from payload
            if (size > sizeof(PacketHeader)) {
                char playerName[64];
                size_t nameLen = size - sizeof(PacketHeader);
                if (nameLen > 63) nameLen = 63;
                memcpy(playerName, data + sizeof(PacketHeader), nameLen);
                playerName[nameLen] = 0;

                // Assign player ID
                uint32_t newPlayerId = g_playerCount;

                // Send handshake reply
                PacketHeader reply;
                reply.type = (uint8_t)PacketType::HandshakeReply;
                reply.senderId = 0;
                reply.sequence = ++g_sequenceNumber;
                reply.payloadSize = sizeof(uint32_t);
                uint32_t playerId = newPlayerId;

                send(clientSocket, &reply, sizeof(PacketHeader), MSG_NOSIGNAL);
                send(clientSocket, &playerId, sizeof(uint32_t), MSG_NOSIGNAL);

                // Send spawn reply
                PacketHeader spawnReply;
                spawnReply.type = (uint8_t)PacketType::SpawnReply;
                spawnReply.senderId = 0;
                spawnReply.sequence = ++g_sequenceNumber;
                spawnReply.payloadSize = 3 * sizeof(float);

                float spawnPos[] = {0.0f, 20.0f, 0.0f};
                send(clientSocket, &spawnReply, sizeof(PacketHeader), MSG_NOSIGNAL);
                send(clientSocket, spawnPos, 3 * sizeof(float), MSG_NOSIGNAL);

                // Add to player list
                if (g_playerCount < MAX_PLAYERS) {
                    RemotePlayer& p = g_players[g_playerCount];
                    p.playerId = newPlayerId;
                    p.playerName = playerName;
                    p.connectionState = PlayerConnectionState::Connected;
                    p.pingMs = 0;
                    p.isLocal = false;
                    memset(&p.playerState, 0, sizeof(PlayerState));
                    g_playerCount++;
                }

                // Broadcast player join to all peers
                PacketHeader joinPkt;
                joinPkt.type = (uint8_t)PacketType::PlayerJoin;
                joinPkt.senderId = 0;
                joinPkt.sequence = ++g_sequenceNumber;
                joinPkt.payloadSize = sizeof(uint32_t) + strlen(playerName) + 1;
                uint8_t joinBuffer[MAX_PACKET_SIZE];
                memcpy(joinBuffer, &joinPkt, sizeof(PacketHeader));
                memcpy(joinBuffer + sizeof(PacketHeader), &newPlayerId, sizeof(uint32_t));
                memcpy(joinBuffer + sizeof(PacketHeader) + sizeof(uint32_t), playerName, strlen(playerName) + 1);
                broadcastToPeers(joinBuffer, sizeof(PacketHeader) + sizeof(uint32_t) + strlen(playerName) + 1);
            }
        }
    }

    // Process state update (host side)
    static void processStateUpdateFromClient(int clientSocket, const uint8_t* data, size_t size) {
        if (size < sizeof(PacketHeader) + sizeof(PlayerState)) return;

        PlayerState state;
        memcpy(&state, data + sizeof(PacketHeader), sizeof(PlayerState));

        // Update the player state
        for (int i = 0; i < g_playerCount; i++) {
            if (g_players[i].playerId == state.playerId) {
                g_players[i].playerState = state;
                break;
            }
        }
    }

    // Process client packets (host side)
    static void processClientPackets(int clientSocket) {
        uint8_t buffer[MAX_PACKET_SIZE];
        memset(buffer, 0, MAX_PACKET_SIZE);

        ssize_t received = recv(clientSocket, buffer, MAX_PACKET_SIZE, MSG_PEEK);
        if (received <= 0) return;

        if (received < (ssize_t)sizeof(PacketHeader)) return;

        PacketHeader header;
        memcpy(&header, buffer, sizeof(PacketHeader));

        size_t expectedSize = sizeof(PacketHeader) + header.payloadSize;
        if (received < (ssize_t)expectedSize) return;

        // Read the packet
        recv(clientSocket, buffer, expectedSize, MSG_WAITALL);

        switch ((PacketType)header.type) {
            case PacketType::Handshake:
                processHandshake(clientSocket, buffer, expectedSize);
                break;
            case PacketType::PlayerStateUpdate:
                processStateUpdateFromClient(clientSocket, buffer, expectedSize);
                break;
            case PacketType::ChatMessage:
                // Relay chat message to all peers
                broadcastToPeers(buffer, expectedSize);
                break;
            default:
                break;
        }
    }

    // Process host packets (client side)
    static void processHostPackets() {
        if (g_clientSocket < 0) return;

        uint8_t buffer[MAX_PACKET_SIZE];
        memset(buffer, 0, MAX_PACKET_SIZE);

        ssize_t received = recv(g_clientSocket, buffer, MAX_PACKET_SIZE, MSG_DONTWAIT);
        if (received <= 0) return;

        if (received < (ssize_t)sizeof(PacketHeader)) return;

        PacketHeader header;
        memcpy(&header, buffer, sizeof(PacketHeader));

        switch ((PacketType)header.type) {
            case PacketType::HandshakeReply: {
                uint32_t playerId;
                recv(g_clientSocket, &playerId, sizeof(uint32_t), MSG_WAITALL);

                // Update local player
                if (g_localPlayerIndex >= 0) {
                    g_players[g_localPlayerIndex].playerId = playerId;
                }
                break;
            }
            case PacketType::SpawnReply: {
                float spawnPos[3];
                recv(g_clientSocket, spawnPos, 3 * sizeof(float), MSG_WAITALL);
                if (g_localPlayerIndex >= 0) {
                    memcpy(g_players[g_localPlayerIndex].playerState.position, spawnPos, 3 * sizeof(float));
                }
                break;
            }
            case PacketType::PlayerJoin: {
                uint32_t newPlayerId;
                recv(g_clientSocket, &newPlayerId, sizeof(uint32_t), MSG_WAITALL);

                // Remaining is player name
                if (received > (ssize_t)(sizeof(PacketHeader) + sizeof(uint32_t))) {
                    size_t nameLen = received - sizeof(PacketHeader) - sizeof(uint32_t);
                    char playerName[64];
                    memcpy(playerName, buffer + sizeof(PacketHeader) + sizeof(uint32_t), nameLen);
                    playerName[nameLen] = 0;

                    if (g_playerCount < MAX_PLAYERS) {
                        RemotePlayer& p = g_players[g_playerCount];
                        p.playerId = newPlayerId;
                        p.playerName = playerName;
                        p.connectionState = PlayerConnectionState::Connected;
                        p.pingMs = 0;
                        p.isLocal = false;
                        memset(&p.playerState, 0, sizeof(PlayerState));
                        g_playerCount++;
                    }
                }
                break;
            }
            case PacketType::PlayerLeave: {
                uint32_t leavingPlayerId;
                if (received > (ssize_t)sizeof(PacketHeader)) {
                    memcpy(&leavingPlayerId, buffer + sizeof(PacketHeader), sizeof(uint32_t));
                    for (int i = 0; i < g_playerCount; i++) {
                        if (g_players[i].playerId == leavingPlayerId) {
                            g_players[i].connectionState = PlayerConnectionState::Disconnected;
                            // Shift players
                            if (i < g_playerCount - 1) {
                                g_players[i] = g_players[g_playerCount - 1];
                            }
                            g_playerCount--;
                            break;
                        }
                    }
                }
                break;
            }
            case PacketType::PlayerStateUpdate: {
                if (received >= (ssize_t)(sizeof(PacketHeader) + sizeof(PlayerState))) {
                    PlayerState state;
                    memcpy(&state, buffer + sizeof(PacketHeader), sizeof(PlayerState));

                    // Update remote player
                    for (int i = 0; i < g_playerCount; i++) {
                        if (g_players[i].playerId == state.playerId && !g_players[i].isLocal) {
                            g_players[i].playerState = state;
                            break;
                        }
                    }
                }
                break;
            }
            case PacketType::ChatMessage: {
                if (received > (ssize_t)sizeof(PacketHeader)) {
                    uint32_t nameLen;
                    if (received > sizeof(PacketHeader) + sizeof(uint32_t)) {
                        memcpy(&nameLen, buffer + sizeof(PacketHeader), sizeof(uint32_t));
                        if (received > sizeof(PacketHeader) + sizeof(uint32_t) + nameLen + 1) {
                            // Could process chat here if needed
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // === Public API Implementation ===

    bool init() {
        g_sessionState = SessionState::Offline;
        g_offlineMode = false;
        memset(g_players, 0, sizeof(g_players));
        g_playerCount = 0;
        g_sequenceNumber = 0;
        g_localPlayerIndex = -1;
        g_serverSocket = -1;
        g_clientSocket = -1;
        g_peerCount = 0;
        memset(g_peerSockets, -1, sizeof(g_peerSockets));

        // Try to check network availability
        g_networkAvailable = initSockets();

        if (!g_networkAvailable) {
            g_sessionState = SessionState::Offline;
            g_offlineMode = true;
        }

        return g_networkAvailable;
    }

    bool isNetworkAvailable() {
        return g_networkAvailable;
    }

    void setOfflineMode(bool offline) {
        g_offlineMode = offline;
        if (offline && isMultiplayerActive()) {
            leaveSession();
        }
    }

    bool isOfflineMode() {
        return g_offlineMode;
    }

    SessionState getSessionState() {
        return g_sessionState;
    }

    bool startHosting(const std::string& gamePath, const std::string& hostName) {
        if (g_offlineMode || !g_networkAvailable) return false;

        g_sessionName = hostName + "'s Game";
        g_sessionGamePath = gamePath;
        g_localHostName = hostName;

        // Setup local player
        if (g_localPlayerIndex < 0) {
            g_localPlayerIndex = g_playerCount;
            g_players[g_playerCount].playerId = 0;
            g_players[g_playerCount].playerName = hostName;
            g_players[g_playerCount].connectionState = PlayerConnectionState::Connected;
            g_players[g_playerCount].pingMs = 0;
            g_players[g_playerCount].isLocal = true;
            g_players[g_playerCount].playerState.playerId = 0;
            g_players[g_playerCount].playerState.health = 100.0f;
            g_players[g_playerCount].playerState.maxHealth = 100.0f;
            g_players[g_playerCount].playerState.isAlive = true;
            g_players[g_playerCount].playerState.selectedTool = -1;
            g_playerCount++;
        }

        // Create server socket
        g_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (g_serverSocket < 0) return false;

        int opt = 1;
        setsockopt(g_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        memset(&g_serverAddr, 0, sizeof(g_serverAddr));
        g_serverAddr.sin_family = AF_INET;
        g_serverAddr.sin_addr.s_addr = INADDR_ANY;
        g_serverAddr.sin_port = htons(GAME_PORT);

        // Set non-blocking
        fcntl(g_serverSocket, F_SETFL, fcntl(g_serverSocket, F_GETFL, 0) | O_NONBLOCK);

        if (bind(g_serverSocket, (struct sockaddr*)&g_serverAddr, sizeof(g_serverAddr)) < 0) {
            close(g_serverSocket);
            g_serverSocket = -1;
            return false;
        }

        if (listen(g_serverSocket, MAX_PLAYERS) < 0) {
            close(g_serverSocket);
            g_serverSocket = -1;
            return false;
        }

        g_sessionState = SessionState::Hosting;
        return true;
    }

    uint32_t getHostId() {
        return 0;
    }

    bool discoverHosts(std::vector<std::string>& hosts) {
        hosts.clear();

        // Simple UDP broadcast to discover hosts
        int discoverSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (discoverSocket < 0) return false;

        int opt = 1;
        setsockopt(discoverSocket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

        // Send discovery request
        struct sockaddr_in broadcastAddr;
        memset(&broadcastAddr, 0, sizeof(broadcastAddr));
        broadcastAddr.sin_family = AF_INET;
        broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;
        broadcastAddr.sin_port = htons(DISCOVERY_PORT);

        const char* discoverMsg = "DISCOVER";
        sendto(discoverSocket, discoverMsg, strlen(discoverMsg), 0,
               (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

        // Wait for responses (short timeout)
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000; // 500ms
        setsockopt(discoverSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        char buffer[256];
        struct sockaddr_in fromAddr;
        socklen_t fromLen = sizeof(fromAddr);

        while (recvfrom(discoverSocket, buffer, sizeof(buffer), 0,
                        (struct sockaddr*)&fromAddr, &fromLen) > 0) {
            if (strncmp(buffer, "DISCOVER_REPLY", 14) == 0) {
                char hostInfo[256];
                char ip[46];
                inet_ntop(AF_INET, &fromAddr.sin_addr, ip, sizeof(ip));
                snprintf(hostInfo, sizeof(hostInfo), "%s", ip);
                hosts.push_back(std::string(hostInfo + 15)); // Skip "DISCOVER_REPLY: "
            }
        }

        close(discoverSocket);
        return !hosts.empty();
    }

    bool joinHost(const std::string& hostIp, const std::string& playerName) {
        if (g_offlineMode || !g_networkAvailable) return false;

        // Setup local player
        if (g_localPlayerIndex < 0) {
            g_localPlayerIndex = g_playerCount;
            g_players[g_playerCount].playerId = 0;
            g_players[g_playerCount].playerName = playerName;
            g_players[g_playerCount].connectionState = PlayerConnectionState::Connecting;
            g_players[g_playerCount].pingMs = 0;
            g_players[g_localPlayerIndex].isLocal = true;
            g_players[g_playerCount].playerState.playerId = 0;
            g_players[g_playerCount].playerState.health = 100.0f;
            g_players[g_playerCount].playerState.maxHealth = 100.0f;
            g_players[g_playerCount].playerState.isAlive = true;
            g_players[g_playerCount].playerState.selectedTool = -1;
            g_playerCount++;
        }

        // Create client socket
        g_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (g_clientSocket < 0) return false;

        memset(&g_hostAddr, 0, sizeof(g_hostAddr));
        g_hostAddr.sin_family = AF_INET;
        inet_pton(AF_INET, hostIp.c_str(), &g_hostAddr.sin_addr);
        g_hostAddr.sin_port = htons(GAME_PORT);

        // Connect
        if (connect(g_clientSocket, (struct sockaddr*)&g_hostAddr, sizeof(g_hostAddr)) < 0) {
            close(g_clientSocket);
            g_clientSocket = -1;
            return false;
        }

        // Send handshake
        PacketHeader header;
        header.type = (uint8_t)PacketType::Handshake;
        header.senderId = 0;
        header.sequence = ++g_sequenceNumber;
        header.payloadSize = playerName.size() + 1;

        uint8_t pkt[MAX_PACKET_SIZE];
        memcpy(pkt, &header, sizeof(PacketHeader));
        memcpy(pkt + sizeof(PacketHeader), playerName.c_str(), playerName.size() + 1);

        send(g_clientSocket, pkt, sizeof(PacketHeader) + playerName.size() + 1, MSG_NOSIGNAL);

        g_sessionState = SessionState::Joined;
        g_sessionName = "Joined Game";
        return true;
    }

    RemotePlayer& getLocalPlayer() {
        if (g_localPlayerIndex >= 0 && g_localPlayerIndex < g_playerCount) {
            return g_players[g_localPlayerIndex];
        }
        return g_players[0];
    }

    const RemotePlayer& getRemotePlayer(int index) {
        if (index >= 0 && index < g_playerCount) {
            return g_players[index];
        }
        return g_players[0];
    }

    int getPlayerCount() {
        return g_playerCount;
    }

    const RemotePlayer* getPlayerById(uint32_t playerId) {
        for (int i = 0; i < g_playerCount; i++) {
            if (g_players[i].playerId == playerId) {
                return &g_players[i];
            }
        }
        return nullptr;
    }

    void processNetwork() {
        switch (g_sessionState) {
            case SessionState::Hosting: {
                // Check for new connections
                struct sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                int newClient = accept(g_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
                if (newClient >= 0 && g_peerCount < MAX_PLAYERS) {
                    g_peerSockets[g_peerCount] = newClient;
                    g_peerCount++;
                }

                // Process each peer
                for (int i = 0; i < g_peerCount; i++) {
                    if (g_peerSockets[i] >= 0) {
                        processClientPackets(g_peerSockets[i]);
                    }
                }
                break;
            }
            case SessionState::Joined:
                processHostPackets();
                break;
            default:
                break;
        }
    }

    void sendStateUpdate(const PlayerState& state) {
        if (!isMultiplayerActive()) return;

        PacketHeader header;
        header.type = (uint8_t)PacketType::PlayerStateUpdate;
        header.senderId = state.playerId;
        header.sequence = ++g_sequenceNumber;
        header.payloadSize = sizeof(PlayerState);

        uint8_t pkt[MAX_PACKET_SIZE];
        memcpy(pkt, &header, sizeof(PacketHeader));
        memcpy(pkt + sizeof(PacketHeader), &state, sizeof(PlayerState));

        if (g_sessionState == SessionState::Joined) {
            sendToHost(pkt, sizeof(PacketHeader) + sizeof(PlayerState));
        } else if (g_sessionState == SessionState::Hosting) {
            broadcastToPeers(pkt, sizeof(PacketHeader) + sizeof(PlayerState));
        }
    }

    void sendChatMessage(const std::string& playerName, const std::string& message) {
        if (!isMultiplayerActive()) return;

        PacketHeader header;
        header.type = (uint8_t)PacketType::ChatMessage;
        header.senderId = 0;
        header.sequence = ++g_sequenceNumber;
        header.payloadSize = playerName.size() + message.size() + 2;

        uint8_t pkt[MAX_PACKET_SIZE];
        memcpy(pkt, &header, sizeof(PacketHeader));
        uint32_t nameLen = playerName.size();
        memcpy(pkt + sizeof(PacketHeader), &nameLen, sizeof(uint32_t));
        memcpy(pkt + sizeof(PacketHeader) + sizeof(uint32_t), playerName.c_str(), playerName.size() + 1);
        memcpy(pkt + sizeof(PacketHeader) + sizeof(uint32_t) + playerName.size() + 1,
               message.c_str(), message.size() + 1);

        size_t pktSize = sizeof(PacketHeader) + sizeof(uint32_t) + playerName.size() + 1 + message.size() + 1;

        if (g_sessionState == SessionState::Joined) {
            sendToHost(pkt, pktSize);
        } else if (g_sessionState == SessionState::Hosting) {
            broadcastToPeers(pkt, pktSize);
        }
    }

    void updatePlayerStates() {
        // This can be extended with interpolation/extrapolation
        // Currently states are updated directly in processNetwork
    }

    int getAveragePing() {
        if (g_playerCount <= 1) return 0;
        int totalPing = 0;
        int count = 0;
        for (int i = 0; i < g_playerCount; i++) {
            if (!g_players[i].isLocal) {
                totalPing += g_players[i].pingMs;
                count++;
            }
        }
        return count > 0 ? totalPing / count : 0;
    }

    int getPing(uint32_t playerId) {
        for (int i = 0; i < g_playerCount; i++) {
            if (g_players[i].playerId == playerId) {
                return g_players[i].pingMs;
            }
        }
        return -1;
    }

    std::string showLobby() {
        consoleInit(GFX_BOTTOM, NULL);
        int selectedOption = 0;
        bool running = true;

        while (running) {
            consoleClear();
            iprintf("=== MULTIPLAYER LOBBY ===\n");
            iprintf("\n");

            if (g_offlineMode) {
                iprintf("OFFLINE MODE ENABLED\n");
                iprintf("Disable in settings.\n");
                iprintf("\n");
            }

            if (!g_offlineMode && g_networkAvailable) {
                if (selectedOption == 0)
                    iprintf("-> Host Game\n");
                else
                    iprintf("   Host Game\n");

                if (selectedOption == 1)
                    iprintf("-> Join Game (Discover)\n");
                else
                    iprintf("   Join Game (Discover)\n");

                if (selectedOption == 2)
                    iprintf("-> Cancel\n");
                else
                    iprintf("   Cancel\n");
            } else {
                if (selectedOption == 0)
                    iprintf("-> Cancel\n");
                else
                    iprintf("   Cancel\n");
            }

            // Input handling
            hidScanInput();
            u32 kDown = hidKeysDown();

            if (kDown & KEY_UP) {
                if (!g_offlineMode && g_networkAvailable) {
                    selectedOption--;
                    if (selectedOption < 0) selectedOption = 0;
                }
            }
            if (kDown & KEY_DOWN) {
                if (!g_offlineMode && g_networkAvailable) {
                    selectedOption++;
                    if (selectedOption > 2) selectedOption = 2;
                }
            }

            if (kDown & KEY_A) {
                if (selectedOption == 0 && !g_offlineMode && g_networkAvailable) {
                    return "host";
                } else if (selectedOption == 1 && !g_offlineMode && g_networkAvailable) {
                    // Discover hosts
                    std::vector<std::string> hosts;
                    discoverHosts(hosts);
                    if (!hosts.empty()) {
                        // Return first host for simplicity
                        return "join_" + hosts[0];
                    }
                } else {
                    return "";
                }
            }

            if (kDown & KEY_B) {
                return "";
            }

            svcSleepThread(10000000ULL);
        }

        return "";
    }

    void leaveSession() {
        cleanupSockets();

        // Reset players, keeping local player info
        for (int i = 0; i < g_playerCount; i++) {
            if (!g_players[i].isLocal) {
                g_players[i].connectionState = PlayerConnectionState::Disconnected;
            }
        }
        g_playerCount = 1; // Keep only local player
        g_localPlayerIndex = 0;

        g_sessionState = SessionState::Offline;
        g_peerCount = 0;
    }

    void shutdown() {
        leaveSession();
        g_networkAvailable = false;
        if (soc_buffer) {
            socExit();
            free(soc_buffer);
            soc_buffer = NULL;
        }
        memset(g_players, 0, sizeof(g_players));
        g_playerCount = 0;
        g_localPlayerIndex = -1;
    }

    bool isMultiplayerActive() {
        return g_sessionState == SessionState::Hosting ||
               g_sessionState == SessionState::Joined ||
               g_sessionState == SessionState::Playing;
    }

    std::string getSessionName() {
        return g_sessionName;
    }

    std::string getSessionGamePath() {
        return g_sessionGamePath;
    }

    // === Network Discovery Implementation ===

    std::string getLocalIpAddress() {
        // Use getaddrinfo to get local IP address
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            struct addrinfo hints, *res;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
                struct sockaddr_in* sa = (struct sockaddr_in*)res->ai_addr;
                std::string ip = inet_ntoa(sa->sin_addr);
                freeaddrinfo(res);
                return ip;
            }
        }
        return "127.0.0.1";
    }

    bool startDiscovery() {
        if (g_discoveryActive) return true;

        // Close old socket if exists
        if (g_discoverySocket >= 0) {
            close(g_discoverySocket);
            g_discoverySocket = -1;
        }

        // Create UDP socket for discovery
        g_discoverySocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (g_discoverySocket < 0) {
            printf("[Multiplayer] Failed to create discovery socket\n");
            return false;
        }

        // Set socket to non-blocking
        int flags = fcntl(g_discoverySocket, F_GETFL, 0);
        fcntl(g_discoverySocket, F_SETFL, flags | O_NONBLOCK);

        // Bind to discovery port
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(DISCOVERY_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(g_discoverySocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            printf("[Multiplayer] Failed to bind discovery socket\n");
            close(g_discoverySocket);
            g_discoverySocket = -1;
            return false;
        }

        g_discoveryActive = true;
        g_discoveredServers.clear();
        g_lastDiscoveryTime = osGetTime() / 1000000ULL;

        // Send broadcast discover packet
        sendDiscoveryBroadcast();

        printf("[Multiplayer] Discovery started\n");
        return true;
    }

    void sendDiscoveryBroadcast() {
        if (g_discoverySocket < 0) return;

        // Send broadcast "OPENBLOX_DISCOVER" packet
        const char* packet = "OPENBLOX_DISCOVER";
        struct sockaddr_in broadcast;
        memset(&broadcast, 0, sizeof(broadcast));
        broadcast.sin_family = AF_INET;
        broadcast.sin_port = htons(DISCOVERY_PORT);
        broadcast.sin_addr.s_addr = INADDR_BROADCAST;

        // Enable broadcast
        int broadcast_opt = 1;
        setsockopt(g_discoverySocket, SOL_SOCKET, SO_BROADCAST, &broadcast_opt, sizeof(broadcast_opt));

        sendto(g_discoverySocket, packet, strlen(packet), 0,
               (struct sockaddr*)&broadcast, sizeof(broadcast));

        // Also try local subnet
        std::string ip = getLocalIpAddress();
        char subnet[64];
        snprintf(subnet, sizeof(subnet), "%s.255", ip.substr(0, ip.rfind('.')).c_str());
        broadcast.sin_addr.s_addr = inet_addr(subnet);
        sendto(g_discoverySocket, packet, strlen(packet), 0,
               (struct sockaddr*)&broadcast, sizeof(broadcast));
    }

    void processDiscoveryResponse(const char* data, size_t size, const struct sockaddr_in* from) {
        // Parse JSON-like response: ip|port|map|players|maxplayers|name
        char buf[256];
        size_t len = size < sizeof(buf) - 1 ? size : sizeof(buf) - 1;
        memcpy(buf, data, len);
        buf[len] = 0;

        char ip[64] = "", map[128] = "", name[128] = "";
        int port = 27016, players = 0, maxplayers = 8;

        // Simple parser: split by '|'
        char* tokens[6];
        int count = 0;
        char* saveptr;
        char* tmp = strdup(buf);
        char* t = strtok_r(tmp, "|", &saveptr);
        while (t && count < 6) {
            tokens[count++] = t;
            t = strtok_r(NULL, "|", &saveptr);
        }
        free(tmp);

        if (count >= 6) {
            strncpy(ip, tokens[0], sizeof(ip) - 1);
            port = atoi(tokens[1]);
            strncpy(map, tokens[2], sizeof(map) - 1);
            players = atoi(tokens[3]);
            maxplayers = atoi(tokens[4]);
            strncpy(name, tokens[5], sizeof(name) - 1);
        }

        // Check if we already have this server
        for (auto& s : g_discoveredServers) {
            if (s.ip == ip && s.name == name) {
                s.players = players;
                s.maxPlayers = maxplayers;
                return; // Update existing entry
            }
        }

        // Add new server entry
        ServerEntry entry;
        entry.ip = std::string(ip);
        entry.name = std::string(name);
        entry.players = players;
        entry.maxPlayers = maxplayers;
        g_discoveredServers.push_back(entry);

        printf("[Multiplayer] Discovered server: %s (%s) port %d\n", name, ip, port);
    }

    void updateDiscovery() {
        if (!g_discoveryActive || g_discoverySocket < 0) return;

        uint64_t now = osGetTime() / 1000000ULL;

        // Resend broadcast every 10 seconds
        if (now - g_lastDiscoveryTime > DISCOVERY_INTERVAL_MS) {
            sendDiscoveryBroadcast();
            g_lastDiscoveryTime = now;
        }

        // Timeout entries older than 3 seconds
        for (int i = (int)g_discoveredServers.size() - 1; i >= 0; i--) {
            // Simple timeout - just keep entries (no timestamp tracking for now)
        }

        // Receive discovery responses
        uint8_t buf[256];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t received = recvfrom(g_discoverySocket, buf, sizeof(buf) - 1, 0,
                                     (struct sockaddr*)&from, &fromlen);
        if (received > 0) {
            buf[received] = 0;
            char* discover = strstr((char*)buf, "OPENBLOX_DISCOVER");
            if (discover) {
                // Someone sent discover - respond with our info if hosting
                if (g_sessionState == SessionState::Hosting ||
                    g_sessionState == SessionState::Playing) {
                    char response[256];
                    snprintf(response, sizeof(response), "%s|%d|%s|%d|%d|%s",
                             g_localHostName.c_str(),
                             g_serverAddr.sin_port,
                             g_sessionGamePath.c_str(),
                             g_playerCount - 1,  // exclude host
                             MAX_PLAYERS,
                             g_sessionName.c_str());
                    sendto(g_discoverySocket, response, strlen(response), 0,
                           (struct sockaddr*)&from, sizeof(from));
                }
            } else {
                // Server response
                processDiscoveryResponse((char*)buf, received, &from);
            }
        }
    }

    void stopDiscovery() {
        if (g_discoverySocket >= 0) {
            close(g_discoverySocket);
            g_discoverySocket = -1;
        }
        g_discoveryActive = false;
        g_discoveredServers.clear();
        printf("[Multiplayer] Discovery stopped\n");
    }

    std::vector<ServerEntry> getDiscoveredServers() {
        return g_discoveredServers;
    }

    bool isValidUserId(const char* userId) {
        if (!userId || strlen(userId) == 0) return false;
        if (strlen(userId) > 32) return false;
        // Check uniqueness - for now just check format
        for (int i = 0; userId[i]; i++) {
            char c = userId[i];
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-')) {
                return false;
            }
        }
        return true;
    }

}