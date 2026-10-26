#include "particle.h"
#include "settings.h"
#include "render_pipeline.h"
#include "texture_loader.h"
#include "content_system.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>

namespace ParticleSystem {

    static std::vector<Emitter*> gEmitters;
    static ParticleQuality gQuality = ParticleQuality::Medium;
    static int gTotalParticles = 0;

    int getMaxParticleCount(ParticleQuality quality) {
        switch (quality) {
            case ParticleQuality::Off:    return 0;
            case ParticleQuality::Low:    return 256;
            case ParticleQuality::Medium: return 512;
            case ParticleQuality::High:   return 1024;
            default: return 512;
        }
    }

    int getMaxEmitterCount(ParticleQuality quality) {
        switch (quality) {
            case ParticleQuality::Off:    return 0;
            case ParticleQuality::Low:    return 8;
            case ParticleQuality::Medium: return 16;
            case ParticleQuality::High:   return 32;
            default: return 16;
        }
    }

    void init() {
        gEmitters.clear();
        gTotalParticles = 0;
        // Sync with settings
        gQuality = static_cast<ParticleQuality>(Settings::get().graphics.particleQuality);
    }

    void shutdown() {
        for (Emitter *e : gEmitters) delete e;
        gEmitters.clear();
        gTotalParticles = 0;
    }

    void setQuality(ParticleQuality quality) {
        gQuality = quality;
        // Enforce limits
        int maxCount = getMaxEmitterCount(quality);
        while ((int)gEmitters.size() > maxCount) {
            Emitter *e = gEmitters.back();
            gEmitters.pop_back();
            delete e;
        }
    }

    ParticleQuality getQuality() { return gQuality; }

    Emitter* createEmitter(const std::string &name, ParticleType type, float x, float y, float z) {
        int maxCount = getMaxEmitterCount(gQuality);
        if ((int)gEmitters.size() >= maxCount) {
            printf("Particle: max emitters reached (%d)\n", maxCount);
            return nullptr;
        }
        Emitter *e = new Emitter();
        e->name = name;
        e->type = type;
        e->x = x; e->y = y; e->z = z;
        e->maxParticles = getMaxParticleCount(gQuality);
        e->spawnRate = 10.0f;
        e->emissionTimer = 0;
        e->particleLifetime = 1.0f;
        e->startSize = 1.0f;
        e->endSize = 0.2f;
        e->speed = 1.0f;
        e->spread = 1.0f;
        e->gravity = -9.8f;
        e->looping = true;
        e->active = true;
        e->is3D = true;
        e->texture = nullptr;

        // Apply type-specific config
        switch (type) {
            case ParticleType::Fire:          configureFireEmitter(e); break;
            case ParticleType::Explosion:     configureExplosionEmitter(e); break;
            case ParticleType::Smoke:         configureSmokeEmitter(e); break;
            case ParticleType::Sparkles:      configureSparklesEmitter(e); break;
            case ParticleType::ForceFieldBeam:    configureForceFieldBeamEmitter(e); break;
            case ParticleType::ForceFieldRadial:  configureForceFieldRadialEmitter(e); break;
            case ParticleType::BeaconGlow:    configureBeaconGlowEmitter(e); break;
            case ParticleType::BeaconTrail:   configureBeaconTrailEmitter(e); break;
        }

        // Try loading texture from content
        const char *texName = nullptr;
        switch (type) {
            case ParticleType::Fire:      texName = "fire_0.png"; break;
            case ParticleType::Explosion: texName = "explosion.png"; break;
            case ParticleType::Smoke:     texName = "Smoke.png"; break;
            case ParticleType::Sparkles:  texName = "sparkle.png"; break;
            case ParticleType::BeaconGlow: texName = "glow.png"; break;
            default: break;
        }
        if (texName) {
            std::string path = ContentSystem::resolveAsset(texName, ContentSystem::ContentType::Texture);
            if (!path.empty()) {
                e->texture = TextureLoader::getCachedTexture(path);
            }
        }

        gEmitters.push_back(e);
        return e;
    }

