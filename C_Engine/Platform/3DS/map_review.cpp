#include "map_review.h"
#include <stdio.h>
#include <algorithm>
#include "font_manager.h"
#include "settings.h"

static std::map<std::string, std::vector<MapReview::Review>> reviewsMap;

void MapReview::init() {
    reviewsMap.clear();
}

void MapReview::shutdown() {
    reviewsMap.clear();
}

void MapReview::rateMap(const std::string& mapPath, int rating, const std::string& comment) {
    MapReview::Review review;
    review.mapPath = mapPath;
    review.rating = std::clamp(rating, 1, 5);
    review.comment = comment;
    review.timestamp = time(nullptr);
    review.reviewer = Settings::get().general.playerName;
    
    reviewsMap[mapPath].push_back(review);
}

float MapReview::getAverageRating(const std::string& mapPath) {
    auto it = reviewsMap.find(mapPath);
    if (it == reviewsMap.end() || it->second.empty()) return 0.0f;
    
    float sum = 0.0f;
    for (const auto& r : it->second) {
        sum += r.rating;
    }
    return sum / (float)it->second.size();
}

std::vector<MapReview::Review>& MapReview::getReviews(const std::string& mapPath) {
    return reviewsMap[mapPath];
}

int MapReview::getReviewCount(const std::string& mapPath) {
    auto it = reviewsMap.find(mapPath);
    if (it == reviewsMap.end()) return 0;
    return (int)it->second.size();
}

void MapReview::drawStarRating(float rating, float x, float y, float scale) {
    // Draw star rating using simple characters
    for (int i = 0; i < 5; i++) {
        char star = (i < (int)rating) ? '*' : 'o';
        FontManager::drawText((int)(x + i * 12.0f * scale), (int)y, std::string(1, star), 0xFFFFFFFF);
    }
}

bool MapReview::load(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;
    
    int count = 0;
    if (fscanf(f, "%d\n", &count) != 1) { fclose(f); return false; }
    
    for (int i = 0; i < count; i++) {
        char mapPath[512], reviewer[256], comment[1024];
        int rating;
        long timestamp;
        if (fscanf(f, "%s\n%s\n%d\n%s\n%ld\n",
            mapPath, reviewer, &rating, comment, &timestamp) != 5) break;
        
        MapReview::Review review;
        review.mapPath = mapPath;
        review.reviewer = reviewer;
        review.rating = rating;
        review.comment = comment;
        review.timestamp = (time_t)timestamp;
        reviewsMap[mapPath].push_back(review);
    }
    fclose(f);
    return true;
}

bool MapReview::save(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;
    
    int totalCount = 0;
    for (auto& pair : reviewsMap) totalCount += (int)pair.second.size();
    
    fprintf(f, "%d\n", totalCount);
    for (auto& pair : reviewsMap) {
        for (const auto& r : pair.second) {
            fprintf(f, "%s\n%s\n%d\n%s\n%ld\n",
                r.mapPath.c_str(),
                r.reviewer.c_str(),
                r.rating,
                r.comment.c_str(),
                (long)r.timestamp);
        }
    }
    fclose(f);
    return true;
}

std::string MapReview::exportForStreetPass() {
    // Export reviews as string for StreetPass sharing
    std::string result;
    for (auto& pair : reviewsMap) {
        for (const auto& r : pair.second) {
            char buf[2048];
            snprintf(buf, sizeof(buf), "%s|%d|%s|%ld\n",
                r.mapPath.c_str(), r.rating, r.reviewer.c_str(), (long)r.timestamp);
            result += buf;
        }
    }
    return result;
}

bool MapReview::importFromStreetPass(const std::string& data) {
    const char* ptr = data.c_str();
    while (*ptr) {
        char mapPath[512], reviewer[256];
        int rating;
        long timestamp;
        char* end;
        
        // Parse: mapPath|rating|reviewer|timestamp\n
        if (sscanf(ptr, "%511[^|]|%d|%255[^|]|%ld%n", mapPath, &rating, reviewer, &timestamp, &end) != 4)
            break;
        
        int len = (int)(end - ptr);
        if (len <= 0) break;
        
        MapReview::Review review;
        review.mapPath = mapPath;
        review.reviewer = reviewer;
        review.rating = std::clamp(rating, 1, 5);
        review.timestamp = (time_t)timestamp;
        review.comment = "[StreetPass review]";
        reviewsMap[mapPath].push_back(review);
        
        ptr += len;
    }
    return true;
}