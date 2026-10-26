#pragma once
#include <string>
#include <vector>
#include "ui_multiplayer.h"

namespace NetworkDiscovery {

    // Initialize network discovery
    // Returns true if UDP sockets can be created
    bool init();

    // Shutdown
    void shutdown();

    // Start listening for server announcements
    void startListening();

    // Send discovery broadcast
    void broadcastDiscovery();

    // Process incoming packets (call in game loop)
    void processPackets();

    // Get discovered servers
    std::vector<ServerEntry> getServerList();

    // Check if discovery is active
    bool isActive();

    // Refresh server list (resend broadcast, wait 3 seconds)
    void refreshServerList();

    // Start advertising this server
    // Call when hosting begins
    void startAdvertising(const std::string& mapName, const std::string& serverName,
                         int port, int players, int maxPlayers);

    // Stop advertising
    void stopAdvertising();

    // Parse JSON response from server
    bool parseServerResponse(const std::string& json, ServerEntry& entry);
};
