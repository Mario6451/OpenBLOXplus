#pragma once

#include <string>
#include <vector>
#include <map>

namespace AudioManager {

    // Sound channel for 3D positional audio
    struct SoundChannel {
        int ndspChannel;
        bool active;
        float x, y, z;          // 3D position
        float volume;           // base volume (0-1)
        float maxDistance;      // max distance before silent
        float referenceDistance;// distance where volume = 1.0
        bool loop;
        bool is3D;              // positional audio enabled
        bool isMusic;           // music channel (not attenuated)
        void *buffer;
        size_t bufferSize;
    };

    // Material enum for collision sounds (matches Roblox Enum.Material)
    // Values from https://create.roblox.com/docs/en-us/reference/engine/enums/Material
    enum class Material {
        Plastic        = 256,   // BasePart only
        SmoothPlastic  = 272,   // BasePart only
        Neon           = 288,   // BasePart only
        Wood           = 512,   // BasePart only
        WoodPlanks     = 528,   // BasePart and Terrain
        Marble         = 784,   // BasePart only
        Basalt         = 788,   // BasePart and Terrain
        Slate          = 800,   // BasePart and Terrain
        CrackedLava    = 804,   // BasePart and Terrain
        Concrete       = 816,   // BasePart and Terrain
        Limestone      = 820,   // BasePart and Terrain
        Granite        = 832,   // BasePart only
        Pavement       = 836,   // BasePart and Terrain
        Brick          = 848,   // BasePart and Terrain
        Pebble         = 864,   // BasePart only
        Cobblestone    = 880,   // BasePart and Terrain
        Rock           = 896,   // BasePart and Terrain
        Sandstone      = 912,   // BasePart and Terrain
        CorrodedMetal  = 1040,  // BasePart only
        DiamondPlate   = 1056,  // BasePart only
        Foil           = 1072,  // BasePart only
        Metal          = 1088,  // BasePart only
        Glass          = 1200,  // BasePart and Terrain
        Grass          = 1280,  // BasePart and Terrain
        LeafyGrass     = 1284,  // BasePart and Terrain
        Sand           = 1296,  // BasePart and Terrain
        Fabric         = 1312,  // BasePart only
        Snow           = 1328,  // BasePart and Terrain
        Mud            = 1344,  // BasePart and Terrain
        Ground         = 1360,  // BasePart and Terrain
        Asphalt        = 1376,  // BasePart and Terrain
        Salt           = 1392,  // BasePart and Terrain
        Ice            = 1536,  // BasePart and Terrain
        Glacier        = 1552,  // BasePart and Terrain
        ForceField     = 1584,  // MeshPart with UVs
        Air            = 1792,  // Terrain only
        Water          = 2048,  // Terrain only
    };

    // Initialize audio system
    bool initAudio();

    // Shutdown and cleanup
    void shutdownAudio();

    // Set master volume (0-100)
    void setMasterVolume(int volume);

    // Set music volume (0-100)
    void setMusicVolume(int volume);

    // Set SFX volume (0-100)
    void setSfxVolume(int volume);

    // Get current volumes
    int getMasterVolume();
    int getMusicVolume();
    int getSfxVolume();

    // Enable/disable collision sounds
    void setCollisionSounds(bool enable);
    bool getCollisionSounds();

    // Enable/disable spatial audio
    void setSpatialAudio(bool enable);
    bool getSpatialAudio();

    // Play sound from file path (wav, mp3, ogg)
    // Returns channel index or -1 on failure
    int playSoundFile(const std::string &path, bool loop = false);

    // Play sound at 3D position (positional audio)
    int playSound3D(const std::string &path, float x, float y, float z, bool loop = false);

    // Update 3D audio based on listener position (call every frame)
    void update3DAudio(float listenerX, float listenerY, float listenerZ, float dt);
    
    // Update tracker/MIDI audio playback (call every frame)
    void updateAudio(float dt);

    // Stop all sounds
    void stopAll();

    // Stop specific channel
    void stopChannel(int channel);

    // Set channel position
    void setChannelPosition(int channel, float x, float y, float z);

    // Play collision sound based on material pair
    void playCollisionSound(Material mat1, Material mat2, float x, float y, float z);

    // Register a custom collision sound for a material pair
    void registerCollisionSound(Material mat1, Material mat2, const std::string &path);

    // Load sound file into cache
    void *cacheSound(const std::string &path);

    // Get cached sound
    void *getCachedSound(const std::string &path);

    // Clear sound cache
    void clearCache();

    // Get active channel count
    int getActiveChannelCount();

    // Get max channels
    int getMaxChannels();

    // Load default collision sounds from content
    void loadDefaultCollisionSounds();

    // Convert material enum to name
    const char* materialName(Material mat);

    // Convert name to material enum
    Material materialFromString(const std::string &name);
    
    // Load custom soundfont from path (returns true on success)
    bool loadCustomSoundFont(const std::string& path);
    
    // Check if a custom soundfont is currently loaded
    bool isCustomSoundFontLoaded();
}
