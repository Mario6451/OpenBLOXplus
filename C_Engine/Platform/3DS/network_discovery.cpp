#include "network_discovery.h"
#include "ui_multiplayer.h"
#include <3ds.h>
#include <3ds/services/soc.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <string>
#include <vector>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000  // 1MB buffer

// MSG_NOSIGNAL is Linux-specific
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

namespace NetworkDiscovery {

    static bool g_initialized = false;
    static bool g_listening = false;
    static int g_listenSocket = -1;
    static std::vector<ServerEntry> g_servers;
    static uint64_t g_lastBroadcast = 0;
    static const int DISCOVERY_PORT = 27015;

    // SOC buffer
    static u32* soc_buffer = NULL;

    // Advertiser state
    static bool g_advertising = false;
    static std::string g_advertiseMap, g_advertiseName;
    static int g_advertisePort = 27016;
    static int g_advertisePlayers = 0, g_advertiseMaxPlayers = 8;

    bool init() {
        if (g_initialized) return true;

        soc_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
        if (!soc_buffer) {
            printf("[NetworkDiscovery] Failed to allocate SOC buffer\n");
            return false;
        }
        Result rc = socInit(soc_buffer, SOC_BUFFERSIZE);
        if (R_FAILED(rc)) {
            printf("[NetworkDiscovery] socInit failed: 0x%08X\n", (unsigned int)rc);
            free(soc_buffer);
            soc_buffer = NULL;
            return false;
        }
        g_initialized = true;
        printf("[NetworkDiscovery] Initialized\n");
        return true;
    }

    void shutdown() {
        stopAdvertising();
        if (g_listenSocket >= 0) {
            close(g_listenSocket);
            g_listenSocket = -1;
        }
        if (soc_buffer) {
            socExit();
            free(soc_buffer);
            soc_buffer = NULL;
        }
        g_initialized = false;
        g_listening = false;
    }

    void startListening() {
        if (!g_initialized) init();

        if (g_listenSocket >= 0) close(g_listenSocket);

        g_listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (g_listenSocket < 0) {
            printf("[NetworkDiscovery] Failed to create listen socket\n");
            return;
        }

        int flags = fcntl(g_listenSocket, F_GETFL, 0);
        fcntl(g_listenSocket, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(DISCOVERY_PORT);

        int optval = 1;
        setsockopt(g_listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        if (bind(g_listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            printf("[NetworkDiscovery] Failed to bind to port %d\n", DISCOVERY_PORT);
            close(g_listenSocket);
            g_listenSocket = -1;
            return;
        }

        g_listening = true;
        printf("[NetworkDiscovery] Listening on port %d\n", DISCOVERY_PORT);
    }

    void broadcastDiscovery() {
        if (!g_initialized) init();

        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) return;

        int optval = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_BROADCAST;
        addr.sin_port = htons(DISCOVERY_PORT);

        const char* msg = "OPENBLOX_DISCOVER";
        sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, sizeof(addr));

        close(sock);
        g_lastBroadcast = osGetTime() / 1000000UL;
    }

    void processPackets() {
        if (!g_listening || g_listenSocket < 0) return;

        char buf[512];
        struct sockaddr_in from;
        socklen_t fromLen = sizeof(from);

        int len = recvfrom(g_listenSocket, buf, sizeof(buf) - 1, 0,
                          (struct sockaddr*)&from, &fromLen);
        if (len <= 0) return;

        buf[len] = '\0';

        if (strncmp(buf, "OPENBLOX_DISCOVER", 17) == 0) {
            if (g_advertising) {
                char response[256];
                snprintf(response, sizeof(response),
                    "{\"ip\":\"%s\",\"port\":%d,\"map\":\"%s\",\"players\":\"%d/%d\",\"name\":\"%s\"}",
                    inet_ntoa(from.sin_addr), g_advertisePort,
                    g_advertiseMap.c_str(), g_advertisePlayers, g_advertiseMaxPlayers,
                    g_advertiseName.c_str());

                sendto(g_listenSocket, response, strlen(response), 0,
                      (struct sockaddr*)&from, sizeof(from));
            }
            return;
        }

        ServerEntry entry;
        if (parseServerResponse(buf, entry)) {
            char ipbuf[64];
            inet_ntop(AF_INET, &from.sin_addr, ipbuf, sizeof(ipbuf));
            entry.ip = std::string(ipbuf);
            bool duplicate = false;
            for (auto& s : g_servers) {
                if (s.ip == entry.ip && s.port == entry.port) {
                    s = entry;
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                g_servers.push_back(entry);
            }
        }
    }

    std::vector<ServerEntry> getServerList() {
        return g_servers;
    }

    bool isActive() {
        return g_listening;
    }

    void refreshServerList() {
        g_servers.clear();
        broadcastDiscovery();
        uint64_t start = osGetTime() / 1000000UL;
        while (osGetTime() / 1000000UL - start < 3000) {
            processPackets();
            usleep(100000);
        }
        printf("[NetworkDiscovery] Found %zu servers\n", g_servers.size());
    }

    void startAdvertising(const std::string& mapName, const std::string& serverName,
                         int port, int players, int maxPlayers) {
        g_advertising = true;
        g_advertiseMap = mapName;
        g_advertiseName = serverName;
        g_advertisePort = port;
        g_advertisePlayers = players;
        g_advertiseMaxPlayers = maxPlayers;
    }

    void stopAdvertising() {
        g_advertising = false;
    }

    bool parseServerResponse(const std::string& json, ServerEntry& entry) {
        // Simple JSON parser for server responses
        // Format: {"ip":"...","port":27016,"map":"...","players":"1/10","name":"..."}

        auto findStr = [&](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\":\"";
            size_t pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos += search.length();
            size_t end = json.find("\"", pos);
            if (end == std::string::npos) return "";
            return json.substr(pos, end - pos);
        };

        auto findInt = [&](const std::string& key) -> int {
            std::string search = "\"" + key + "\":";
            size_t pos = json.find(search);
            if (pos == std::string::npos) return 0;
            pos += search.length();
            int result = 0;
            for (size_t i = pos; i < json.size(); i++) {
                if (isdigit(json[i]) || json[i] == '-') {
                    result = result * 10 + (json[i] >= '0' && json[i] <= '9' ? json[i] - '0' : -1);
                } else break;
            }
            return result;
        };

        entry.ip = findStr("ip");
        entry.port = findInt("port");
        entry.map = findStr("map");
        entry.name = findStr("name");

        std::string playersStr = findStr("players");
        size_t slash = playersStr.find("/");
        int p = 0, mp = 8;
        if (slash != std::string::npos) {
            p = 0; mp = 8;
            for (size_t i = 0; i < slash && i < playersStr.size(); i++) p = p * 10 + (playersStr[i] - '0');
            for (size_t i = slash + 1; i < playersStr.size(); i++) mp = mp * 10 + (playersStr[i] - '0');
        }
        entry.players = p;
        entry.maxPlayers = mp;

        return !entry.ip.empty() || entry.port > 0;
    }
}
