#include "terrain.h"
#include "citro_renderer.h"
#include "texture_loader.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <map>

namespace Terrain {

    // Internal state
    TerrainType currentTerrainType;
    SeaLevel seaLevel;
    TerrainRegion selectedRegion;
    bool showWireframe;

    // Chunk storage
    static std::map<std::string, ClassicTerrainChunk*> classicChunks;
    static std::map<std::string, ModernTerrainChunk*> modernChunks;

    // GPU buffers for terrain rendering
    static C3D_Tex* terrainTextures[34]; // Terrain material textures
    static bool texturesLoaded = false;

    // Copy buffer for paste operations
    static std::vector<Voxel> copiedVoxels;
    static std::vector<TerrainCell> copiedCells;
    static TerrainRegion copyRegion;

    // Max active chunks (3DS memory constraints)
    static const int MAX_CLASSIC_CHUNKS = 32;
    static const int MAX_MODERN_CHUNKS = 16;

    // Helper: chunk coordinate key
    static std::string chunkKey(int cx, int cy, int cz) {
        return std::to_string(cx) + "," + std::to_string(cy) + "," + std::to_string(cz);
    }

    // Helper: world to chunk coordinates
    static void worldToChunk(float x, float y, float z, int& cx, int& cy, int& cz) {
        // Classic: 64-stud chunks
        cx = (int)floor(x / 64.0f);
        cy = (int)floor(y / 64.0f);
        cz = (int)floor(z / 64.0f);
    }

    // Helper: world to local voxel coordinates
    static void worldToLocal(float x, float y, float z, int& lx, int& ly, int& lz) {
        lx = (int)floor(fmod(fmod(x, 64.0f), 16.0f));
        ly = (int)floor(fmod(fmod(y, 64.0f), 16.0f));
        lz = (int)floor(fmod(fmod(z, 64.0f), 16.0f));
        if (lx < 0) lx += 16;
        if (ly < 0) ly += 16;
        if (lz < 0) lz += 16;
    }

    // Simple Perlin noise for terrain generation
    static float perlinNoise2D(float x, float y, float seed) {
        int ix = (int)floor(x);
        int iy = (int)floor(y);
        float fx = x - ix;
        float fy = y - iy;
        fx = fx * fx * (3.0f - 2.0f * fx);
        fy = fy * fy * (3.0f - 2.0f * fy);
        int n = ix + iy * 57 + (int)seed * 131;
        n = (n << 13) ^ n;
        int a = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
        n = (n + 31) + iy * 57 + (int)seed * 131;
        n = (n << 13) ^ n;
        int b = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
        float result = (a / 1073741824.0f) * (1.0f - fx) + (b / 1073741824.0f) * fx;
        result = result * (1.0f - fy) + ((result + 0.5f) * fy);
        return result * 2.0f - 1.0f;
    }

    static float noise3D(float x, float y, float z, float seed) {
        return (perlinNoise2D(x, y, seed) + perlinNoise2D(y, z, seed + 1.0f) + perlinNoise2D(x, z, seed + 2.0f)) / 3.0f;
    }

    // Get or create classic terrain chunk
    static ClassicTerrainChunk* getOrCreateClassicChunk(int cx, int cy, int cz) {
        std::string key = chunkKey(cx, cy, cz);
        auto it = classicChunks.find(key);
        if (it != classicChunks.end()) {
            return it->second;
        }
        if ((int)classicChunks.size() >= MAX_CLASSIC_CHUNKS) {
            // Remove oldest chunk (simple LRU: remove first entry)
            auto oldest = classicChunks.begin();
            delete oldest->second;
            classicChunks.erase(oldest);
        }
        ClassicTerrainChunk* chunk = new ClassicTerrainChunk();
        chunk->chunkX = cx;
        chunk->chunkY = cy;
        chunk->chunkZ = cz;
        chunk->dirty = true;
        memset(chunk->voxels, 0, sizeof(chunk->voxels));
        classicChunks[key] = chunk;
        return chunk;
    }

