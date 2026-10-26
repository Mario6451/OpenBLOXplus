#include "audio_manager.h"
#include "settings.h"
#include "content_system.h"
#include <3ds.h>
#include <3ds/ndsp.h>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ogg_decoder.h"
#include "../../ThirdParty/minimp3.h"

namespace AudioManager {

namespace {
    static const int MAX_CHANNELS = 16;
    static std::vector<SoundChannel> gChannels;
    static std::map<std::string, void*> gSoundCache;
    static std::map<std::string, std::string> gCollisionSounds;
    
    static int gMasterVolume = 80;
    static int gMusicVolume = 80;
    static int gSfxVolume = 80;
    static bool gCollisionSoundsEnabled = true;
    static bool gSpatialAudioEnabled = true;
    static std::string gCustomSoundFontPath = "";
    static bool gCustomSoundFontLoaded = false;
    
    // MaxMod state disabled - library not available
    static bool gMaxModInitialized = false;
    
    struct AllocBuf { void* ptr; size_t size; };
    static std::vector<AllocBuf> gAllocs;

    static void short_to_float32(const short *in, float *out, int samples) {
        for (int i = 0; i < samples; ++i) out[i] = in[i] / 32768.0f;
    }

    static float applyVolume(float baseVolume) {
        return baseVolume * (gMasterVolume / 100.0f);
    }
}

bool initAudio() {
    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    
    // MaxMod init disabled - libmaxmod not available for 3DS build
    
    // Ensure soundfont directory exists
    mkdir("sdmc:/OpenBLOX/shareddata", 0755);
    mkdir("sdmc:/OpenBLOX/shareddata/soundfonts", 0755);
    
    gChannels.resize(MAX_CHANNELS);
    for (int i = 0; i < MAX_CHANNELS; i++) {
        gChannels[i].ndspChannel = i;
        gChannels[i].active = false;
        gChannels[i].buffer = nullptr;
        gChannels[i].is3D = false;
        gChannels[i].isMusic = false;
        gChannels[i].volume = 1.0f;
        gChannels[i].referenceDistance = 5.0f;
        gChannels[i].maxDistance = 50.0f;
    }
    // Apply settings
    gMasterVolume = Settings::get().sound.masterVolume;
    gMusicVolume = Settings::get().sound.musicVolume;
    gSfxVolume = Settings::get().sound.sfxVolume;
    gCollisionSoundsEnabled = Settings::get().sound.collisionSounds;
    gSpatialAudioEnabled = Settings::get().sound.spatialAudio;
    
    // Load custom soundfont if configured
    if (!Settings::get().sound.customSoundFont.empty()) {
        loadCustomSoundFont(Settings::get().sound.customSoundFont);
    }
    
    loadDefaultCollisionSounds();
    return true;
}

void shutdownAudio() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gChannels[i].buffer) {
            linearFree(gChannels[i].buffer);
            gChannels[i].buffer = nullptr;
        }
    }
    
    // MaxMod/libxmp shutdown disabled - libraries not available
    gMaxModInitialized = false;
    
    gChannels.clear();
    for (auto &p : gAllocs) if (p.ptr) linearFree(p.ptr);
    gAllocs.clear();
    clearCache();
    ndspExit();
}

void setMasterVolume(int volume) { gMasterVolume = volume; }
void setMusicVolume(int volume) { gMusicVolume = volume; }
void setSfxVolume(int volume) { gSfxVolume = volume; }
int getMasterVolume() { return gMasterVolume; }
int getMusicVolume() { return gMusicVolume; }
int getSfxVolume() { return gSfxVolume; }

void setCollisionSounds(bool enable) { gCollisionSoundsEnabled = enable; }
bool getCollisionSounds() { return gCollisionSoundsEnabled; }
void setSpatialAudio(bool enable) { gSpatialAudioEnabled = enable; }
bool getSpatialAudio() { return gSpatialAudioEnabled; }

static int findFreeChannel() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!gChannels[i].active) return i;
    }
    return -1;
}

