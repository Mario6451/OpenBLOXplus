#pragma once

#include <string>
#include <map>
#include <vector>

// Memory optimization: texture compression, LOD, frustum culling, VRAM budget

namespace MemoryOptimizer {

    // Texture memory info
    struct TextureMemInfo {
        std::string path;
        size_t originalSize;
        size_t compressedSize;
        int originalWidth, originalHeight;
        int compressedWidth, compressedHeight;
        bool isCompressed;
        bool isInVRAM;
    };

    // LOD levels for meshes
    struct LODLevel {
        void *mesh;              // mesh pointer
        float minDistance;       // minimum distance for this LOD
        float maxDistance;       // maximum distance for this LOD
        int vertexCount;         // vertex count for debugging
    };

    // Initialize memory optimizer
    void init();

    // Shutdown and cleanup
    void shutdown();

    // === Texture Management ===

    // Register a loaded texture for tracking
    void registerTexture(const std::string &path, void *texture,
                         int width, int height, size_t size);

    // Auto-scale texture to fit VRAM budget
    // Returns scaled dimensions
    void getOptimalSize(int originalWidth, int originalHeight,
                        int *outWidth, int *outHeight);

    // Compress texture (if textureAutoScale enabled)
    void *compressTexture(void *texture, int width, int height);

    // Get total VRAM usage
    size_t getVRAMUsage();

    // Get VRAM budget (3DS max ~64MB FRAM, we use portion)
    size_t getVRAMBudget();

    // Get VRAM usage percentage (0-100)
    float getVRAMUsagePercent();

    // Check if a texture should be downsampled
    bool shouldDownsample(int width, int height);

    // Get texture info
    const TextureMemInfo* getTextureInfo(const std::string &path);

    // Force unload textures when VRAM is full
    void evictTexturesIfNeeded();

    // === LOD System ===

    // Set LOD levels for a mesh
    void setLODLevels(const std::string &meshPath, void **meshes, int count,
                      const float *minDists, const float *maxDists);

    // Get current LOD index based on distance
    int getLODIndex(const std::string &meshPath, float distance);

    // Get LOD mesh pointer
    void *getLODMesh(const std::string &meshPath, float distance);

    // === Frustum Culling ===

    // Initialize frustum planes
    void updateFrustum(float *viewProjMatrix);

    // Check if bounding sphere is in frustum
    bool isInFrustum(float sx, float sy, float sz, float radius);

    // Check if bounding box is in frustum
    bool isInFrustumBox(float minX, float minY, float minZ,
                        float maxX, float maxY, float maxZ);

    // === FPS Counter ===

    // Update FPS (call once per frame)
    void updateFPS();

    // Get current FPS
    int getCurrentFPS();

    // Get average FPS
    float getAverageFPS();

    // Get frame time (ms)
    float getFrameTime();

    // === Debug Info ===

    // Get memory debug string
    std::string getMemoryDebugString();

    // Get particle memory info
    int getParticleCount();
}