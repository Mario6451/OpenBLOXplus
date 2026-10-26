#pragma once

#include <vector>
#include <string>
#include <3ds.h>
#include <citro3d.h>

namespace ParticleSystem {

    // Particle types
    enum class ParticleType {
        Fire,
        Explosion,
        Smoke,
        Sparkles,
        ForceFieldBeam,
        ForceFieldRadial,
        BeaconGlow,
        BeaconTrail
    };

    // Quality levels (matches settings)
    enum class ParticleQuality {
        Off = 0,
        Low = 1,
        Medium = 2,
        High = 3
    };

    // Single particle
    struct Particle {
        float x, y, z;          // position
        float vx, vy, vz;       // velocity
        float r, g, b, a;       // color
        float size;             // size
        float lifetime;         // remaining lifetime
        float maxLifetime;      // max lifetime
        float rotation;         // rotation angle
        float spin;             // rotation speed
    };

    // Particle emitter
    struct Emitter {
        std::string name;
        ParticleType type;
        float x, y, z;          // emitter position
        int maxParticles;       // max active particles
        float spawnRate;        // particles per second
        float emissionTimer;    // timer for spawning
        float particleLifetime; // max particle lifetime
        float startSize;        // initial size
        float endSize;          // final size
        float speed;            // emission speed
        float spread;           // angular spread
        float gravity;          // gravity force
        bool looping;           // continuous emission
        bool active;            // emitter active
        bool is3D;              // render in 3D space
        std::vector<Particle> particles;
        void *texture;          // particle texture
    };

    // Initialize particle system
    void init();

    // Shutdown and cleanup
    void shutdown();

    // Set quality level (from settings)
    void setQuality(ParticleQuality quality);
    ParticleQuality getQuality();

    // Create an emitter
    Emitter* createEmitter(const std::string &name, ParticleType type, float x, float y, float z);

    // Destroy an emitter
    void destroyEmitter(Emitter *emitter);

    // Update all emitters
    void update(float dt);

    // Render all active particles (3D)
    void renderParticles3D();

    // Render all active particles (2D screen-space)
    void renderParticles2D();

    // Load particle texture from path
    void *loadParticleTexture(const std::string &path);

    // Get max particle count for quality level
    int getMaxParticleCount(ParticleQuality quality);

    // Get max emitter count for quality level
    int getMaxEmitterCount(ParticleQuality quality);

    // Burst: emit all particles at once (for explosions)
    void burstEmitter(Emitter *emitter);

    // Stop emitter
    void stopEmitter(Emitter *emitter);

    // Resume emitter
    void resumeEmitter(Emitter *emitter);

    // Get particle count (for debug)
    int getTotalParticleCount();

    // Get emitter count
    int getEmitterCount();

    // Preset configurations for known particle types
    void configureFireEmitter(Emitter *emitter);
    void configureExplosionEmitter(Emitter *emitter);
    void configureSmokeEmitter(Emitter *emitter);
    void configureSparklesEmitter(Emitter *emitter);
    void configureForceFieldBeamEmitter(Emitter *emitter);
    void configureForceFieldRadialEmitter(Emitter *emitter);
    void configureBeaconGlowEmitter(Emitter *emitter);
    void configureBeaconTrailEmitter(Emitter *emitter);
}