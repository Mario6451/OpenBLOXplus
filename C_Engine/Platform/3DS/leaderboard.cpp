#include "leaderboard.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <algorithm>

namespace Leaderboard {
    static std::vector<LeaderboardEntry> entries;

    void init() { load(); }

    bool submitScore(const std::string& mapId, u64 score) {
        LeaderboardEntry e;
        e.username = "Player";
        e.score = score;
        e.rank = 0;
        e.timestamp = (u64)time(NULL);
        entries.push_back(e);
        std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.score > b.score;
        });
        for (u32 i = 0; i < entries.size(); i++) entries[i].rank = i + 1;
        save();
        return true;
    }

    std::vector<LeaderboardEntry> getLocalLeaderboard(const std::string& /*mapId*/) {
        return entries;
    }

    bool downloadLeaderboard(const std::string& /*mapId*/) { return false; }

    std::vector<LeaderboardEntry> getTopScores(const std::string& /*mapId*/, int count) {
        std::vector<LeaderboardEntry> result;
        for (int i = 0; i < count && i < (int)entries.size(); i++) result.push_back(entries[i]);
        return result;
    }

    int getPlayerRank(const std::string& /*mapId*/, const std::string& username) {
        for (u32 i = 0; i < entries.size(); i++) {
            if (entries[i].username == username) return (int)i + 1;
        }
        return -1;
    }

    void clearLocalData() { entries.clear(); save(); }

    bool save() {
        FILE* f = fopen("sdmc:/OpenBLOX/leaderboard.dat", "wb");
        if (!f) return false;
        u32 count = (u32)entries.size();
        fwrite(&count, sizeof(u32), 1, f);
        for (auto& e : entries) {
            char name[64];
            strncpy(name, e.username.c_str(), 63); name[63] = 0;
            fwrite(name, 64, 1, f);
            fwrite(&e.score, sizeof(u64), 1, f);
            fwrite(&e.rank, sizeof(u32), 1, f);
            fwrite(&e.timestamp, sizeof(u64), 1, f);
        }
        fclose(f);
        return true;
    }

    bool load() {
        FILE* f = fopen("sdmc:/OpenBLOX/leaderboard.dat", "rb");
        if (!f) return true;
        u32 count = 0;
        fread(&count, sizeof(u32), 1, f);
        entries.resize(count);
        for (u32 i = 0; i < count; i++) {
            char name[64];
            fread(name, 64, 1, f);
            entries[i].username = name;
            fread(&entries[i].score, sizeof(u64), 1, f);
            fread(&entries[i].rank, sizeof(u32), 1, f);
            fread(&entries[i].timestamp, sizeof(u64), 1, f);
        }
        fclose(f);
        return true;
    }

    void shutdown() { save(); }
}