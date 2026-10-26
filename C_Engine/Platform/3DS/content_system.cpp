#include "content_system.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace ContentSystem {

    static std::string shareddataRoot = "sdmc:/OpenBLOX/shareddata/";
    static std::vector<ContentPack> gContentPacks;
    static std::map<std::string, ContentEntry> gIndex;

    // Custom content sources
    struct CustomSource {
        std::string name;
        std::string path;
        int priority;
    };
    static std::vector<CustomSource> gCustomSources;

    // Get relative path for content type
    static const char* getTypeRelativePath(ContentType type) {
        switch (type) {
            case ContentType::Texture:   return "textures/";
            case ContentType::Sound:     return "sounds/";
            case ContentType::Mesh:      return "meshes/";
            case ContentType::Font:      return "fonts/";
            case ContentType::Sky:       return "sky/";
            case ContentType::Music:     return "music/";
            case ContentType::Script:    return "scripts/";
            case ContentType::Particle:  return "particles/";
            case ContentType::Model:     return "models/";
            default:                     return "";
        }
    }

    // Get file extensions for content type
    static std::vector<std::string> getTypeExtensions(ContentType type) {
        switch (type) {
            case ContentType::Texture:   return {".png", ".dds", ".jpg", ".jpeg", ".bmp"};
            case ContentType::Sound:     return {".ogg", ".mp3", ".wav", ".mp4"};
            case ContentType::Mesh:      return {".mesh", ".rbxm", ".obj", ".fbx"};
            case ContentType::Font:      return {".fnt", ".font", ".fontdef", ".ttf", ".otf"};
            case ContentType::Sky:       return {".png", ".dds", ".jpg", ".*"};
            case ContentType::Music:     return {".ogg", ".mp3", ".wav"};
            case ContentType::Script:    return {".lua", ".luac", ".rbxmx"};
            case ContentType::Particle:  return {".particle", ".material"};
            case ContentType::Model:     return {".rbxm", ".mesh", ".obj"};
            default:                     return {".*"};
        }
    }

    void init() {
        gContentPacks.clear();
        gIndex.clear();
        gCustomSources.clear();
        printf("Content system initialized\n");
        printf("Shareddata root: %s\n", shareddataRoot.c_str());
    }

    void setShareddataRoot(const std::string& path) {
        shareddataRoot = path;
        if (!shareddataRoot.empty() && shareddataRoot.back() != '/') {
            shareddataRoot += '/';
        }
    }

    const std::string& getShareddataRoot() {
        return shareddataRoot;
    }

    std::string getPlatformContentPath(int era) {
        switch (era) {
            case 2012: return shareddataRoot + "../platformcontent2012/";
            case 2015: return shareddataRoot + "../platformcontent2015/";
            case 2016: return shareddataRoot + "../platformcontent2016/";
            case 2019: return shareddataRoot + "../platformcontent2019/";
            case 2022: return shareddataRoot + "../PlatformContent2022/";
            default:   return shareddataRoot + "../platformcontent2012/";
        }
    }

    // Helper to check if file extension matches type
    static bool matchesType(const std::string& fileName, ContentType type) {
        std::vector<std::string> exts = getTypeExtensions(type);
        for (auto& ext : exts) {
            if (ext == ".*") return true;
            if (fileName.size() >= ext.size() &&
                fileName.substr(fileName.size() - ext.size()) == ext) {
                return true;
            }
        }
        return false;
    }

    void scanDirectory(const std::string& path, ContentSource source, std::map<std::string, ContentEntry>& index) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) continue;

            std::string fileName = entry->d_name;
            std::string fullPath = path + fileName;

            // Check file exists and get size
            struct stat st;
            if (stat(fullPath.c_str(), &st) != 0) continue;

            ContentEntry entry_data;
            entry_data.fileName = fileName;
            entry_data.fullPath = fullPath;
            entry_data.relativePath = path;
            entry_data.fileSize = st.st_size;
            entry_data.exists = true;

            // Detect type from extension
            bool typeDetected = false;
            for (int t = 0; t < (int)ContentType::Unknown; t++) {
                ContentType type = (ContentType)t;
                if (matchesType(fileName, type)) {
                    entry_data.type = type;
                    entry_data.source = source;
                    typeDetected = true;
                    break;
                }
            }

            if (!typeDetected) {
                entry_data.type = ContentType::Unknown;
            }

            // Create key: assetId:type
            std::string key = fileName;
            index[key] = entry_data;
        }
        closedir(dir);
    }

    void buildContentIndex() {
        gIndex.clear();

        // Scan content2007/ (built-in)
        std::string contentBase = shareddataRoot + "../content2007/";
        std::string contentPath = contentBase + getTypeRelativePath(ContentType::Texture);
        scanDirectory(contentPath, ContentSource::ContentFolder, gIndex);
        contentPath = contentBase + getTypeRelativePath(ContentType::Sound);
        scanDirectory(contentPath, ContentSource::ContentFolder, gIndex);
        contentPath = contentBase + getTypeRelativePath(ContentType::Mesh);
        scanDirectory(contentPath, ContentSource::ContentFolder, gIndex);
        contentPath = contentBase + getTypeRelativePath(ContentType::Font);
        scanDirectory(contentPath, ContentSource::ContentFolder, gIndex);

        // Scan shareddata/
        std::string sharedPath = shareddataRoot + getTypeRelativePath(ContentType::Texture);
        scanDirectory(sharedPath, ContentSource::SharedData, gIndex);
        sharedPath = shareddataRoot + getTypeRelativePath(ContentType::Sound);
        scanDirectory(sharedPath, ContentSource::SharedData, gIndex);
        sharedPath = shareddataRoot + getTypeRelativePath(ContentType::Mesh);
        scanDirectory(sharedPath, ContentSource::SharedData, gIndex);
        sharedPath = shareddataRoot + getTypeRelativePath(ContentType::Font);
        scanDirectory(sharedPath, ContentSource::SharedData, gIndex);

        // Scan platformcontent2012/
        std::string pc2012 = getPlatformContentPath(2012) + getTypeRelativePath(ContentType::Texture);
        scanDirectory(pc2012, ContentSource::PlatformContent2012, gIndex);
        pc2012 = getPlatformContentPath(2012) + getTypeRelativePath(ContentType::Sound);
        scanDirectory(pc2012, ContentSource::PlatformContent2012, gIndex);

        // Scan platformcontent2015/
        std::string pc2015 = getPlatformContentPath(2015) + getTypeRelativePath(ContentType::Texture);
        scanDirectory(pc2015, ContentSource::PlatformContent2015, gIndex);
        pc2015 = getPlatformContentPath(2015) + getTypeRelativePath(ContentType::Sound);
        scanDirectory(pc2015, ContentSource::PlatformContent2015, gIndex);

        // Scan platformcontent2016/
        std::string pc2016 = getPlatformContentPath(2016) + getTypeRelativePath(ContentType::Texture);
        scanDirectory(pc2016, ContentSource::PlatformContent2016, gIndex);
        pc2016 = getPlatformContentPath(2016) + getTypeRelativePath(ContentType::Sound);
        scanDirectory(pc2016, ContentSource::PlatformContent2016, gIndex);

        // Scan platformcontent2019/
        std::string pc2019 = getPlatformContentPath(2019) + getTypeRelativePath(ContentType::Texture);
        scanDirectory(pc2019, ContentSource::PlatformContent2019, gIndex);
        pc2019 = getPlatformContentPath(2019) + getTypeRelativePath(ContentType::Sound);
        scanDirectory(pc2019, ContentSource::PlatformContent2019, gIndex);

        // Scan platformcontent2022/
        std::string pc2022 = getPlatformContentPath(2022) + getTypeRelativePath(ContentType::Texture);
        scanDirectory(pc2022, ContentSource::PlatformContent2022, gIndex);
        pc2022 = getPlatformContentPath(2022) + getTypeRelativePath(ContentType::Sound);
        scanDirectory(pc2022, ContentSource::PlatformContent2022, gIndex);

        // Scan assetpacks/
        std::string packsPath = shareddataRoot + "assetpacks/";
        DIR* packsDir = opendir(packsPath.c_str());
        if (packsDir) {
            struct dirent* pack;
            while ((pack = readdir(packsDir)) != NULL) {
                if (pack->d_type == DT_DIR) continue;
                // Scan assetpack files
                std::string packFile = packsPath + pack->d_name;
                struct stat st;
                if (stat(packFile.c_str(), &st) != 0) continue;

                ContentEntry entry_data;
                entry_data.fileName = pack->d_name;
                entry_data.fullPath = packFile;
                entry_data.relativePath = packsPath;
                entry_data.fileSize = st.st_size;
                entry_data.exists = true;
                entry_data.source = ContentSource::AssetPacks;

                // Detect type
                bool typeDetected = false;
                for (int t = 0; t < (int)ContentType::Unknown; t++) {
                    ContentType type = (ContentType)t;
                    if (matchesType(pack->d_name, type)) {
                        entry_data.type = type;
                        typeDetected = true;
                        break;
                    }
                }
                if (!typeDetected) entry_data.type = ContentType::Unknown;

                gIndex[pack->d_name] = entry_data;
            }
            closedir(packsDir);
        }

        // Scan custom sources
        for (auto& src : gCustomSources) {
            scanDirectory(src.path, ContentSource::Unknown, gIndex);
        }

        printf("Content index built: %d entries\n", (int)gIndex.size());
    }

    int getIndexSize() {
        return (int)gIndex.size();
    }

    IndexStats getIndexStats() {
        IndexStats stats = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        stats.totalEntries = (int)gIndex.size();

        for (auto& pair : gIndex) {
            switch (pair.second.type) {
                case ContentType::Texture:   stats.textureCount++; break;
                case ContentType::Sound:     stats.soundCount++; break;
                case ContentType::Mesh:      stats.meshCount++; break;
                case ContentType::Font:      stats.fontCount++; break;
                case ContentType::Model:     stats.modelCount++; break;
                case ContentType::Script:    stats.scriptCount++; break;
                case ContentType::Particle:  stats.particleCount++; break;
                case ContentType::Sky:       stats.skyCount++; break;
                case ContentType::Music:     stats.musicCount++; break;
                default:                     stats.unknownCount++; break;
            }
            stats.totalSize += pair.second.fileSize;
        }
        return stats;
    }

    int getSourcePriority(ContentSource source) {
        switch (source) {
            case ContentSource::PlatformContent2022: return 8;
            case ContentSource::PlatformContent2019: return 7;
            case ContentSource::PlatformContent2016: return 6;
            case ContentSource::PlatformContent2015: return 5;
            case ContentSource::PlatformContent2012: return 4;
            case ContentSource::SharedData:          return 3;
            case ContentSource::AssetPacks:          return 2;
            case ContentSource::ContentFolder:       return 1;
            default:                                 return 0;
        }
    }

    std::string resolveAsset(const std::string& assetId, ContentType type, ContentSource preferredSource) {
        if (gIndex.empty()) {
            buildContentIndex();
        }

        ContentEntry* bestEntry = nullptr;
        int bestPriority = -1;

        for (auto& pair : gIndex) {
            if (pair.second.fileName == assetId && pair.second.type == type) {
                if (preferredSource != ContentSource::Unknown) {
                    if (pair.second.source == preferredSource) {
                        return pair.second.fullPath;
                    }
                }

                int priority = getSourcePriority(pair.second.source);
                if (priority > bestPriority) {
                    bestPriority = priority;
                    bestEntry = &pair.second;
                }
            }
        }

        if (bestEntry) {
            return bestEntry->fullPath;
        }

        return "";
    }

    std::vector<ContentEntry> getAllAssetPaths(const std::string& assetId, ContentType type) {
        std::vector<ContentEntry> results;

        for (auto& pair : gIndex) {
            if (pair.second.fileName == assetId && pair.second.type == type) {
                results.push_back(pair.second);
            }
        }

        // Sort by priority (highest first)
        std::sort(results.begin(), results.end(), [](const ContentEntry& a, const ContentEntry& b) {
            return getSourcePriority(a.source) > getSourcePriority(b.source);
        });

        return results;
    }

    bool assetExists(const std::string& assetId, ContentType type) {
        return !resolveAsset(assetId, type).empty();
    }

    bool getAssetData(const std::string& assetId, ContentType type, std::vector<u8>& data) {
        std::string path = resolveAsset(assetId, type);
        if (path.empty()) return false;

        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        data.resize(size);
        fread(data.data(), 1, size, f);
        fclose(f);
        return true;
    }

    void scanContentPacks() {
        gContentPacks.clear();

        // Scan assetpacks/ directory
        std::string packsPath = shareddataRoot + "assetpacks/";
        DIR* dir = opendir(packsPath.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) continue;

            std::string fileName = entry->d_name;
            std::string fullPath = packsPath + fileName;

            struct stat st;
            if (stat(fullPath.c_str(), &st) != 0) continue;

            ContentPack pack;
            pack.id = fileName;
            size_t dot = fileName.rfind('.');
            pack.name = (dot != std::string::npos) ? fileName.substr(0, dot) : fileName;
            pack.version = "1.0";
            pack.description = "";
            pack.isEnabled = true;
            pack.sizeBytes = st.st_size;
            pack.path = fullPath;

            gContentPacks.push_back(pack);
        }
        closedir(dir);

        printf("Content packs scanned: %d\n", (int)gContentPacks.size());
    }

    std::vector<ContentPack>& getContentPacks() {
        return gContentPacks;
    }

    void setContentPackEnabled(const std::string& packId, bool enabled) {
        for (auto& pack : gContentPacks) {
            if (pack.id == packId) {
                pack.isEnabled = enabled;
                break;
            }
        }
    }

    bool isContentPackEnabled(const std::string& packId) {
        for (auto& pack : gContentPacks) {
            if (pack.id == packId) {
                return pack.isEnabled;
            }
        }
        return false;
    }

    void addContentSource(const std::string& name, const std::string& path, int priority) {
        CustomSource src;
        src.name = name;
        src.path = path;
        src.priority = priority;
        gCustomSources.push_back(src);
    }

    ContentEntry* getContentSource(const std::string& name) {
        for (auto& pair : gIndex) {
            if (pair.second.fileName == name) {
                return &pair.second;
            }
        }
        return nullptr;
    }

    const ContentEntry* getIndexEntry(const std::string& assetId, ContentType type) {
        for (auto& pair : gIndex) {
            if (pair.second.fileName == assetId && pair.second.type == type) {
                return &pair.second;
            }
        }
        return nullptr;
    }

    void shutdown() {
        gIndex.clear();
        gContentPacks.clear();
        gCustomSources.clear();
        printf("Content system shutdown\n");
    }

}