#include "particle_system.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <sstream>
#include <map>
#include <vector>
#include <3ds.h>

#define MAX_PARTICLES 2000
#define MAX_INSTANCES 64

struct StaticParticle {
    float pos[3], vel[3];
    float color[4], size, rotation, rotSpeed;
    float life, maxLife;
    bool active;
};

struct StaticInstance {
    int id;
    char sys[64];
    float pos[3];
    float elapsed;
    bool active;
    int particleCount;
    int particles[MAX_PARTICLES]; // indices into a global particle pool
};

// Internal pools (kept outside class for fixed memory layout on 3DS)
static StaticParticle  particlePool[MAX_PARTICLES];
static StaticInstance  instancePool[MAX_INSTANCES];
static int particlePoolUsed = 0;
static int nextId = 1;
static bool initialized = false;

// Static member definitions for ParticleSystem
std::map<std::string, ParticleEmitterConfig> ParticleSystem::emitters;
std::map<std::string, MaterialConfig> ParticleSystem::materials;
std::vector<ParticleSystem::ParticleInstance> ParticleSystem::instances;
int ParticleSystem::nextInstanceId = 1;

void ParticleSystem::init() {
    if (initialized) return;
    memset(particlePool, 0, sizeof(particlePool));
    memset(instancePool, 0, sizeof(instancePool));
    initialized = true;
}

void ParticleSystem::shutdown() {
    stopAll();
    emitters.clear();
    materials.clear();
    initialized = false;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static void parseFloatArray(const std::string& line, float* out, int count) {
    std::string trimmed = trim(line);
    std::istringstream iss(trimmed);
    for (int i = 0; i < count; ++i) iss >> out[i];
}

static bool readFile(const std::string& path, std::string& out) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(size);
    fread(&out[0], 1, size, f);
    fclose(f);
    return true;
}

bool ParticleSystem::loadParticleFile(const std::string& path, std::string /*contentFolder*/) {
    std::string content;
    if (!readFile(path, content)) return false;

    std::istringstream stream(content);
    std::string line;
    std::string currentSystem;

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '/') continue;

        if (trimmed.find("particle_system") != std::string::npos) {
            size_t pos = trimmed.find("particle_system");
            std::string after = trimmed.substr(pos + 15);
            currentSystem = trim(after);
            ParticleEmitterConfig cfg;
            cfg.name = currentSystem;
            ParticleSystem::emitters[currentSystem] = cfg;
        }

        if (!currentSystem.empty() && ParticleSystem::emitters.count(currentSystem)) {
            ParticleEmitterConfig& cfg = ParticleSystem::emitters[currentSystem];

            if (trimmed.find("particle_width") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.particleWidth; cfg.name = currentSystem; }
            else if (trimmed.find("particle_height") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.particleHeight; cfg.name = currentSystem; }
            else if (trimmed.find("quota") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.quota; cfg.name = currentSystem; }
            else if (trimmed.find("emission_rate") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.emissionRate; cfg.name = currentSystem; }
            else if (trimmed.find("velocity_min") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.velocityMin; cfg.name = currentSystem; }
            else if (trimmed.find("velocity_max") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.velocityMax; cfg.name = currentSystem; }
            else if (trimmed.find("time_to_live_min") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.ttlMin; cfg.name = currentSystem; }
            else if (trimmed.find("time_to_live_max") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.ttlMax; cfg.name = currentSystem; }
            else if (trimmed.find("duration") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.duration; cfg.name = currentSystem; }
            else if (trimmed.find("material") == 0) {
                size_t pos = trimmed.find("material");
                std::string after = trim(trimmed.substr(pos + 8));
                cfg.materialName = after;
            }

            if (trimmed.find("emitter ") == 0) {
                std::string after = trim(trimmed.substr(8));
                cfg.emitterType = after;
            }

            if (trimmed.find("angle") == 0) {
                size_t pos = trimmed.find("angle");
                std::string after = trim(trimmed.substr(pos + 5));
                float val; std::istringstream(after) >> val;
                cfg.angle = val;
            }

            if (trimmed.find("direction") == 0) {
                size_t pos = trimmed.find("direction");
                std::string after = trim(trimmed.substr(pos + 9));
                parseFloatArray(after, cfg.direction, 3);
            }

            if (trimmed.find("position") == 0) {
                size_t pos = trimmed.find("position");
                std::string after = trim(trimmed.substr(pos + 8));
                parseFloatArray(after, cfg.position, 3);
            }

            if (trimmed.find("colour_range_start") == 0) {
                size_t pos = trimmed.find("colour_range_start");
                std::string after = trim(trimmed.substr(pos + 16));
                parseFloatArray(after, cfg.colorStart, 4);
            }

            if (trimmed.find("colour_range_end") == 0) {
                size_t pos = trimmed.find("colour_range_end");
                std::string after = trim(trimmed.substr(pos + 14));
                parseFloatArray(after, cfg.colorEnd, 4);
            }

            if (trimmed.find("ColourFader") != std::string::npos) continue;
            if (trimmed.find("Scaler") != std::string::npos) continue;
            if (trimmed.find("Rotator") != std::string::npos) continue;
            if (trimmed.find("ColourInterpolator") != std::string::npos) continue;

            if (trimmed.find("red") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.colorFadeR; }
            if (trimmed.find("green") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.colorFadeG; }
            if (trimmed.find("blue") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.colorFadeB; }
            if (trimmed.find("alpha") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.colorFadeA; }
            if (trimmed.find("rate") == 0) { std::istringstream(trimmed) >> currentSystem >> cfg.scaleRate; }
        }
    }
    return true;
}

