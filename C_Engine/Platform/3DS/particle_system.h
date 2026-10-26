#pragma once
#include <string>
#include <vector>
#include <map>
#include <3ds.h>

/*
 * Classic Roblox Particle System
 * Parses .particle and .material files from content folders
 * and renders billboarded particles via Citro3D.
 */

struct ParticleEmitterConfig {
    std::string name;
    std::string materialName;

    // Geometry
    float particleWidth = 5.0f;
    float particleHeight = 5.0f;
    int quota = 100;

    // Billboard
    bool billboardType = true;     // point
    bool sorted = false;
    bool localSpace = false;

    // Emitter
    std::string emitterType = "Point"; // Point, Ellipsoid
    float angle = 360.0f;
    float emissionRate = 50.0f;
    float velocityMin = 5.0f;
    float velocityMax = 15.0f;
    float ttlMin = 1.0f;
    float ttlMax = 2.0f;
    float duration = 0.0f;        // 0 = infinite
    float position[3] = {0, 0, 0};
    float direction[3] = {0, 1, 0};
    float size[3] = {0, 0, 0};    // Ellipsoid dimensions
    float colorStart[4] = {1, 1, 1, 1};
    float colorEnd[4] = {1, 1, 1, 1};

    // Affectors
    float colorFadeR = 0, colorFadeG = 0, colorFadeB = 0, colorFadeA = 0;
    float scaleRate = 0.0f;
    float rotationSpeedMin = 0, rotationSpeedMax = 0;
    float rotationMin = 0, rotationMax = 0;
};

struct MaterialConfig {
    std::string name;
    bool lighting = false;
    bool depthWrite = true;
    bool alphaBlend = false;
    std::string texturePath;
    std::string textureAddressMode = "wrap";
};

struct Particle {
    float pos[3];
    float vel[3];
    float color[4];
    float size;
    float rotation;
    float rotSpeed;
    float life;
    float maxLife;
    bool active;
};

class ParticleSystem {
public:
    static void init();
    static void shutdown();

    // Load particle systems from a .particle file
    static bool loadParticleFile(const std::string& path, std::string contentFolder = "");
    // Load materials from a .material file
    static bool loadMaterialFile(const std::string& path, std::string contentFolder = "");

    // Spawn a particle system instance
    static int spawn(const std::string& systemName, float pos[3]);
    // Update all active particle systems
    static void update(float dt);
    // Render all active particles
    static void render();

    // Stop a specific particle instance
    static void stop(int instanceId);
    // Stop all
    static void stopAll();

    // Get material texture path by name
    static const std::string& getMaterialTexture(const std::string& materialName);

private:
    struct ParticleInstance {
        int id;
        std::string systemName;
        float pos[3];
        float elapsed;
        bool active;
        std::vector<Particle> particles;
    };

    static std::map<std::string, ParticleEmitterConfig> emitters;
    static std::map<std::string, MaterialConfig> materials;
    static std::vector<ParticleInstance> instances;
    static int nextInstanceId;

    static void updateInstance(ParticleInstance& inst, const ParticleEmitterConfig& cfg, float dt);
    static void emitParticles(ParticleInstance& inst, const ParticleEmitterConfig& cfg, float dt);
};