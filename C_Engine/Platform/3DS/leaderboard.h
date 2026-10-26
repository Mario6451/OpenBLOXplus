#pragma once
#include <string>
#include <vector>
#include <3ds.h>

namespace Leaderboard {
    struct LeaderboardEntry {
        std::string username;
        u64 score;
        u32 rank;
        u64 timestamp;
    };

    void init();

    // Submit score for a map
    bool submitScore(const std::string& mapId, u64 score);

    // Get local leaderboard for a map
    std::vector<LeaderboardEntry> getLocalLeaderboard(const std::string& mapId);

    // Download remote leaderboard (via asset_loader)
    bool downloadLeaderboard(const std::string& mapId);

    // Get top N scores
    std::vector<LeaderboardEntry> getTopScores(const std::string& mapId, int count);

    // Get player's rank for a map
    int getPlayerRank(const std::string& mapId, const std::string& username);

    // Clear local data
    void clearLocalData();

    // Save/load
    bool save();
    bool load();

    void shutdown();
}