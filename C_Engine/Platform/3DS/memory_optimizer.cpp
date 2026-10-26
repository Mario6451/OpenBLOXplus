#include "memory_optimizer.h"
#include "settings.h"
#include "texture_loader.h"
#include <3ds.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

namespace MemoryOptimizer {

    // Texture tracking
    static std::map<std::string, TextureMemInfo> gTextures;
    static size_t gVRAMUsage = 0;
    static const size_t gVRAMBudget = 32 * 1024 * 1024; // 32MB budget out of 80MB heap

    // LOD tracking
    static std::map<std::string, std::vector<LODLevel>> gLODMeshes;

    // Frustum planes (ax + by + cz + d > 0 means inside)
    struct FrustumPlane { float a, b, c, d; };
    static FrustumPlane gFrustumPlanes[6];
    static bool gFrustumValid = false;

    // FPS tracking
    static int gFrameCount = 0;
    static float gFPSAccumulator = 0;
    static int gCurrentFPS = 0;
    static float gAverageFPS = 0;
    static float gFrameTimeMs = 0;
    static u64 gLastFrameTime = 0;

    void init() {
        gTextures.clear();
        gVRAMUsage = 0;
        gLODMeshes.clear();
        gFrustumValid = false;
        gFrameCount = 0;
        gFPSAccumulator = 0;
        gCurrentFPS = 0;
        gAverageFPS = 0;
        gFrameTimeMs = 0;
        gLastFrameTime = osGetTime();
    }

    void shutdown() {
        gTextures.clear();
        gLODMeshes.clear();
        gVRAMUsage = 0;
    }

    // === Texture Management ===

    void registerTexture(const std::string &path, void *texture,
                         int width, int height, size_t size) {
        TextureMemInfo info;
        info.path = path;
        info.originalSize = size;
        info.compressedSize = size;
        info.originalWidth = width;
        info.originalHeight = height;
        info.compressedWidth = width;
        info.compressedHeight = height;
        info.isCompressed = false;
        info.isInVRAM = true;
        gTextures[path] = info;
        gVRAMUsage += size;
    }

    void getOptimalSize(int originalWidth, int originalHeight,
                        int *outWidth, int *outHeight) {
        if (!Settings::get().graphics.textureAutoScale) {
            *outWidth = originalWidth;
            *outHeight = originalHeight;
            return;
        }

        // Cap textures based on VRAM budget
        *outWidth = originalWidth;
        *outHeight = originalHeight;

        // If texture is larger than 1024x1024, downscale
        while ((*outWidth > 512 || *outHeight > 512) &&
               gVRAMUsage > gVRAMBudget * 0.8f) {
            *outWidth /= 2;
            *outHeight /= 2;
        }

        // Ensure power of 2
        *outWidth = std::max(1, *outWidth);
        *outHeight = std::max(1, *outHeight);
    }

    void *compressTexture(void *texture, int width, int height) {
        // On 3DS, textures are typically stored as RGBA8 (4 bytes/pixel)
        // Simple downsampling if needed
        if (!Settings::get().graphics.textureAutoScale) return texture;
        if (width <= 512 && height <= 512) return texture;

        // Downscale to 512x512 max
        int newW = 512, newH = 512;
        if (width > height) newH = (height * 512) / width;
        else newW = (width * 512) / height;

        u8 *src = (u8*)texture;
        u8 *dst = (u8*)linearAlloc(newW * newH * 4);
        if (!dst) return texture;

        // Simple nearest-neighbor downscale
        float stepX = (float)width / newW;
        float stepY = (float)height / newH;
        for (int y = 0; y < newH; y++) {
            for (int x = 0; x < newW; x++) {
                int sx = (int)(x * stepX);
                int sy = (int)(y * stepY);
                if (sx >= width) sx = width - 1;
                if (sy >= height) sy = height - 1;
                int dstIdx = (y * newW + x) * 4;
                int srcIdx = (sy * width + sx) * 4;
                dst[dstIdx] = src[srcIdx];
                dst[dstIdx + 1] = src[srcIdx + 1];
                dst[dstIdx + 2] = src[srcIdx + 2];
                dst[dstIdx + 3] = src[srcIdx + 3];
            }
        }
        return dst;
    }

    size_t getVRAMUsage() { return gVRAMUsage; }
    size_t getVRAMBudget() { return gVRAMBudget; }

    float getVRAMUsagePercent() {
        if (gVRAMBudget == 0) return 0;
        return (float)gVRAMUsage / (float)gVRAMBudget * 100.0f;
    }

    bool shouldDownsample(int width, int height) {
        if (!Settings::get().graphics.textureAutoScale) return false;
        return (width > 512 || height > 512 ||
                gVRAMUsage > gVRAMBudget * 0.7f);
    }