    void destroyEmitter(Emitter *emitter) {
        auto it = std::find(gEmitters.begin(), gEmitters.end(), emitter);
        if (it != gEmitters.end()) {
            gEmitters.erase(it);
            delete emitter;
        }
    }

    static float randf(float min, float max) {
        return min + ((float)rand() / RAND_MAX) * (max - min);
    }

    static void spawnParticle(Emitter *e) {
        if ((int)e->particles.size() >= e->maxParticles) return;
        Particle p;
        float angle = randf(0, M_PI * 2.0f);
        float upSpeed = randf(e->speed * 0.5f, e->speed);
        p.x = e->x; p.y = e->y; p.z = e->z;
        p.vx = sinf(angle) * cosf(angle) * e->spread * e->speed * 0.3f;
        p.vy = upSpeed;
        p.vz = cosf(angle) * sinf(angle) * e->spread * e->speed * 0.3f;
        p.size = e->startSize;
        p.lifetime = e->particleLifetime;
        p.maxLifetime = e->particleLifetime;
        p.rotation = randf(0, M_PI * 2.0f);
        p.spin = randf(-5.0f, 5.0f);

        // Type-specific colors
        switch (e->type) {
            case ParticleType::Fire:
                p.r = 1.0f; p.g = randf(0.3f, 0.8f); p.b = 0.0f; break;
            case ParticleType::Smoke:
                p.r = 0.5f; p.g = 0.5f; p.b = 0.5f; break;
            case ParticleType::Explosion:
                p.r = 1.0f; p.g = randf(0.5f, 0.9f); p.b = randf(0.0f, 0.3f); break;
            case ParticleType::Sparkles:
                p.r = 1.0f; p.g = 1.0f; p.b = 1.0f; break;
            case ParticleType::ForceFieldBeam:
            case ParticleType::ForceFieldRadial:
                p.r = 0.3f; p.g = 0.8f; p.b = 1.0f; break;
            case ParticleType::BeaconGlow:
                p.r = 1.0f; p.g = 1.0f; p.b = 0.8f; break;
            case ParticleType::BeaconTrail:
                p.r = 0.8f; p.g = 0.8f; p.b = 1.0f; break;
            default:
                p.r = 1.0f; p.g = 1.0f; p.b = 1.0f;
        }
        p.a = 1.0f;
        e->particles.push_back(p);
    }

    void update(float dt) {
        if (gQuality == ParticleQuality::Off) return;

        for (Emitter *e : gEmitters) {
            if (!e->active || !e->looping) continue;

            // Spawn new particles
            e->emissionTimer += dt;
            float spawnInterval = 1.0f / e->spawnRate;
            while (e->emissionTimer >= spawnInterval) {
                spawnParticle(e);
                e->emissionTimer -= spawnInterval;
            }

            // Update particles
            for (auto it = e->particles.begin(); it != e->particles.end();) {
                Particle &p = *it;
                p.lifetime -= dt;
                if (p.lifetime <= 0) {
                    it = e->particles.erase(it);
                    continue;
                }
                float lifeRatio = p.lifetime / p.maxLifetime;
                p.vy += e->gravity * dt;
                p.x += p.vx * dt;
                p.y += p.vy * dt;
                p.z += p.vz * dt;
                p.size = e->endSize + (e->startSize - e->endSize) * lifeRatio;
                p.a = lifeRatio;
                p.rotation += p.spin * dt;
                ++it;
            }
        }
        gTotalParticles = 0;
        for (Emitter *e : gEmitters) gTotalParticles += (int)e->particles.size();
    }

    void renderParticles3D() {
        if (gQuality == ParticleQuality::Off) return;
        RenderPipeline::setBlend(true);
        for (Emitter *e : gEmitters) {
            if (!e->active || e->particles.empty()) continue;
            for (const Particle &p : e->particles) {
                RenderPipeline::pushMatrix();
                RenderPipeline::translate(p.x, p.y, p.z);
                RenderPipeline::rotate(p.rotation * 57.2958f, 0, 0, 1);
                RenderPipeline::scale(p.size, p.size, 1.0f);
                RenderPipeline::drawRect(-0.5f, -0.5f, 1.0f, 1.0f, p.r, p.g, p.b, p.a);
                RenderPipeline::popMatrix();
            }
        }
        RenderPipeline::setBlend(false);
    }