static bool decodeWav(const std::string &path, short **pcm, int *channels, int *sampleRate, int *samples) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    
    // Parse RIFF/WAV header
    char header[4];
    if (fread(header, 1, 4, f) != 4 || memcmp(header, "RIFF", 4) != 0) { fclose(f); return false; }
    
    // Read chunk size (skip it)
    unsigned int riffSize;
    if (fread(&riffSize, 4, 1, f) != 1) { fclose(f); return false; }
    
    // Read "WAVE" descriptor
    if (fread(header, 1, 4, f) != 4 || memcmp(header, "WAVE", 4) != 0) { fclose(f); return false; }
    
    // Seek to find "fmt " chunk - it should be at offset 12
    if (fread(header, 1, 4, f) != 4 || memcmp(header, "fmt ", 4) != 0) { fclose(f); return false; }
    
    // Read fmt chunk size (4 bytes after "fmt ")
    unsigned int fmtSize;
    if (fread(&fmtSize, 4, 1, f) != 1) { fclose(f); return false; }
    
    // Read channels (at offset 22 from file start, but we're already past "fmt " header)
    // fmt chunk: size(4) + audioFormat(2) + numChannels(2) + sampleRate(4)
    uint16_t fmtAudioFormat;
    if (fread(&fmtAudioFormat, 2, 1, f) != 1) { fclose(f); return false; }
    if (fmtAudioFormat != 1) { fclose(f); return false; }  // Only support PCM
    
    *channels = 1;
    uint16_t fmtChannels;
    if (fread(&fmtChannels, 2, 1, f) == 1) *channels = fmtChannels;
    
    if (fread(sampleRate, 4, 1, f) != 1) { fclose(f); return false; }
    
    // Seek back to find "data" chunk from offset 12
    fseek(f, 12, SEEK_SET);
    unsigned int dataSize = 0;
    bool foundData = false;
    while (!feof(f)) {
        if (fread(header, 1, 4, f) != 4) break;
        if (memcmp(header, "data", 4) == 0) {
            fread(&dataSize, 4, 1, f);
            foundData = true;
            break;
        }
        unsigned int skip;
        if (fread(&skip, 4, 1, f) != 1) break;
        fseek(f, skip, SEEK_CUR);
    }
    if (!foundData) { fclose(f); return false; }
    
    *samples = (dataSize / 2) / *channels;  /* Per-channel sample frames */
    *pcm = (short*)malloc(dataSize);
    if (!*pcm) { fclose(f); return false; }
    fread(*pcm, dataSize, 1, f);
    fclose(f);
    return true;
}

static bool decodeMp3(const std::string &path, short **pcm, int *channels, int *sampleRate, int *samples) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *fileData = (unsigned char*)malloc(fileSize);
    if (!fileData) { fclose(f); return false; }
    fread(fileData, 1, fileSize, f);
    fclose(f);
    
    mp3dec_t dec;
    mp3dec_frame_info_t frame;
    mp3dec_init(&dec);
    
    // Allocate output buffer (approximate)
    int totalSamples = 0;
    int maxSamples = fileSize / 2;
    short *buffer = (short*)malloc(maxSamples * sizeof(short));
    if (!buffer) { free(fileData); return false; }
    
    const unsigned char *offset = fileData;
    while (offset < fileData + fileSize) {
        size_t frameSize = mp3dec_decode_frame(&dec, offset, (int)(fileData + fileSize - offset), nullptr, &frame);
        if (frameSize <= 0) break;
        int channels = frame.channels;
        int samplesPerChannel = frame.frame_bytes / (channels * 2);
        if (totalSamples + samplesPerChannel * channels > maxSamples) break;

        short *frameBuf = buffer + totalSamples;
        mp3dec_decode_frame(&dec, offset, (int)frameSize, frameBuf, &frame);

        totalSamples += samplesPerChannel * channels;
        offset += frameSize;
    }
    
    *pcm = buffer;
    *channels = frame.channels;
    *sampleRate = frame.hz;
    *samples = totalSamples / frame.channels;  /* Per-channel sample frames */
    free(fileData);
    return true;
}

