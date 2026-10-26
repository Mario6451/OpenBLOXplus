#pragma once

#include <string>
#include <vector>
#include <map>
#include <time.h>

namespace MapReview {

    struct Review {
        std::string mapPath;
        std::string reviewer;
        int rating;           // 1-5 stars
        std::string comment;
        time_t timestamp;
    };

    void init();
    void shutdown();

    void rateMap(const std::string& mapPath, int rating, const std::string& comment = "");
    float getAverageRating(const std::string& mapPath);
    std::vector<Review>& getReviews(const std::string& mapPath);
    int getReviewCount(const std::string& mapPath);
    void drawStarRating(float rating, float x, float y, float scale = 1.0f);
    bool load(const std::string& path);
    bool save(const std::string& path);
    std::string exportForStreetPass();
    bool importFromStreetPass(const std::string& data);
}