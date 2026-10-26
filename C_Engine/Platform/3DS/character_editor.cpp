#include "character_editor.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace CharacterEditor {

    static CharacterAvatar gAvatar;
    static std::map<AvatarItemType, std::vector<AvatarItem>> gItems;
    static const std::string charcustomDir = "sdmc:/OpenBLOX/shareddata/charcustom/";
    static const std::string avatarSavePath = "sdmc:/OpenBLOX/shareddata/avatar.dat";

    // BrickColor RGB values (subset of common colors)
    static const struct {
        BrickColor id;
        u8 r, g, b;
    } brickColorTable[] = {
        { BrickColor::BrightBlue, 30, 144, 255 },
        { BrickColor::LightBlue, 120, 190, 255 },
        { BrickColor::BrightGreen, 50, 205, 50 },
        { BrickColor::BrightYellow, 255, 235, 0 },
        { BrickColor::BrightRed, 255, 50, 50 },
        { BrickColor::DarkNoyellow, 180, 140, 0 },
        { BrickColor::MediumBlue, 0, 0, 191 },
        { BrickColor::MediumGreen, 44, 124, 94 },
        { BrickColor::Black, 0, 0, 0 },
        { BrickColor::DarkGray, 70, 75, 80 },
        { BrickColor::Gray, 150, 150, 150 },
        { BrickColor::LightYellow, 255, 255, 170 },
        { BrickColor::DarkRed, 139, 0, 0 },
        { BrickColor::DarkBlue, 0, 0, 139 },
        { BrickColor::DarkGreen, 0, 100, 0 },
        { BrickColor::BrightViolet, 200, 0, 200 },
        { BrickColor::DarkOrange, 220, 130, 25 },
        { BrickColor::LightOrange, 240, 180, 0 },
        { BrickColor::Maroon, 110, 26, 33 },
        { BrickColor::NavyBlue, 0, 0, 100 },
        { BrickColor::PaleOak, 185, 160, 130 },
        { BrickColor::LightGray, 200, 200, 200 },
        { BrickColor::Cobalt, 0, 80, 180 },
        { BrickColor::DeepOrange, 220, 100, 0 },
        { BrickColor::DarkPurple, 80, 0, 120 },
        { BrickColor::Brown, 139, 90, 43 },
        { BrickColor::DarkCyan, 0, 100, 100 },
        { BrickColor::Blush, 240, 180, 180 },
        { BrickColor::GreenCyan, 0, 180, 100 },
        { BrickColor::DarkBrown, 70, 40, 20 },
        { BrickColor::OldYellow, 200, 200, 100 },
        { BrickColor::PastelBlue, 150, 190, 255 },
        { BrickColor::PastelGreen, 150, 255, 150 },
        { BrickColor::PastelViolet, 200, 150, 255 },
        { BrickColor::Pink, 255, 150, 180 },
        { BrickColor::BabyBlue, 150, 200, 255 },
        { BrickColor::LightPink, 255, 180, 200 },
        { BrickColor::ForestGreen, 0, 120, 0 },
    };

    void init() {
        gAvatar = CharacterAvatar();
        gAvatar.playerName = "Player";
        gAvatar.playerId = 0;
        gItems.clear();
        printf("Character editor initialized\n");
    }

    CharacterAvatar& getAvatar() {
        return gAvatar;
    }

    void setPlayerName(const std::string& name) {
        gAvatar.playerName = name;
    }

    void setPlayerId(int id) {
        gAvatar.playerId = id;
    }

    void setBodyColor(BodyPart part, BrickColor color) {
        int idx = (int)part;
        if (idx >= 0 && idx < (int)BodyPart::Count) {
            gAvatar.bodyColors[idx] = color;
        }
    }

    BrickColor getBodyColor(BodyPart part) {
        int idx = (int)part;
        if (idx >= 0 && idx < (int)BodyPart::Count) {
            return gAvatar.bodyColors[idx];
        }
        return BrickColor::BrightBlue;
    }

    // Helper to scan a directory for files
    static void scanDirectory(const std::string& path, AvatarItemType type, std::vector<AvatarItem>& items) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) continue;

            std::string fileName = entry->d_name;
            std::string fullPath = path + fileName;

            // Check extension
            bool valid = false;
            switch (type) {
                case AvatarItemType::Shirt:
                    if (fileName.find(".png") != std::string::npos) valid = true;
                    break;
                case AvatarItemType::Pants:
                    if (fileName.find(".png") != std::string::npos) valid = true;
                    break;
                case AvatarItemType::TShirt:
                    if (fileName.find(".png") != std::string::npos) valid = true;
                    break;
                case AvatarItemType::Face:
                    if (fileName.find(".png") != std::string::npos || fileName.find(".dds") != std::string::npos) valid = true;
                    break;
                case AvatarItemType::Hat:
                    if (fileName.find(".mesh") != std::string::npos || fileName.find(".rbxm") != std::string::npos) valid = true;
                    break;
                default:
                    break;
            }

            if (valid) {
                AvatarItem item;
                item.id = fileName;
                // Get name without extension
                size_t dot = fileName.rfind('.');
                item.name = (dot != std::string::npos) ? fileName.substr(0, dot) : fileName;
                item.filePath = fullPath;
                item.type = type;
                item.isEnabled = true;
                items.push_back(item);
            }
        }
        closedir(dir);
    }

    void scanAvatarItems() {
        gItems.clear();

        // Scan each category
        scanDirectory(charcustomDir + "shirts/", AvatarItemType::Shirt, gItems[AvatarItemType::Shirt]);
        scanDirectory(charcustomDir + "pants/", AvatarItemType::Pants, gItems[AvatarItemType::Pants]);
        scanDirectory(charcustomDir + "tshirts/", AvatarItemType::TShirt, gItems[AvatarItemType::TShirt]);
        scanDirectory(charcustomDir + "faces/", AvatarItemType::Face, gItems[AvatarItemType::Face]);
        scanDirectory(charcustomDir + "hats/", AvatarItemType::Hat, gItems[AvatarItemType::Hat]);

        int total = 0;
        for (auto& pair : gItems) {
            total += pair.second.size();
        }
        printf("Avatar items scanned: %d total\n", total);
    }

    std::vector<AvatarItem>& getAvailableItems(AvatarItemType type) {
        return gItems[type];
    }

    bool equipItem(const AvatarItem& item) {
        switch (item.type) {
            case AvatarItemType::Shirt:
                gAvatar.currentShirt = item.id;
                break;
            case AvatarItemType::Pants:
                gAvatar.currentPants = item.id;
                break;
            case AvatarItemType::TShirt:
                gAvatar.currentTShirt = item.id;
                break;
            case AvatarItemType::Face:
                gAvatar.currentFace = item.id;
                break;
            case AvatarItemType::Hat:
                gAvatar.currentHat = item.id;
                break;
            default:
                return false;
        }
        printf("Equipped: %s (%s)\n", item.name.c_str(), item.id.c_str());
        return true;
    }

    void unequipItem(AvatarItemType type) {
        switch (type) {
            case AvatarItemType::Shirt:
                gAvatar.currentShirt.clear();
                break;
            case AvatarItemType::Pants:
                gAvatar.currentPants.clear();
                break;
            case AvatarItemType::TShirt:
                gAvatar.currentTShirt.clear();
                break;
            case AvatarItemType::Face:
                gAvatar.currentFace.clear();
                break;
            case AvatarItemType::Hat:
                gAvatar.currentHat.clear();
                break;
            default:
                break;
        }
    }

    const AvatarItem* getEquippedItem(AvatarItemType type) {
        for (auto& item : gItems[type]) {
            std::string equippedId;
            switch (type) {
                case AvatarItemType::Shirt:   equippedId = gAvatar.currentShirt;   break;
                case AvatarItemType::Pants:   equippedId = gAvatar.currentPants;   break;
                case AvatarItemType::TShirt:  equippedId = gAvatar.currentTShirt;  break;
                case AvatarItemType::Face:    equippedId = gAvatar.currentFace;    break;
                case AvatarItemType::Hat:     equippedId = gAvatar.currentHat;     break;
                default: break;
            }
            if (item.id == equippedId) {
                return &item;
            }
        }
        return nullptr;
    }

    std::string getItemFilePath(const AvatarItem& item) {
        return item.filePath;
    }

    u32 getBrickColorRGBA(BrickColor color) {
        u8 r = 128, g = 128, b = 128; // Default gray

        for (const auto& entry : brickColorTable) {
            if ((int)entry.id == (int)color) {
                r = entry.r;
                g = entry.g;
                b = entry.b;
                break;
            }
        }

        return (0xFF << 24) | (r << 16) | (g << 8) | b;
    }

    // R15 body color setters/getters
    void setR15BodyColor(R15BodyPart part, BrickColor color) {
        int idx = (int)part;
        if (idx >= 0 && idx < (int)R15BodyPart::Count) {
            gAvatar.r15BodyColors[idx] = color;
        }
    }

    BrickColor getR15BodyColor(R15BodyPart part) {
        int idx = (int)part;
        if (idx >= 0 && idx < (int)R15BodyPart::Count) {
            return gAvatar.r15BodyColors[idx];
        }
        return BrickColor::BrightBlue;
    }

    void setCharacterType(CharacterType type) {
        gAvatar.characterType = type;
        if (type == CharacterType::R15) {
            // Initialize R15 colors to defaults
            for (int i = 0; i < (int)R15BodyPart::Count; i++) {
                gAvatar.r15BodyColors[i] = BrickColor::BrightBlue;
            }
            gAvatar.r15BodyColors[(int)R15BodyPart::Head] = BrickColor::BrightYellow;
        }
    }

    CharacterType getCharacterType() {
        return gAvatar.characterType;
    }

    void setR15ModelPath(const std::string& path) {
        gAvatar.r15ModelPath = path;
    }

    const std::string& getR15ModelPath() {
        return gAvatar.r15ModelPath;
    }

    bool saveAvatar() {
        FILE* f = fopen(avatarSavePath.c_str(), "w");
        if (!f) return false;

        fprintf(f, "playerName=%s\n", gAvatar.playerName.c_str());
        fprintf(f, "playerId=%d\n", gAvatar.playerId);

        for (int i = 0; i < (int)BodyPart::Count; i++) {
            char key[32];
            snprintf(key, sizeof(key), "bodyColor.%d", i);
            fprintf(f, "%s=%d\n", key, (int)gAvatar.bodyColors[i]);
        }

        if (!gAvatar.currentShirt.empty())   fprintf(f, "shirt=%s\n", gAvatar.currentShirt.c_str());
        if (!gAvatar.currentPants.empty())   fprintf(f, "pants=%s\n", gAvatar.currentPants.c_str());
        if (!gAvatar.currentTShirt.empty())  fprintf(f, "tshirt=%s\n", gAvatar.currentTShirt.c_str());
        if (!gAvatar.currentFace.empty())    fprintf(f, "face=%s\n", gAvatar.currentFace.c_str());
        if (!gAvatar.currentHat.empty())     fprintf(f, "hat=%s\n", gAvatar.currentHat.c_str());

        fclose(f);
        printf("Avatar saved\n");
        return true;
    }

    bool loadAvatar() {
        FILE* f = fopen(avatarSavePath.c_str(), "r");
        if (!f) return false;

        char line[256];
        while (fgets(line, sizeof(line), f)) {
            char* nl = strchr(line, '\n'); if (nl) *nl = 0;
            char* cr = strchr(line, '\r'); if (cr) *cr = 0;

            char* eq = strchr(line, '=');
            if (!eq) continue;
            *eq = 0;
            char* key = line;
            char* val = eq + 1;

            if (strcmp(key, "playerName") == 0) gAvatar.playerName = val;
            else if (strcmp(key, "playerId") == 0) gAvatar.playerId = atoi(val);
            else if (strncmp(key, "bodyColor.", 10) == 0) {
                int idx = atoi(key + 10);
                if (idx >= 0 && idx < (int)BodyPart::Count) {
                    gAvatar.bodyColors[idx] = (BrickColor)atoi(val);
                }
            }
            else if (strcmp(key, "shirt") == 0) gAvatar.currentShirt = val;
            else if (strcmp(key, "pants") == 0) gAvatar.currentPants = val;
            else if (strcmp(key, "tshirt") == 0) gAvatar.currentTShirt = val;
            else if (strcmp(key, "face") == 0) gAvatar.currentFace = val;
            else if (strcmp(key, "hat") == 0) gAvatar.currentHat = val;
        }

        fclose(f);
        printf("Avatar loaded\n");
        return true;
    }

    void resetAvatar() {
        gAvatar.playerName = "Player";
        gAvatar.playerId = 0;

        gAvatar.bodyColors[(int)BodyPart::Head] = BrickColor::BrightYellow;
        gAvatar.bodyColors[(int)BodyPart::Torso] = BrickColor::BrightBlue;
        gAvatar.bodyColors[(int)BodyPart::LeftArm] = BrickColor::BrightBlue;
        gAvatar.bodyColors[(int)BodyPart::RightArm] = BrickColor::BrightBlue;
        gAvatar.bodyColors[(int)BodyPart::LeftLeg] = BrickColor::BrightBlue;
        gAvatar.bodyColors[(int)BodyPart::RightLeg] = BrickColor::BrightBlue;

        gAvatar.currentShirt.clear();
        gAvatar.currentPants.clear();
        gAvatar.currentTShirt.clear();
        gAvatar.currentFace.clear();
        gAvatar.currentHat.clear();

        printf("Avatar reset to default\n");
    }

    void shutdown() {
        saveAvatar();
        gItems.clear();
        printf("Character editor shutdown\n");
    }

}