int playSoundFile(const std::string &path, bool loop) {
    int ch = findFreeChannel();
    if (ch < 0) return -1;
    
    short *pcm = nullptr;
    int channels = 0, sampleRate = 0, samples = 0;
    
    // Try decode based on extension
    std::string lowerPath = path;
    for (auto &c : lowerPath) c = tolower(c);
    
    // Handle tracker files (MOD/S3M/XM/IT) - disabled, MaxMod not available
    /*
    if ((lowerPath.find(".mod") != std::string::npos) ||
        (lowerPath.find(".s3m") != std::string::npos) ||
        (lowerPath.find(".xm") != std::string::npos) ||
        (lowerPath.find(".it") != std::string::npos)) {
        if (gCurrentModule) {
            mmStop();
        }
        
        mmModule* mod = mmLoadFile(path.c_str());
        if (mod) {
            gCurrentModule = mod;
            mmStartSong(0);
            if (loop) mod->mmFlags |= MMFLAG_REPEAT;
            else mod->mmFlags &= ~MMFLAG_REPEAT;
            return 0;
        }
        return -1;
    }
    */
    
    // Handle MIDI files - disabled, libxmp not available
    /*
    if (lowerPath.find(".mid") != std::string::npos ||
        lowerPath.find(".midi") != std::string::npos) {
        if (gXmpContext) {
            xmp_end_player(gXmpContext);
        }
        if (gCurrentXmpModule) {
            xmp_free_module(gCurrentXmpModule);
            gCurrentXmpModule = nullptr;
        }
        
        gCurrentXmpModule = xmp_load_module(path.c_str());
        if (gCurrentXmpModule) {
            gXmpContext = xmp_create_context();
            if (gXmpContext) {
                if (xmp_init_player(gXmpContext, gCurrentXmpModule) == 0) {
                    xmp_start_player(gXmpContext, 44100, 0);
                    return 0;
                }
            }
            xmp_free_module(gCurrentXmpModule);
            gCurrentXmpModule = nullptr;
        }
        return -1;
    }
    */
    
    bool decoded = false;
    if (lowerPath.find(".wav") != std::string::npos) {
        decoded = decodeWav(path, &pcm, &channels, &sampleRate, &samples);
    } else if (lowerPath.find(".ogg") != std::string::npos || lowerPath.find(".oga") != std::string::npos) {
        decoded = decode_ogg_to_pcm(path, &channels, &sampleRate, &pcm, &samples);
    } else if (lowerPath.find(".mp3") != std::string::npos) {
        decoded = decodeMp3(path, &pcm, &channels, &sampleRate, &samples);
    }
    
    if (!decoded || !pcm || samples <= 0) return -1;
    
    int totalSamples = samples * channels;
    float *buf = (float*)linearAlloc(totalSamples * sizeof(float));
    if (!buf) { free(pcm); return -1; }
    short_to_float32(pcm, buf, totalSamples);
    free(pcm);
    
    ndspWaveBuf waveBuf;
    memset(&waveBuf, 0, sizeof(waveBuf));
    waveBuf.data_vaddr = buf;
    waveBuf.nsamples = samples;
    waveBuf.looping = loop ? 1 : 0;
    ndspChnWaveBufAdd(ch, &waveBuf);
    
    SoundChannel &s = gChannels[ch];
    s.active = true;
    s.buffer = buf;
    s.bufferSize = totalSamples * sizeof(float);
    s.loop = loop;
    s.isMusic = (lowerPath.find("/music/") != std::string::npos);
    s.volume = s.isMusic ? (gMusicVolume / 100.0f) : (gSfxVolume / 100.0f);
    
    gAllocs.push_back({buf, totalSamples * sizeof(float)});
    return ch;
}

int playSound3D(const std::string &path, float x, float y, float z, bool loop) {
    int ch = playSoundFile(path, loop);
    if (ch >= 0) {
        gChannels[ch].is3D = gSpatialAudioEnabled;
        gChannels[ch].x = x;
        gChannels[ch].y = y;
        gChannels[ch].z = z;
    }
    return ch;
}

void updateAudio(float dt) {
    (void)dt;
    // MaxMod/libxmp update disabled - libraries not available
}

