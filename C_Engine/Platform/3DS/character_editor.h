#pragma once

#include <string>
#include <vector>
#include <map>
#include <3ds.h>

// Character Editor & Avatar Locker for 3DS OpenBLOX
// Allows customization of character appearance with body colors,
// clothing, faces, and hats from shareddata/charcustom

namespace CharacterEditor {

    // BrickColor enum for Roblox color palette
    enum class BrickColor {
        BrightBlue = 1,
        LightBlue = 2,
        BrightGreen = 3,
        BrightYellow = 4,
        BrightRed = 5,
        DarkNoyellow = 6,
        MediumBlue = 7,
        MediumGreen = 8,
        Black = 9,
        DarkGray = 10,
        Gray = 11,
        LightYellow = 12,
        DarkRed = 13,
        DarkBlue = 14,
        DarkGreen = 15,
        BrightViolet = 16,
        DarkOrange = 17,
        LightOrange = 18,
        Maroon = 19,
        NavyBlue = 20,
        PaleOak = 21,
        BrightBluishGreen = 22,
        BrickYellow = 25,
        LightRed = 26,
        LightBluishGreen = 27,
        MediumRed = 28,
        LightOrange2 = 29,
        BrightPurple = 30,
        BrightViolet2 = 31,
        WarmYellow = 32,
        LightBlue2 = 33,
        LightGray = 34,
        Cobalt = 35,
        DeepOrange = 36,
        DarkPurple = 37,
        Brown = 38,
        DarkCyan = 39,
        Blush = 40,
        GreenCyan = 45,
        DarkBrown = 46,
        YellowishOrange = 47,
        OldYellow = 48,
        PastelBlue = 49,
        PastelGreen = 50,
        PastelViolet = 51,
        Lilac = 52,
        Pink = 53,
        BabyBlue = 54,
        LightPink = 55,
        PastelYellow = 56,
        PaleMagenta = 57,
        BrightBluishGreen2 = 58,
        OliveYellow = 59,
        MossGreen = 60,
        RedOrange = 61,
        ReddishOrange = 62,
        YellowishGreen = 63,
        EarthOrange = 64,
        Orange = 65,
        Olive = 66,
        Yellow = 67,
        Violet = 68,
        ForestGreen = 69,
        YellowGreen = 70,
        BlueGray = 71,
        DarkNoyellow2 = 72,
        PastelBrown = 73,
        DarkMint = 74,
        DarkMossGray = 75,
        DarkPineGreen = 76,
        LightMint = 77,
        MediumGray = 78,
        Red = 79,
        DarkBlueGray = 80,
        Count
    };

    // Character body part for colors (R6)
    enum class BodyPart {
        Head,
        Torso,
        LeftArm,
        RightArm,
        LeftLeg,
        RightLeg,
        Count
    };

    // R15 Character body parts
    enum class R15BodyPart {
        Head,
        Neck,
        LeftShoulder,
        RightShoulder,
        LeftUpperArm,
        RightUpperArm,
        LeftLowerArm,
        RightLowerArm,
        LeftHand,
        RightHand,
        UpperTorso,
        LowerTorso,
        LeftHip,
        RightHip,
        LeftUpperLeg,
        RightUpperLeg,
        LeftLowerLeg,
        RightLowerLeg,
        LeftFoot,
        RightFoot,
        Count
    };

    // Character type
    enum class CharacterType {
        R6,     // Classic 6-part character
        R15     // Modern 15-part character
    };

    // Avatar item types
    enum class AvatarItemType {
        Shirt,
        Pants,
        TShirt,
        Face,
        Hat,
        Count
    };

    // Avatar item entry (from shareddata/charcustom)
    struct AvatarItem {
        std::string id;
        std::string name;
        std::string filePath;
        AvatarItemType type;
        bool isEnabled;
    };

    // Character avatar settings
    struct CharacterAvatar {
        std::string playerName;
        int playerId;

        // Character model type
        CharacterType characterType = CharacterType::R6;

        // Body colors (R6: 6 parts)
        BrickColor bodyColors[(int)BodyPart::Count] = {
            BrickColor::BrightYellow,  // Head
            BrickColor::BrightBlue,    // Torso
            BrickColor::BrightBlue,    // LeftArm
            BrickColor::BrightBlue,    // RightArm
            BrickColor::BrightBlue,    // LeftLeg
            BrickColor::BrightBlue     // RightLeg
        };

        // Body colors (R15: 21 parts)
        BrickColor r15BodyColors[(int)R15BodyPart::Count];

        // Equipped items
        std::string currentShirt;
        std::string currentPants;
        std::string currentTShirt;
        std::string currentFace;
        std::string currentHat;

        // R15 model path (e.g., "characterR15.rbxm")
        std::string r15ModelPath;
    };

    // Initialize character editor
    void init();

    // Get current character avatar
    CharacterAvatar& getAvatar();

    // Set player name
    void setPlayerName(const std::string& name);

    // Set player ID
    void setPlayerId(int id);

    // Set body color
    void setBodyColor(BodyPart part, BrickColor color);

    // Get body color
    BrickColor getBodyColor(BodyPart part);

    // Scan and load available avatar items from shareddata/charcustom
    void scanAvatarItems();

    // Get available items of a specific type
    std::vector<AvatarItem>& getAvailableItems(AvatarItemType type);

    // Equip an avatar item
    bool equipItem(const AvatarItem& item);

    // Unequip an item type
    void unequipItem(AvatarItemType type);

    // Get equipped item of a type
    const AvatarItem* getEquippedItem(AvatarItemType type);

    // Get item file path
    std::string getItemFilePath(const AvatarItem& item);

    // Get brick color RGBA from BrickColor enum
    u32 getBrickColorRGBA(BrickColor color);

    // R15 body color setters/getters
    void setR15BodyColor(R15BodyPart part, BrickColor color);
    BrickColor getR15BodyColor(R15BodyPart part);

    // Set character type (R6 or R15)
    void setCharacterType(CharacterType type);
    CharacterType getCharacterType();

    // Set R15 model path
    void setR15ModelPath(const std::string& path);
    const std::string& getR15ModelPath();

    // Save character settings
    bool saveAvatar();

    // Load character settings
    bool loadAvatar();

    // Reset to default avatar
    void resetAvatar();

    // Shutdown

    void shutdown();
}
