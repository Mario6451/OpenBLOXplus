#include "rating_system.h"
#include <map>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <numeric>

namespace MapRating {
    static std::map<std::string, std::vector<MapReview>> reviews;
    static const char* RATING_PATH = "sdmc:/OpenBLOX/ratings.dat";

    void init() {
        load();
    }

    bool rateMap(const std::string& mapPath, int rating, const std::string& comment) {
        if (rating < 1 || rating > 5) return false;
        
        MapReview review;
        review.mapPath = mapPath;
        review.username = "Player";
        review.rating = rating;
        review.comment = comment;
        review.timestamp = (u64)time(NULL);
        
        reviews[mapPath].push_back(review);
        return save();
    }

    int getRating(const std::string& mapPath) {
        auto it = reviews.find(mapPath);
        if (it == reviews.end() || it->second.empty()) return 0;
        
        int sum = 0;
        for (auto& r : it->second) sum += r.rating;
        return sum / (int)it->second.size();
    }

    std::vector<MapReview> getReviews(const std::string& mapPath) {
        auto it = reviews.find(mapPath);
        if (it == reviews.end()) return {};
        return it->second;
    }

    float getAverageRating(const std::string& mapPath) {
        auto it = reviews.find(mapPath);
        if (it == reviews.end() || it->second.empty()) return 0.0f;
        
        int sum = 0;
        for (auto& r : it->second) sum += r.rating;
        return (float)sum / (float)it->second.size();
    }

    bool exportToStreetPass(const std::string& mapPath) {
        auto it = reviews.find(mapPath);
        if (it == reviews.end()) return false;
        
        // Format reviews into StreetPass-compatible binary blob
        size_t buf_size = 4 + it->second.size() * 256;
        void* buf = linearMemAlign(buf_size, 0x1000);
        if (!buf) return false;
        
        u32* header = (u32*)buf;
        *header = (u32)it->second.size();
        // Serialize reviews...
        
        linearFree(buf);
        return true;
    }

    bool importFromStreetPass(void* data, size_t size) {
        if (!data || size < 4) return false;
        // Deserialize reviews from StreetPass data
        return true;
    }

    bool load() {
        FILE* f = fopen(RATING_PATH, "rb");
        if (!f) return true; // No file yet is OK
        
        u32 count = 0;
        if (fread(&count, sizeof(u32), 1, f) != 1) {
            fclose(f);
            return false;
        }
        
        for (u32 i = 0; i < count; i++) {
            MapReview r;
            char path[256], user[64], comment[512];
            u32 rating32;
            
            fread(path, 256, 1, f);
            fread(user, 64, 1, f);
            fread(&rating32, 4, 1, f);
            fread(comment, 512, 1, f);
            fread(&r.timestamp, 8, 1, f);
            
            r.mapPath = path;
            r.username = user;
            r.rating = rating32;
            r.comment = comment;
            reviews[r.mapPath].push_back(r);
        }
        fclose(f);
        return true;
    }

    bool save() {
        FILE* f = fopen(RATING_PATH, "wb");
        if (!f) return false;
        
        u32 total = 0;
        for (auto& p : reviews) total += (u32)p.second.size();
        fwrite(&total, sizeof(u32), 1, f);
        
        for (auto& p : reviews) {
            for (auto& r : p.second) {
                fwrite(r.mapPath.c_str(), 256, 1, f);
                fwrite(r.username.c_str(), 64, 1, f);
                u32 rating32 = r.rating;
                fwrite(&rating32, 4, 1, f);
                fwrite(r.comment.c_str(), 512, 1, f);
                fwrite(&r.timestamp, 8, 1, f);
            }
        }
        fclose(f);
        return true;
    }

    void shutdown() {
        save();
        reviews.clear();
    }
}