void update3DAudio(float listenerX, float listenerY, float listenerZ, float dt) {
    (void)dt;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        SoundChannel &s = gChannels[i];
        if (!s.active || !s.is3D || s.isMusic) continue;
        
        float dx = s.x - listenerX;
        float dy = s.y - listenerY;
        float dz = s.z - listenerZ;
        float distance = sqrtf(dx * dx + dy * dy + dz * dz);
        
        // Inverse distance attenuation
        float attenuation;
        if (distance < s.referenceDistance) {
            attenuation = 1.0f;
        } else {
            attenuation = s.referenceDistance / distance;
            if (distance > s.maxDistance) attenuation = 0.0f;
        }
        s.volume = attenuation * applyVolume(1.0f);
    }
}

void stopAll() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        ndspChnReset(i);
        if (gChannels[i].buffer) {
            linearFree(gChannels[i].buffer);
            gChannels[i].buffer = nullptr;
        }
        gChannels[i].active = false;
    }
    gAllocs.clear();
}

void stopChannel(int ch) {
    if (ch < 0 || ch >= MAX_CHANNELS) return;
    ndspChnReset(ch);
    if (gChannels[ch].buffer) {
        linearFree(gChannels[ch].buffer);
        gChannels[ch].buffer = nullptr;
    }
    gChannels[ch].active = false;
}

void setChannelPosition(int ch, float x, float y, float z) {
    if (ch < 0 || ch >= MAX_CHANNELS) return;
    gChannels[ch].x = x;
    gChannels[ch].y = y;
    gChannels[ch].z = z;
}

void playCollisionSound(Material mat1, Material mat2, float x, float y, float z) {
    if (!gCollisionSoundsEnabled) return;
    
    // Normalize pair (lower material first)
    Material first = mat1, second = mat2;
    if (mat2 < mat1) { first = mat2; second = mat1; }
    
    std::string key = std::string(materialName(first)) + "_" + materialName(second);
    auto it = gCollisionSounds.find(key);
    if (it != gCollisionSounds.end()) {
        playSound3D(it->second, x, y, z);
    }
}

void registerCollisionSound(Material mat1, Material mat2, const std::string &path) {
    Material first = mat1, second = mat2;
    if (mat2 < mat1) { first = mat2; second = mat1; }
    std::string key = std::string(materialName(first)) + "_" + materialName(second);
    gCollisionSounds[key] = path;
}

void *cacheSound(const std::string &path) {
    auto it = gSoundCache.find(path);
    if (it != gSoundCache.end()) return it->second;
    
    // Try to load and cache
    int ch = playSoundFile(path, false);
    if (ch >= 0 && gChannels[ch].buffer) {
        gSoundCache[path] = gChannels[ch].buffer;
        return gChannels[ch].buffer;
    }
    return nullptr;
}

void *getCachedSound(const std::string &path) {
    auto it = gSoundCache.find(path);
    if (it != gSoundCache.end()) return it->second;
    return nullptr;
}

void clearCache() {
    gSoundCache.clear();
}

int getActiveChannelCount() {
    int count = 0;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (gChannels[i].active) count++;
    }
    return count;
}

int getMaxChannels() { return MAX_CHANNELS; }