    // Get or create modern terrain chunk
    static ModernTerrainChunk* getOrCreateModernChunk(int cx, int cy, int cz) {
        std::string key = chunkKey(cx, cy, cz);
        auto it = modernChunks.find(key);
        if (it != modernChunks.end()) {
            return it->second;
        }
        if ((int)modernChunks.size() >= MAX_MODERN_CHUNKS) {
            auto oldest = modernChunks.begin();
            delete oldest->second;
            modernChunks.erase(oldest);
        }
        ModernTerrainChunk* chunk = new ModernTerrainChunk();
        chunk->chunkX = cx;
        chunk->chunkY = cy;
        chunk->chunkZ = cz;
        chunk->dirty = true;
        memset(chunk->cells, 0, sizeof(chunk->cells));
        modernChunks[key] = chunk;
        return chunk;
    }

    // ==================== Implementation ====================

    void init() {
        currentTerrainType = TerrainType::ClassicVoxel;
        seaLevel.height = 0.0f;
        seaLevel.enableWater = false;
        seaLevel.waterMaterial = TerrainMaterial::Water;
        memset(&selectedRegion, 0, sizeof(selectedRegion));
        showWireframe = false;
        texturesLoaded = false;
    }

    void setTerrainType(TerrainType type) {
        currentTerrainType = type;
        forceRebuildAllChunks();
    }

    TerrainType getTerrainType() {
        return currentTerrainType;
    }

    Voxel getVoxel(float x, float y, float z) {
        int cx, cy, cz, lx, ly, lz;
        worldToChunk(x, y, z, cx, cy, cz);
        worldToLocal(x, y, z, lx, ly, lz);
        auto it = classicChunks.find(chunkKey(cx, cy, cz));
        if (it != classicChunks.end()) {
            return it->second->voxels[lx][ly][lz];
        }
        return {TerrainMaterial::Air, false, 0xFFFFFFFF};
    }

    void setVoxel(float x, float y, float z, TerrainMaterial material, u32 color) {
        int cx, cy, cz, lx, ly, lz;
        worldToChunk(x, y, z, cx, cy, cz);
        worldToLocal(x, y, z, lx, ly, lz);
        ClassicTerrainChunk* chunk = getOrCreateClassicChunk(cx, cy, cz);
        chunk->voxels[lx][ly][lz].material = material;
        chunk->voxels[lx][ly][lz].isSolid = (material != TerrainMaterial::Air);
        chunk->voxels[lx][ly][lz].color = color;
        chunk->dirty = true;
    }

    bool isVoxelSolid(float x, float y, float z) {
        return getVoxel(x, y, z).isSolid;
    }

    TerrainCell getCell(float x, float y, float z) {
        int cx, cy, cz;
        worldToChunk(x, y, z, cx, cy, cz);
        auto it = modernChunks.find(chunkKey(cx, cy, cz));
        if (it != modernChunks.end()) {
            int lx = (int)floor(x - cx * 64.0f);
            int ly = (int)floor(y - cy * 64.0f);
            int lz = (int)floor(z - cz * 64.0f);
            if (lx >= 0 && lx < 32 && ly >= 0 && ly < 32 && lz >= 0 && lz < 32) {
                return it->second->cells[lx][ly][lz];
            }
        }
        return {TerrainMaterial::Air, 0.0f, 0.5f, 0.5f};
    }

    void setCell(float x, float y, float z, TerrainMaterial material, float density) {
        int cx, cy, cz;
        worldToChunk(x, y, z, cx, cy, cz);
        ModernTerrainChunk* chunk = getOrCreateModernChunk(cx, cy, cz);
        int lx = (int)floor(x - cx * 64.0f);
        int ly = (int)floor(y - cy * 64.0f);
        int lz = (int)floor(z - cz * 64.0f);
        if (lx >= 0 && lx < 32 && ly >= 0 && ly < 32 && lz >= 0 && lz < 32) {
            chunk->cells[lx][ly][lz].material = material;
            chunk->cells[lx][ly][lz].density = density;
            chunk->dirty = true;
        }
    }