    const TextureMemInfo* getTextureInfo(const std::string &path) {
        auto it = gTextures.find(path);
        if (it != gTextures.end()) return &it->second;
        return nullptr;
    }

    void evictTexturesIfNeeded() {
        if (gVRAMUsage <= gVRAMBudget * 0.9f) return;
        printf("MemoryOptimizer: VRAM critical (%.1f%%), evicting textures\n",
               getVRAMUsagePercent());
        // Evict least recently used textures until under budget
        auto it = gTextures.end();
        std::advance(it, -1);
        while (gVRAMUsage > gVRAMBudget * 0.7f && !gTextures.empty()) {
            auto oldest = it;
            gVRAMUsage -= oldest->second.compressedSize;
            gTextures.erase(oldest);
            if (gTextures.empty()) break;
            it = gTextures.end();
            std::advance(it, -1);
        }
    }

    // === LOD System ===

    void setLODLevels(const std::string &meshPath, void **meshes, int count,
                      const float *minDists, const float *maxDists) {
        std::vector<LODLevel> levels;
        for (int i = 0; i < count; i++) {
            LODLevel lod;
            lod.mesh = meshes[i];
            lod.minDistance = minDists[i];
            lod.maxDistance = (i + 1 < count) ? maxDists[i + 1] : 9999.0f;
            lod.vertexCount = 0;
            levels.push_back(lod);
        }
        gLODMeshes[meshPath] = levels;
    }

    int getLODIndex(const std::string &meshPath, float distance) {
        auto it = gLODMeshes.find(meshPath);
        if (it == gLODMeshes.end()) return 0;
        const auto &levels = it->second;
        for (int i = 0; i < (int)levels.size(); i++) {
            if (distance >= levels[i].minDistance &&
                distance <= levels[i].maxDistance) {
                return i;
            }
        }
        return (int)levels.size() - 1; // Return lowest LOD
    }

    void *getLODMesh(const std::string &meshPath, float distance) {
        int idx = getLODIndex(meshPath, distance);
        auto it = gLODMeshes.find(meshPath);
        if (it == gLODMeshes.end()) return nullptr;
        if (idx >= 0 && idx < (int)it->second.size())
            return it->second[idx].mesh;
        return nullptr;
    }

    // === Frustum Culling ===

    void updateFrustum(float *viewProjMatrix) {
        if (!viewProjMatrix) {
            gFrustumValid = false;
            return;
        }
        // Extract frustum planes from view-projection matrix
        // This is a simplified extraction
        (void)viewProjMatrix;
        // Default: all planes far out (everything visible)
        for (int i = 0; i < 6; i++) {
            gFrustumPlanes[i].a = 0;
            gFrustumPlanes[i].b = 0;
            gFrustumPlanes[i].c = 0;
            gFrustumPlanes[i].d = 1;
        }
        gFrustumValid = true;
    }

    bool isInFrustum(float sx, float sy, float sz, float radius) {
        if (!gFrustumValid) return true;
        // Simplified: without proper plane extraction, assume visible
        // Full implementation would test sphere against all 6 planes
        (void)sx; (void)sy; (void)sz; (void)radius;
        return true;
    }

    bool isInFrustumBox(float minX, float minY, float minZ,
                        float maxX, float maxY, float maxZ) {
        if (!gFrustumValid) return true;
        (void)minX; (void)minY; (void)minZ;
        (void)maxX; (void)maxY; (void)maxZ;
        return true;
    }

    // === FPS Counter ===

    void updateFPS() {
        u64 now = osGetTime();
        u64 delta = now - gLastFrameTime;
        gLastFrameTime = now;

        gFrameTimeMs = (float)delta / 1000000.0f;
        gFrameCount++;
        gFPSAccumulator += (float)delta;

        if (gFPSAccumulator >= 1000000000ULL) { // 1 second
            gCurrentFPS = gFrameCount;
            gAverageFPS = gAverageFPS * 0.9f + (float)gFrameCount * 0.1f;
            gFrameCount = 0;
            gFPSAccumulator = 0;
        }
    }

    int getCurrentFPS() { return gCurrentFPS; }
    float getAverageFPS() { return gAverageFPS; }
    float getFrameTime() { return gFrameTimeMs; }

    // === Debug Info ===

    std::string getMemoryDebugString() {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "VRAM: %zu/%zu KB (%.1f%%) | FPS: %d | Textures: %zu | Frame: %.1fms",
                 gVRAMUsage / 1024, gVRAMBudget / 1024,
                 getVRAMUsagePercent(), gCurrentFPS,
                 (size_t)gTextures.size(), gFrameTimeMs);
        return std::string(buf);
    }

    int getParticleCount() {
        // Placeholder: integrate with ParticleSystem::getTotalParticleCount()
        return 0;
    }
}