void loadDefaultCollisionSounds() {
    // Map material pairs to collision sounds from content folder sounds/
    // Located in sdmc:/OpenBLOX/content2012/sounds/
    
    std::string basePath = "sdmc:/OpenBLOX/content2012/sounds/";
    
    // Helper: register collision sound with proper lowercase filename
    auto reg = [&basePath](Material m1, Material m2, const std::string& fname) {
        std::string path = basePath + fname + ".mp3";
        registerCollisionSound(m1, m2, path);
    };
    
    // ---- Official material collision pairs (Roblox Enum.Material) ----
    
    // Grass pairs
    reg(Material::Grass, Material::Grass,     "grass");
    reg(Material::Grass, Material::Water,     "splat");
    reg(Material::Grass, Material::Sand,      "grass");
    
    // Sand pairs
    reg(Material::Sand, Material::Sand,       "sand");
    reg(Material::Sand, Material::Water,      "splat");
    
    // Ice-based materials
    reg(Material::Ice, Material::Ice,         "ice");
    reg(Material::Ice, Material::Grass,       "icegrass");
    reg(Material::Ice, Material::Sand,        "icestone");
    
    // Stone/rock based materials
    reg(Material::Rock, Material::Rock,       "stone");
    reg(Material::Granite, Material::Granite, "stone");
    reg(Material::Marble, Material::Marble,   "stone");
    reg(Material::Concrete, Material::Concrete, "stone");
    reg(Material::Asphalt, Material::Asphalt, "stone");
    reg(Material::Slate, Material::Slate,     "stone");
    reg(Material::Basalt, Material::Basalt,   "stone");
    reg(Material::Brick, Material::Brick,     "stone");
    reg(Material::Limestone, Material::Limestone, "stone");
    reg(Material::Pavement, Material::Pavement, "stone");
    reg(Material::Pebble, Material::Pebble,   "stone");
    reg(Material::Cobblestone, Material::Cobblestone, "stone");
    reg(Material::Sandstone, Material::Sandstone, "stone");
    reg(Material::CrackedLava, Material::CrackedLava, "stone");
    
    // Wood-based
    reg(Material::Wood, Material::Wood,       "woodwood");
    reg(Material::WoodPlanks, Material::WoodPlanks, "woodwood");
    
    // Neon
    reg(Material::Neon, Material::Neon,       "plasticplastic");
    
    // Fabric/soft
    reg(Material::Fabric, Material::Fabric,   "woodwood");
    
    // Mud
    reg(Material::Mud, Material::Mud,         "grass");
    
    // Water splash
    reg(Material::Water, Material::Water,     "splat");
    
    // LeafyGrass
    reg(Material::LeafyGrass, Material::LeafyGrass, "grass");
    
    // Metal
    reg(Material::Metal, Material::Metal,     "metal");
    reg(Material::DiamondPlate, Material::DiamondPlate, "metal");
    reg(Material::CorrodedMetal, Material::CorrodedMetal, "metal");
    reg(Material::Foil, Material::Foil,       "metal");
    
    // Plastic
    reg(Material::Plastic, Material::Plastic, "plasticplastic");
    reg(Material::SmoothPlastic, Material::SmoothPlastic, "plasticplastic");
    
    // Snow
    reg(Material::Snow, Material::Snow,       "ice");
    
    // Fallback: register all unregistered pairs to collide.mp3
    std::string fallbackPath = basePath + "collide.mp3";
    // Use explicit array of all materials instead of iterating raw enum values
    const Material allMaterials[] = {
        Material::Plastic, Material::SmoothPlastic, Material::Neon, Material::Wood,
        Material::WoodPlanks, Material::Marble, Material::Basalt, Material::Slate,
        Material::CrackedLava, Material::Concrete, Material::Limestone, Material::Granite,
        Material::Pavement, Material::Brick, Material::Pebble, Material::Cobblestone,
        Material::Rock, Material::Sandstone, Material::CorrodedMetal, Material::DiamondPlate,
        Material::Foil, Material::Metal, Material::Glass, Material::Grass,
        Material::LeafyGrass, Material::Sand, Material::Fabric, Material::Snow,
        Material::Mud, Material::Ground, Material::Asphalt, Material::Salt,
        Material::Ice, Material::Glacier, Material::ForceField, Material::Air, Material::Water
    };
    const int numMaterials = sizeof(allMaterials) / sizeof(allMaterials[0]);
    for (int i = 0; i < numMaterials; i++) {
        for (int j = i; j < numMaterials; j++) {
            Material m1 = allMaterials[i], m2 = allMaterials[j];
            std::string key = std::string(materialName(m1)) + "_" + materialName(m2);
            if (gCollisionSounds.find(key) == gCollisionSounds.end()) {
                registerCollisionSound(m1, m2, fallbackPath);
            }
        }
    }
}

