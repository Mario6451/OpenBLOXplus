#pragma once
#include <string>
#include <vector>
#include <3ds.h>

namespace MapRating {
    struct MapReview {
        std::string mapPath;
        std::string username;
        int rating;          // 1-5 stars
        std::string comment;
        u64 timestamp;
    };

    void init();
    bool rateMap(const std::string& mapPath, int rating, const std::string& comment = "");
    int getRating(const std::string& mapPath);
    std::vector<MapReview> getReviews(const std::string& mapPath);
    float getAverageRating(const std::string& mapPath);
    bool exportToStreetPass(const std::string& mapPath);
    bool importFromStreetPass(void* data, size_t size);
    bool load();
    bool save();
    void shutdown();
}