    float getCellDensity(float x, float y, float z) {
        return getCell(x, y, z).density;
    }

    BrushSettings getDefaultBrush() {
        return {4.0f, 1.0f, TerrainMaterial::Grass, BrushType::Add, false};
    }

    void applyBrush(float x, float y, float z, const BrushSettings& settings) {
        int radius = (int)ceil(settings.radius);
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dz = -radius; dz <= radius; dz++) {
                    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                    if (dist > settings.radius) continue;
                    float falloff = 1.0f - (dist / settings.radius);
                    falloff = falloff * settings.strength;
                    int px = (int)round(x + dx);
                    int py = (int)round(y + dy);
                    int pz = (int)round(z + dz);
                    switch (settings.type) {
                        case BrushType::Add:
                            if (currentTerrainType == TerrainType::ClassicVoxel) {
                                setVoxel((float)px, (float)py, (float)pz, settings.material);
                            } else {
                                setCell((float)px, (float)py, (float)pz, settings.material, 1.0f);
                            }
                            break;
                        case BrushType::Subtract:
                            if (currentTerrainType == TerrainType::ClassicVoxel) {
                                setVoxel((float)px, (float)py, (float)pz, TerrainMaterial::Air);
                            } else {
                                setCell((float)px, (float)py, (float)pz, TerrainMaterial::Air, 0.0f);
                            }
                            break;
                        case BrushType::Paint: {
                            TerrainCell cell = getCell((float)px, (float)py, (float)pz);
                            if (cell.density > 0.0f) {
                                setCell((float)px, (float)py, (float)pz, settings.material, cell.density);
                            }
                            break;
                        }
                        case BrushType::Smooth:
                            // Average density with neighbors
                            break;
                        case BrushType::Flatten:
                            if (py == (int)round(y)) {
                                if (currentTerrainType == TerrainType::ClassicVoxel) {
                                    setVoxel((float)px, (float)py, (float)pz, settings.material);
                                } else {
                                    setCell((float)px, (float)py, (float)pz, settings.material, 1.0f);
                                }
                            }
                            break;
                        case BrushType::Erode: {
                            TerrainCell cell = getCell((float)px, (float)py, (float)pz);
                            cell.density -= falloff * 0.1f;
                            if (cell.density < 0.0f) cell.density = 0.0f;
                            setCell((float)px, (float)py, (float)pz, cell.material, cell.density);
                            break;
                        }
                        case BrushType::Grow: {
                            TerrainCell cell = getCell((float)px, (float)py, (float)pz);
                            cell.density += falloff * 0.1f;
                            if (cell.density > 1.0f) cell.density = 1.0f;
                            setCell((float)px, (float)py, (float)pz, cell.material, cell.density);
                            break;
                        }
                    }
                }
            }
        }
        updateChunksAround(x, y, z);
    }

    void generateTerrain(const GenerateOptions& options) {
        classicChunks.clear();
        modernChunks.clear();
        const int size = 128; // Generate 128x128 studs
        for (int x = 0; x < size; x++) {
            for (int z = 0; z < size; z++) {
                float height;
                if (options.flat) {
                    height = options.flatHeight;
                } else {
                    height = noise3D(x * options.scale, 0, z * options.scale, options.seed);
                    height = (height + 1.0f) * 0.5f;
                    height *= options.amplitude;
                }
                int hy = (int)round(height);
                for (int y = 0; y <= hy; y++) {
                    TerrainMaterial mat;
                    if (y == hy) {
                        mat = TerrainMaterial::Grass;
                    } else if (y > hy - 3) {
                        mat = TerrainMaterial::Mud;
                    } else {
                        mat = TerrainMaterial::Rock;
                    }
                    if (currentTerrainType == TerrainType::ClassicVoxel) {
                        setVoxel((float)x, (float)y, (float)z, mat);
                    } else {
                        setCell((float)x, (float)y, (float)z, mat, 1.0f);
                    }
                }
            }
        }
        forceRebuildAllChunks();
    }

    void flattenRegion(const TerrainRegion& region, float targetY) {
        int minX = (int)region.minX, maxX = (int)region.maxX;
        int minY = (int)region.minY, maxY = (int)region.maxY;
        int minZ = (int)region.minZ, maxZ = (int)region.maxZ;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                for (int z = minZ; z <= maxZ; z++) {
                    if (y == (int)targetY) {
                        if (currentTerrainType == TerrainType::ClassicVoxel) {
                            setVoxel((float)x, (float)y, (float)z, TerrainMaterial::Grass);
                        } else {
                            setCell((float)x, (float)y, (float)z, TerrainMaterial::Grass, 1.0f);
                        }
                    } else {
                        if (currentTerrainType == TerrainType::ClassicVoxel) {
                            setVoxel((float)x, (float)y, (float)z, TerrainMaterial::Air);
                        } else {
                            setCell((float)x, (float)y, (float)z, TerrainMaterial::Air, 0.0f);
                        }
                    }
                }
            }
        }
        updateChunksAround((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f);
    }

    void smoothRegion(const TerrainRegion& region, float strength) {
        int minX = (int)region.minX, maxX = (int)region.maxX;
        int minY = (int)region.minY, maxY = (int)region.maxY;
        int minZ = (int)region.minZ, maxZ = (int)region.maxZ;
        for (int x = minX + 1; x < maxX; x++) {
            for (int y = minY + 1; y < maxY; y++) {
                for (int z = minZ + 1; z < maxZ; z++) {
                    if (currentTerrainType != TerrainType::ClassicVoxel) {
                        TerrainCell cell = getCell((float)x, (float)y, (float)z);
                        float avg = 0.0f;
                        int count = 0;
                        for (int dx = -1; dx <= 1; dx++) {
                            for (int dy = -1; dy <= 1; dy++) {
                                for (int dz = -1; dz <= 1; dz++) {
                                    avg += getCellDensity((float)(x + dx), (float)(y + dy), (float)(z + dz));
                                    count++;
                                }
                            }
                        }
                        avg /= count;
                        cell.density = cell.density * (1.0f - strength) + avg * strength;
                        setCell((float)x, (float)y, (float)z, cell.material, cell.density);
                    }
                }
            }
        }
        updateChunksAround((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f);
    }

    void selectRegion(const TerrainRegion& region) {
        selectedRegion = region;
        selectedRegion.selected = true;
    }

    void clearSelection() {
        selectedRegion.selected = false;
    }

    void copySelection() {
        if (!selectedRegion.selected) return;
        copiedVoxels.clear();
        copiedCells.clear();
        copyRegion = selectedRegion;
        int minX = (int)selectedRegion.minX, maxX = (int)selectedRegion.maxX;
        int minY = (int)selectedRegion.minY, maxY = (int)selectedRegion.maxY;
        int minZ = (int)selectedRegion.minZ, maxZ = (int)selectedRegion.maxZ;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                for (int z = minZ; z <= maxZ; z++) {
                    if (currentTerrainType == TerrainType::ClassicVoxel) {
                        copiedVoxels.push_back(getVoxel((float)x, (float)y, (float)z));
                    } else {
                        copiedCells.push_back(getCell((float)x, (float)y, (float)z));
                    }
                }
            }
        }
    }

    void pasteSelection(float offsetX, float offsetY, float offsetZ) {
        if (currentTerrainType == TerrainType::ClassicVoxel && !copiedVoxels.empty()) {
            int dx = (int)copyRegion.maxX - (int)copyRegion.minX;
            int dy = (int)copyRegion.maxY - (int)copyRegion.minY;
            int dz = (int)copyRegion.maxZ - (int)copyRegion.minZ;
            int idx = 0;
            for (int x = 0; x <= dx; x++) {
                for (int y = 0; y <= dy; y++) {
                    for (int z = 0; z <= dz; z++) {
                        int px = (int)offsetX + x;
                        int py = (int)offsetY + y;
                        int pz = (int)offsetZ + z;
                        if (idx < (int)copiedVoxels.size()) {
                            setVoxel((float)px, (float)py, (float)pz, copiedVoxels[idx].material, copiedVoxels[idx].color);
                        }
                        idx++;
                    }
                }
            }
        } else if (currentTerrainType != TerrainType::ClassicVoxel && !copiedCells.empty()) {
            int dx = (int)copyRegion.maxX - (int)copyRegion.minX;
            int dy = (int)copyRegion.maxY - (int)copyRegion.minY;
            int dz = (int)copyRegion.maxZ - (int)copyRegion.minZ;
            int idx = 0;
            for (int x = 0; x <= dx; x++) {
                for (int y = 0; y <= dy; y++) {
                    for (int z = 0; z <= dz; z++) {
                        int px = (int)offsetX + x;
                        int py = (int)offsetY + y;
                        int pz = (int)offsetZ + z;
                        if (idx < (int)copiedCells.size()) {
                            setCell((float)px, (float)py, (float)pz, copiedCells[idx].material, copiedCells[idx].density);
                        }
                        idx++;
                    }
                }
            }
        }
        updateChunksAround(offsetX, offsetY, offsetZ);
    }

    void deleteSelection() {
        if (!selectedRegion.selected) return;
        int minX = (int)selectedRegion.minX, maxX = (int)selectedRegion.maxX;
        int minY = (int)selectedRegion.minY, maxY = (int)selectedRegion.maxY;
        int minZ = (int)selectedRegion.minZ, maxZ = (int)selectedRegion.maxZ;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                for (int z = minZ; z <= maxZ; z++) {
                    if (currentTerrainType == TerrainType::ClassicVoxel) {
                        setVoxel((float)x, (float)y, (float)z, TerrainMaterial::Air);
                    } else {
                        setCell((float)x, (float)y, (float)z, TerrainMaterial::Air, 0.0f);
                    }
                }
            }
        }
        updateChunksAround((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f);
    }

    TerrainRegion getSelectedRegion() {
        return selectedRegion;
    }

    void setSeaLevel(float height) {
        seaLevel.height = height;
    }

    float getSeaLevel() {
        return seaLevel.height;
    }

    void enableWater(bool enable) {
        seaLevel.enableWater = enable;
    }

    void convertPartsToTerrain(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) {
        for (int x = (int)minX; x <= (int)maxX; x++) {
            for (int y = (int)minY; y <= (int)maxY; y++) {
                for (int z = (int)minZ; z <= (int)maxZ; z++) {
                    if (currentTerrainType == TerrainType::ClassicVoxel) {
                        setVoxel((float)x, (float)y, (float)z, TerrainMaterial::Concrete);
                    } else {
                        setCell((float)x, (float)y, (float)z, TerrainMaterial::Concrete, 1.0f);
                    }
                }
            }
        }
        updateChunksAround((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f);
    }

    void updateChunksAround(float x, float y, float z) {
        int cx, cy, cz;
        worldToChunk(x, y, z, cx, cy, cz);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    std::string key = chunkKey(cx + dx, cy + dy, cz + dz);
                    if (currentTerrainType == TerrainType::ClassicVoxel) {
                        auto it = classicChunks.find(key);
                        if (it != classicChunks.end()) it->second->dirty = true;
                    } else {
                        auto it = modernChunks.find(key);
                        if (it != modernChunks.end()) it->second->dirty = true;
                    }
                }
            }
        }
    }

    int getActiveChunkCount() {
        return (int)classicChunks.size() + (int)modernChunks.size();
    }

    void forceRebuildAllChunks() {
        for (auto& pair : classicChunks) pair.second->dirty = true;
        for (auto& pair : modernChunks) pair.second->dirty = true;
    }

    // Generate mesh for classic terrain chunk using greedy meshing
    static void generateClassicChunkMesh(ClassicTerrainChunk* chunk, std::vector<float>& vertices, std::vector<float>& normals, std::vector<float>& uvs, std::vector<uint32_t>& indices) {
        int baseX = chunk->chunkX * 64;
        int baseY = chunk->chunkY * 64;
        int baseZ = chunk->chunkZ * 64;
        uint32_t vertStart = vertices.size() / 6;
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 16; y++) {
                for (int z = 0; z < 16; z++) {
                    Voxel& voxel = chunk->voxels[x][y][z];
                    if (!voxel.isSolid) continue;
                    // Check neighbors for visible faces
                    int px = baseX + x * 4;
                    int py = baseY + y * 4;
                    int pz = baseZ + z * 4;
                    // Top face
                    bool top = (y == 15) || !chunk->voxels[x][y + 1][z].isSolid;
                    if (top) {
                        vertices.insert(vertices.end(), {
                            (float)px, (float)py + 4, (float)pz,
                            (float)px + 4, (float)py + 4, (float)pz,
                            (float)px + 4, (float)py + 4, (float)pz + 4,
                            (float)px, (float)py + 4, (float)pz + 4
                        });
                        normals.insert(normals.end(), {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0});
                        uvs.insert(uvs.end(), {0, 0, 1, 0, 1, 1, 0, 1});
                        indices.insert(indices.end(), {vertStart, vertStart + 1, vertStart + 2, vertStart, vertStart + 2, vertStart + 3});
                        vertStart += 4;
                    }
                    // Similar faces for bottom, left, right, front, back could be added
                    // For now, top-only rendering for performance on 3DS
                }
            }
        }
    }

    // Render terrain using chunk meshes
    void renderTerrain(float camX, float camY, float camZ) {
        const float renderDist = 256.0f;
        const float renderDistSq = renderDist * renderDist;

        if (currentTerrainType == TerrainType::ClassicVoxel) {
            std::vector<float> allVertices, allNormals, allUvs;
            std::vector<uint32_t> allIndices;
            for (auto& pair : classicChunks) {
                ClassicTerrainChunk* chunk = pair.second;
                int cx = chunk->chunkX * 64 + 32;
                int cy = chunk->chunkY * 64 + 32;
                int cz = chunk->chunkZ * 64 + 32;
                float dx = cx - camX, dy = cy - camY, dz = cz - camZ;
                if (dx*dx + dy*dy + dz*dz > renderDistSq) continue;
                if (chunk->dirty) {
                    generateClassicChunkMesh(chunk, allVertices, allNormals, allUvs, allIndices);
                    chunk->dirty = false;
                }
            }
            // TODO: Proper terrain rendering with GX2 buffers
        }
    }

    void renderTerrainWireframe(bool show) {
        showWireframe = show;
    }

    std::string getMaterialTexturePath(TerrainMaterial mat, const std::string& contentVersion) {
        const char* name = "air";
        switch (mat) {
            case TerrainMaterial::Plastic:      name = "plastic"; break;
            case TerrainMaterial::SmoothPlastic: name = "smoothplastic"; break;
            case TerrainMaterial::Neon:         name = "neon"; break;
            case TerrainMaterial::Wood:         name = "wood"; break;
            case TerrainMaterial::WoodPlanks:   name = "woodplanks"; break;
            case TerrainMaterial::Marble:       name = "marble"; break;
            case TerrainMaterial::Basalt:       name = "basalt"; break;
            case TerrainMaterial::Slate:        name = "slate"; break;
            case TerrainMaterial::CrackedLava:  name = "crackedlava"; break;
            case TerrainMaterial::Concrete:     name = "concrete"; break;
            case TerrainMaterial::Limestone:    name = "limestone"; break;
            case TerrainMaterial::Granite:      name = "granite"; break;
            case TerrainMaterial::Pavement:     name = "pavement"; break;
            case TerrainMaterial::Brick:        name = "brick"; break;
            case TerrainMaterial::Pebble:       name = "pebble"; break;
            case TerrainMaterial::Cobblestone:  name = "cobblestone"; break;
            case TerrainMaterial::Rock:         name = "rock"; break;
            case TerrainMaterial::Sandstone:    name = "sandstone"; break;
            case TerrainMaterial::Glass:        name = "glass"; break;
            case TerrainMaterial::Grass:        name = "grass"; break;
            case TerrainMaterial::LeafyGrass:   name = "leafygrass"; break;
            case TerrainMaterial::Sand:         name = "sand"; break;
            case TerrainMaterial::Fabric:       name = "fabric"; break;
            case TerrainMaterial::Snow:         name = "snow"; break;
            case TerrainMaterial::Mud:          name = "mud"; break;
            case TerrainMaterial::Ground:       name = "ground"; break;
            case TerrainMaterial::Asphalt:      name = "asphalt"; break;
            case TerrainMaterial::Salt:         name = "salt"; break;
            case TerrainMaterial::Ice:          name = "ice"; break;
            case TerrainMaterial::Glacier:      name = "glacier"; break;
            case TerrainMaterial::ForceField:   name = "forcefield"; break;
            case TerrainMaterial::Air:          name = "air"; break;
            case TerrainMaterial::Water:        name = "water"; break;
            default: break;
        }
        return contentVersion + "/textures/TerrainTools/mtrl_" + name + ".png";
    }

    u32 getMaterialColor(TerrainMaterial mat) {
        switch (mat) {
            case TerrainMaterial::Plastic:      return 0xFF666666; // Gray plastic
            case TerrainMaterial::SmoothPlastic:return 0xFF666666;
            case TerrainMaterial::Neon:         return 0xFFFF00FF; // Magenta
            case TerrainMaterial::Wood:         return 0xFFA0522D; // Sienna
            case TerrainMaterial::WoodPlanks:   return 0xFFA0522D;
            case TerrainMaterial::Marble:       return 0xFFF0F0F0; // White marble
            case TerrainMaterial::Basalt:       return 0xFF3A3A3A; // Dark gray
            case TerrainMaterial::Slate:        return 0xFF708090; // Slate gray
            case TerrainMaterial::CrackedLava:  return 0xFF402000; // Dark brown
            case TerrainMaterial::Concrete:     return 0xFF888888; // Gray
            case TerrainMaterial::Limestone:    return 0xFFDCDCB0; // Beige
            case TerrainMaterial::Granite:      return 0xFF808080; // Gray
            case TerrainMaterial::Pavement:     return 0xFF696969; // Dim gray
            case TerrainMaterial::Brick:        return 0xFFB22222; // Fire brick red
            case TerrainMaterial::Pebble:       return 0xFFA9A9A9; // Dark gray
            case TerrainMaterial::Cobblestone:  return 0xFF696969; // Dim gray
            case TerrainMaterial::Rock:         return 0xFF666666; // Medium gray
            case TerrainMaterial::Sandstone:    return 0xFFEEDC82; // Burlywood
            case TerrainMaterial::Glass:        return 0xFFB0E0E6; // Light blue
            case TerrainMaterial::Grass:        return 0xFF5A8B2C; // Green
            case TerrainMaterial::LeafyGrass:   return 0xFF228B22; // Forest green
            case TerrainMaterial::Sand:         return 0xFFE8D5A3; // Tan
            case TerrainMaterial::Fabric:       return 0xFFFFB0D0; // Pinkish
            case TerrainMaterial::Snow:         return 0xFFFFFFFF; // White
            case TerrainMaterial::Mud:          return 0xFF8B6914; // Brown
            case TerrainMaterial::Ground:       return 0xFF8B4513; // Saddle brown
            case TerrainMaterial::Asphalt:      return 0xFF444444; // Dark gray
            case TerrainMaterial::Salt:         return 0xFFFFFFFF; // White
            case TerrainMaterial::Ice:          return 0xFFB0E0E6; // Powder blue
            case TerrainMaterial::Glacier:      return 0xFFE0E0FF; // Blizzard blue
            case TerrainMaterial::ForceField:   return 0xFF80C0FF; // Light blue
            case TerrainMaterial::Air:          return 0xFF000000; // Transparent
            case TerrainMaterial::Water:        return 0xFF4169E1; // Royal blue
            default: return 0xFF888888;
        }
    }

    void loadTerrainFromFile(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return;
        uint32_t version;
        if (fread(&version, sizeof(version), 1, f) != 1) { fclose(f); return; }
        if (version == 1) {
            // Classic voxel format
            uint32_t count;
            fread(&count, sizeof(count), 1, f);
            for (uint32_t i = 0; i < count; i++) {
                float x, y, z;
                uint32_t mat, color;
                fread(&x, sizeof(float), 1, f);
                fread(&y, sizeof(float), 1, f);
                fread(&z, sizeof(float), 1, f);
                fread(&mat, sizeof(uint32_t), 1, f);
                fread(&color, sizeof(uint32_t), 1, f);
                setVoxel(x, y, z, (TerrainMaterial)mat, color);
            }
        } else if (version == 2) {
            // Modern cell format
            uint32_t count;
            fread(&count, sizeof(count), 1, f);
            for (uint32_t i = 0; i < count; i++) {
                float x, y, z, density;
                uint32_t mat;
                fread(&x, sizeof(float), 1, f);
                fread(&y, sizeof(float), 1, f);
                fread(&z, sizeof(float), 1, f);
                fread(&mat, sizeof(uint32_t), 1, f);
                fread(&density, sizeof(float), 1, f);
                setCell(x, y, z, (TerrainMaterial)mat, density);
            }
        }
        fclose(f);
        forceRebuildAllChunks();
    }

    void saveTerrainToFile(const std::string& path) {
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) return;
        uint32_t version = (currentTerrainType == TerrainType::ClassicVoxel) ? 1 : 2;
        fwrite(&version, sizeof(version), 1, f);
        if (currentTerrainType == TerrainType::ClassicVoxel) {
            uint32_t count = 0;
            for (auto& pair : classicChunks) {
                ClassicTerrainChunk* chunk = pair.second;
                for (int x = 0; x < 16; x++)
                    for (int y = 0; y < 16; y++)
                        for (int z = 0; z < 16; z++)
                            if (chunk->voxels[x][y][z].isSolid) count++;
            }
            fwrite(&count, sizeof(count), 1, f);
            for (auto& pair : classicChunks) {
                ClassicTerrainChunk* chunk = pair.second;
                int baseX = chunk->chunkX * 64;
                int baseY = chunk->chunkY * 64;
                int baseZ = chunk->chunkZ * 64;
                for (int x = 0; x < 16; x++) {
                    for (int y = 0; y < 16; y++) {
                        for (int z = 0; z < 16; z++) {
                            const Voxel& v = chunk->voxels[x][y][z];
                            if (!v.isSolid) continue;
                            float fx = (float)(baseX + x * 4);
                            float fy = (float)(baseY + y * 4);
                            float fz = (float)(baseZ + z * 4);
                            uint32_t mat = (uint32_t)v.material;
                            fwrite(&fx, sizeof(float), 1, f);
                            fwrite(&fy, sizeof(float), 1, f);
                            fwrite(&fz, sizeof(float), 1, f);
                            fwrite(&mat, sizeof(uint32_t), 1, f);
                            fwrite(&v.color, sizeof(uint32_t), 1, f);
                        }
                    }
                }
            }
        }
        fclose(f);
    }

    void shutdown() {
        for (auto& pair : classicChunks) delete pair.second;
        classicChunks.clear();
        for (auto& pair : modernChunks) delete pair.second;
        modernChunks.clear();
        copiedVoxels.clear();
        copiedCells.clear();
        memset(&selectedRegion, 0, sizeof(selectedRegion));
    }
}