bool ParticleSystem::loadMaterialFile(const std::string& path, std::string /*contentFolder*/) {
    std::string content;
    if (!readFile(path, content)) return false;

    std::istringstream stream(content);
    std::string line;
    std::string currentMaterial;

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '/') continue;

        if (trimmed.find("material ") == 0) {
            std::string after = trim(trimmed.substr(9));
            currentMaterial = after;
            MaterialConfig cfg;
            cfg.name = currentMaterial;
            ParticleSystem::materials[currentMaterial] = cfg;
        }

        if (!currentMaterial.empty() && ParticleSystem::materials.count(currentMaterial)) {
            MaterialConfig& cfg = ParticleSystem::materials[currentMaterial];

            if (trimmed.find("lighting off") != std::string::npos) cfg.lighting = false;
            if (trimmed.find("lighting on") != std::string::npos) cfg.lighting = true;
            if (trimmed.find("depth_write off") != std::string::npos) cfg.depthWrite = false;
            if (trimmed.find("depth_write on") != std::string::npos) cfg.depthWrite = true;
            if (trimmed.find("alpha_blend") != std::string::npos) cfg.alphaBlend = true;
            if (trimmed.find("src_alpha") != std::string::npos) cfg.alphaBlend = true;

            if (trimmed.find("texture ") == 0) {
                size_t pos = trimmed.find("texture");
                std::string after = trim(trimmed.substr(pos + 8));
                cfg.texturePath = after;
            }
        }
    }
    return true;
}

int ParticleSystem::spawn(const std::string& name, float pos[3]) {
    if (emitters.find(name) == emitters.end()) return -1;
    if (nextId > MAX_INSTANCES) return -1;

    int idx = nextId - 1;
    instancePool[idx].id = nextId;
    strncpy(instancePool[idx].sys, name.c_str(), 63);
    instancePool[idx].sys[63] = '\0';
    memcpy(instancePool[idx].pos, pos, sizeof(float) * 3);
    instancePool[idx].elapsed = 0;
    instancePool[idx].active = true;
    instancePool[idx].particleCount = 0;
    memset(instancePool[idx].particles, 0, sizeof(int) * MAX_PARTICLES);

    return nextId++;
}

static float randf(float min, float max) {
    return min + (float)rand() / RAND_MAX * (max - min);
}

