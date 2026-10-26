#pragma once
#include <vector>
#include <string>

namespace UIFileSelector {
    // Show a scrollable list and return selected index. Uses console and attempts a graphical preview.
    int selectFromList(const std::vector<std::string> &names);
    
    // Scan directory recursively for RBXL/RBXM files (supports folder navigation)
    std::vector<std::string> scanRbxlFiles(const std::string& basePath);
    
    // Filter RBXL files by search query (case-insensitive)
    std::vector<std::string> filterRbxlFiles(const std::vector<std::string>& files, const std::string& query);
    
    // Show a game selector UI with search, folders, and RBXL filtering
    // Returns selected path (empty on cancel)
    std::string showGameSelector(const std::string& title);
}