    void renderParticles2D() {
        if (gQuality == ParticleQuality::Off) return;
        RenderPipeline::setBlend(true);
        for (Emitter *e : gEmitters) {
            if (!e->active || e->particles.empty() || e->is3D) continue;
            for (const Particle &p : e->particles) {
                RenderPipeline::drawRect(p.x, p.y, p.size, p.size, p.r, p.g, p.b, p.a);
            }
        }
        RenderPipeline::setBlend(false);
    }

    void *loadParticleTexture(const std::string &path) {
        return TextureLoader::getCachedTexture(path);
    }

    void burstEmitter(Emitter *emitter) {
        if (!emitter) return;
        for (int i = 0; i < emitter->maxParticles / 2; i++) {
            spawnParticle(emitter);
        }
    }

    void stopEmitter(Emitter *emitter) {
        if (emitter) emitter->active = false;
    }

    void resumeEmitter(Emitter *emitter) {
        if (emitter) emitter->active = true;
    }

    int getTotalParticleCount() { return gTotalParticles; }
    int getEmitterCount() { return (int)gEmitters.size(); }

    // === Preset configurations ===

    void configureFireEmitter(Emitter *e) {
        e->spawnRate = 20.0f;
        e->particleLifetime = 0.8f;
        e->startSize = 0.6f;
        e->endSize = 0.1f;
        e->speed = 2.0f;
        e->spread = 0.5f;
        e->gravity = -1.0f;
        e->looping = true;
    }

    void configureExplosionEmitter(Emitter *e) {
        e->spawnRate = 100.0f;
        e->particleLifetime = 0.6f;
        e->startSize = 1.0f;
        e->endSize = 0.05f;
        e->speed = 5.0f;
        e->spread = 2.0f;
        e->gravity = -5.0f;
        e->looping = false;
        e->active = false; // burst only
    }

    void configureSmokeEmitter(Emitter *e) {
        e->spawnRate = 8.0f;
        e->particleLifetime = 2.0f;
        e->startSize = 0.5f;
        e->endSize = 2.0f;
        e->speed = 1.0f;
        e->spread = 0.8f;
        e->gravity = 1.0f; // rises
        e->looping = true;
    }

    void configureSparklesEmitter(Emitter *e) {
        e->spawnRate = 15.0f;
        e->particleLifetime = 1.0f;
        e->startSize = 0.3f;
        e->endSize = 0.05f;
        e->speed = 1.5f;
        e->spread = 1.5f;
        e->gravity = -3.0f;
        e->looping = true;
    }

    void configureForceFieldBeamEmitter(Emitter *e) {
        e->spawnRate = 30.0f;
        e->particleLifetime = 0.3f;
        e->startSize = 0.4f;
        e->endSize = 0.1f;
        e->speed = 3.0f;
        e->spread = 0.2f;
        e->gravity = 0.0f;
        e->looping = true;
    }

    void configureForceFieldRadialEmitter(Emitter *e) {
        e->spawnRate = 25.0f;
        e->particleLifetime = 0.5f;
        e->startSize = 0.3f;
        e->endSize = 0.8f;
        e->speed = 2.0f;
        e->spread = 3.0f;
        e->gravity = 0.0f;
        e->looping = true;
    }

    void configureBeaconGlowEmitter(Emitter *e) {
        e->spawnRate = 12.0f;
        e->particleLifetime = 1.5f;
        e->startSize = 0.8f;
        e->endSize = 0.2f;
        e->speed = 0.5f;
        e->spread = 0.3f;
        e->gravity = 2.0f; // rises
        e->looping = true;
    }

    void configureBeaconTrailEmitter(Emitter *e) {
        e->spawnRate = 20.0f;
        e->particleLifetime = 0.8f;
        e->startSize = 0.5f;
        e->endSize = 0.0f;
        e->speed = 0.3f;
        e->spread = 0.1f;
        e->gravity = -1.0f;
        e->looping = true;
    }
}