void ParticleSystem::update(float dt) {
    for (int i = 0; i < nextId; ++i) {
        StaticInstance& inst = instancePool[i];
        if (!inst.active) continue;

        inst.elapsed += dt;

        const auto& it = emitters.find(inst.sys);
        if (it == emitters.end()) continue;
        const ParticleEmitterConfig& cfg = it->second;

        // Check duration limit
        if (cfg.duration > 0 && inst.elapsed > cfg.duration) {
            inst.active = false;
            continue;
        }

        // Emit new particles
        int toEmit = (int)(cfg.emissionRate * dt);
        toEmit = (toEmit > cfg.quota) ? cfg.quota : toEmit;

        for (int j = 0; j < toEmit && inst.particleCount < cfg.quota && particlePoolUsed < MAX_PARTICLES; ++j) {
            int poolIdx = particlePoolUsed++;
            StaticParticle* p = &particlePool[poolIdx];

            p->active = true;
            p->pos[0] = inst.pos[0] + cfg.position[0] + randf(-1, 1);
            p->pos[1] = inst.pos[1] + cfg.position[1];
            p->pos[2] = inst.pos[2] + cfg.position[2] + randf(-1, 1);

            float speed = randf(cfg.velocityMin, cfg.velocityMax);
            float angle = randf(0, cfg.angle) * 3.14159f / 180.0f;
            p->vel[0] = cfg.direction[0] * speed + cosf(angle) * speed * 0.5f;
            p->vel[1] = cfg.direction[1] * speed;
            p->vel[2] = cfg.direction[2] * speed + sinf(angle) * speed * 0.5f;

            p->life = randf(cfg.ttlMin, cfg.ttlMax);
            p->maxLife = p->life;
            p->size = cfg.particleWidth;

            p->color[0] = cfg.colorStart[0];
            p->color[1] = cfg.colorStart[1];
            p->color[2] = cfg.colorStart[2];
            p->color[3] = cfg.colorStart[3];

            p->rotation = 0;
            p->rotSpeed = randf(-200, 200);

            inst.particles[inst.particleCount++] = poolIdx;
        }

        // Update existing particles
        for (int j = 0; j < inst.particleCount; ++j) {
            int poolIdx = inst.particles[j];
            if (poolIdx >= MAX_PARTICLES) continue;
            StaticParticle* p = &particlePool[poolIdx];
            if (!p->active) continue;

            p->life -= dt;
            if (p->life <= 0) {
                p->active = false;
                continue;
            }

            p->pos[0] += p->vel[0] * dt;
            p->pos[1] += p->vel[1] * dt;
            p->pos[2] += p->vel[2] * dt;

            // Color fade
            p->color[0] += cfg.colorFadeR * dt;
            p->color[1] += cfg.colorFadeG * dt;
            p->color[2] += cfg.colorFadeB * dt;
            p->color[3] += cfg.colorFadeA * dt;
            if (p->color[0] < 0) p->color[0] = 0;
            if (p->color[1] < 0) p->color[1] = 0;
            if (p->color[2] < 0) p->color[2] = 0;
            if (p->color[3] < 0) p->color[3] = 0;
            if (p->color[0] > 1) p->color[0] = 1;
            if (p->color[1] > 1) p->color[1] = 1;
            if (p->color[2] > 1) p->color[2] = 1;
            if (p->color[3] > 1) p->color[3] = 1;

            // Scale
            if (cfg.scaleRate != 0) {
                p->size += cfg.scaleRate * dt;
                if (p->size < 0) p->size = 0;
            }

            // Rotation
            p->rotation += p->rotSpeed * dt;

            // Gravity approximation (fire/smoke drift up)
            p->vel[1] += 2.0f * dt;
        }

        // Clean up dead particles from front
        int writeIdx = 0;
        for (int j = 0; j < inst.particleCount; ++j) {
            int poolIdx = inst.particles[j];
            if (poolIdx < MAX_PARTICLES && particlePool[poolIdx].active) {
                inst.particles[writeIdx++] = poolIdx;
            }
        }
        inst.particleCount = writeIdx;
    }
}

void ParticleSystem::render() {
    // Particle rendering placeholder - integrate with citro_renderer.cpp
    // For each active instance, render particles as billboarded sprites
    // This would use C3D_Tex and C3D_DrawArrays
    // Full implementation requires texture loading integration
}

void ParticleSystem::stop(int id) {
    int idx = id - 1;
    if (idx >= 0 && idx < MAX_INSTANCES) {
        instancePool[idx].active = false;
        // Deactivate particles
        for (int j = 0; j < instancePool[idx].particleCount; ++j) {
            int poolIdx = instancePool[idx].particles[j];
            if (poolIdx < MAX_PARTICLES) particlePool[poolIdx].active = false;
        }
        instancePool[idx].particleCount = 0;
    }
}

void ParticleSystem::stopAll() {
    for (int i = 0; i < nextId; ++i) {
        instancePool[i].active = false;
        for (int j = 0; j < instancePool[i].particleCount; ++j) {
            int poolIdx = instancePool[i].particles[j];
            if (poolIdx < MAX_PARTICLES) particlePool[poolIdx].active = false;
        }
        instancePool[i].particleCount = 0;
    }
    nextId = 1;
    particlePoolUsed = 0;
}

const std::string& ParticleSystem::getMaterialTexture(const std::string& name) {
    static std::string empty;
    auto it = materials.find(name);
    if (it != materials.end()) return it->second.texturePath;
    return empty;
}