bool loadCustomSoundFont(const std::string& path) {
    if (path.empty()) {
        gCustomSoundFontLoaded = false;
        return false;
    }
    
    // Validate .sf2 extension
    std::string lowerPath = path;
    for (auto& c : lowerPath) c = tolower(c);
    if (lowerPath.length() < 4 || lowerPath.substr(lowerPath.length() - 4) != ".sf2") {
        printf("[AudioManager] Invalid soundfont file: %s (must be .sf2)\n", path.c_str());
        return false;
    }
    
    // Ensure soundfonts directory exists
    mkdir("sdmc:/OpenBLOX/shareddata", 0755);
    mkdir("sdmc:/OpenBLOX/shareddata/soundfonts", 0755);
    
    // Check if file exists
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        printf("[AudioManager] Soundfont not found: %s\n", path.c_str());
        return false;
    }
    fclose(f);
    
    // Soundfont loaded - path stored for MIDI playback
    gCustomSoundFontPath = path;
    gCustomSoundFontLoaded = true;
    printf("[AudioManager] Custom soundfont loaded: %s\n", path.c_str());
    return true;
}

bool isCustomSoundFontLoaded() {
    return gCustomSoundFontLoaded;
}

const char* materialName(Material mat) {
    switch (mat) {
        case Material::Plastic:       return "plastic";
        case Material::SmoothPlastic: return "smoothplastic";
        case Material::Neon:          return "neon";
        case Material::Wood:          return "wood";
        case Material::WoodPlanks:    return "wood";
        case Material::Marble:        return "stone";
        case Material::Basalt:        return "stone";
        case Material::Slate:         return "stone";
        case Material::CrackedLava:   return "stone";
        case Material::Concrete:      return "stone";
        case Material::Limestone:     return "stone";
        case Material::Granite:       return "stone";
        case Material::Pavement:      return "stone";
        case Material::Brick:         return "stone";
        case Material::Pebble:        return "stone";
        case Material::Cobblestone:   return "stone";
        case Material::Rock:          return "stone";
        case Material::Sandstone:     return "stone";
        case Material::CorrodedMetal: return "metal";
        case Material::DiamondPlate:  return "metal";
        case Material::Foil:          return "metal";
        case Material::Metal:         return "metal";
        case Material::Glass:         return "glass";
        case Material::Grass:         return "grass";
        case Material::LeafyGrass:    return "grass";
        case Material::Sand:          return "sand";
        case Material::Fabric:        return "fabric";
        case Material::Snow:          return "ice";
        case Material::Mud:           return "grass";
        case Material::Ground:        return "ground";
        case Material::Asphalt:       return "stone";
        case Material::Salt:          return "sand";
        case Material::Ice:           return "ice";
        case Material::Glacier:       return "ice";
        case Material::ForceField:    return "forcefield";
        case Material::Air:           return "air";
        case Material::Water:         return "water";
        default:                      return "plastic";
    }
}

Material materialFromString(const std::string &name) {
    static const struct { const char *nm; Material m; } names[] = {
        {"plastic",       Material::Plastic},
        {"smoothplastic", Material::SmoothPlastic},
        {"neon",          Material::Neon},
        {"wood",          Material::Wood},
        {"woodplanks",    Material::WoodPlanks},
        {"marble",        Material::Marble},
        {"basalt",        Material::Basalt},
        {"slate",         Material::Slate},
        {"crackedlava",   Material::CrackedLava},
        {"concrete",      Material::Concrete},
        {"limestone",     Material::Limestone},
        {"granite",       Material::Granite},
        {"pavement",      Material::Pavement},
        {"brick",         Material::Brick},
        {"pebble",        Material::Pebble},
        {"cobblestone",   Material::Cobblestone},
        {"rock",          Material::Rock},
        {"sandstone",     Material::Sandstone},
        {"corrodedmetal", Material::CorrodedMetal},
        {"diamondplate",  Material::DiamondPlate},
        {"foil",          Material::Foil},
        {"metal",         Material::Metal},
        {"glass",         Material::Glass},
        {"grass",         Material::Grass},
        {"leafygrass",    Material::LeafyGrass},
        {"sand",          Material::Sand},
        {"fabric",        Material::Fabric},
        {"snow",          Material::Snow},
        {"mud",           Material::Mud},
        {"ground",        Material::Ground},
        {"asphalt",       Material::Asphalt},
        {"salt",          Material::Salt},
        {"ice",           Material::Ice},
        {"glacier",       Material::Glacier},
        {"forcefield",    Material::ForceField},
        {"air",           Material::Air},
        {"water",         Material::Water},
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (strcasecmp(name.c_str(), names[i].nm) == 0) return names[i].m;
    }
    return Material::Concrete;
}
}
