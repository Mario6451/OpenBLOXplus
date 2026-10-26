#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <3ds.h>
#include <citro3d.h>

// Terrain system supporting classic voxel (2011-2016) and modern (2016+) terrain

namespace Terrain {

    enum class TerrainType {
        None,           // 2007 (no terrain existed)
        ClassicVoxel,   // 2012 (voxeltemplate.rbxl)
        Smooth2015,     // 2015 (smoothtemplate2015.rbxl)
        Smooth2016,     // 2016 (smoothtemplate2016.rbxl)
        Smooth2019,     // 2019
        Smooth2022,     // 2022 (smoothtemplate2022.rbxl)
    };


    // Terrain materials (matches Roblox Enum.Material exactly)
    // Values from https://create.roblox.com/docs/en-us/reference/engine/enums/Material
    enum class TerrainMaterial {
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
        Cardboard      = 2304,  // BasePart only
        Carpet         = 2305,  // BasePart only
        CeramicTiles   = 2306,  // BasePart only
        ClayRoofTiles  = 2307,  // BasePart only
        RoofShingles   = 2308,  // BasePart only
        Leather        = 2309,  // BasePart only
        Plaster        = 2310,  // BasePart only
        Rubber         = 2311,  // BasePart only
    };

    // Voxel data for classic terrain
    struct Voxel {
        TerrainMaterial material;
        bool           isSolid;
        u32            color; // For classic colored voxels
    };

    // Classic voxel terrain (2005-2011)
    // 4-stud resolution blocks
    struct ClassicTerrainChunk {
        int chunkX, chunkY, chunkZ;
        Voxel voxels[16][16][16]; // 64x64x64 studs per chunk
        bool dirty; // Needs mesh rebuild
    };

    // Modern terrain cell (2015+)
    // Higher resolution sculpting
    struct TerrainCell {
        TerrainMaterial material;
        float density;       // 0.0 (air) to 1.0 (solid)
        float temperature;   // Affects snow/ice formation
        float moisture;      // Affects sand/clay formation
    };

    // Modern terrain chunk
    struct ModernTerrainChunk {
        int chunkX, chunkY, chunkZ;
        TerrainCell cells[32][32][32];
        bool dirty;
    };

    // Brush types for terrain editing
    enum class BrushType {
        Add,          // Add terrain material
        Subtract,     // Remove terrain (carve air)
        Paint,        // Paint surface material
        Smooth,       // Smooth terrain transitions
        Flatten,      // Flatten to plane
        Erode,        // Erode surface
        Grow,         // Grow surface outward
    };

    // Brush settings
    struct BrushSettings {
        float radius;           // Brush radius in studs
        float strength;         // 0.0 to 1.0
        TerrainMaterial material;
        BrushType type;
        bool smooth;
    };

    // Terrain region for selection/manipulation
    struct TerrainRegion {
        float minX, minY, minZ;
        float maxX, maxY, maxZ;
        bool selected;
    };

    // Sea level setting
    struct SeaLevel {
        float height;
        bool enableWater;
        TerrainMaterial waterMaterial;
    };

    // Generate terrain options
    struct GenerateOptions {
        float scale;            // Terrain scale
        float amplitude;        // Height amplitude
        float roughness;        // Noise roughness
        int octaves;            // Noise octaves
        float seed;             // Random seed
        bool flat;              // Generate flat terrain
        float flatHeight;       // Height for flat terrain
    };

    // ==================== API ====================

    // Initialize terrain system
    void init();

    // Set terrain mode (classic vs modern)
    void setTerrainType(TerrainType type);
    TerrainType getTerrainType();

    // Load/save terrain data
    void loadTerrainFromFile(const std::string& path);
    void saveTerrainToFile(const std::string& path);

    // Classic voxel terrain functions
    Voxel getVoxel(float x, float y, float z);
    void setVoxel(float x, float y, float z, TerrainMaterial material, u32 color = 0xFFFFFFFF);
    bool isVoxelSolid(float x, float y, float z);

    // Modern terrain functions
    TerrainCell getCell(float x, float y, float z);
    void setCell(float x, float y, float z, TerrainMaterial material, float density);
    float getCellDensity(float x, float y, float z);

    // Brush operations
    void applyBrush(float x, float y, float z, const BrushSettings& settings);
    BrushSettings getDefaultBrush();

    // Terrain generation
    void generateTerrain(const GenerateOptions& options);
    void flattenRegion(const TerrainRegion& region, float targetY);
    void smoothRegion(const TerrainRegion& region, float strength);

    // Region operations
    void selectRegion(const TerrainRegion& region);
    void clearSelection();
    void copySelection();
    void pasteSelection(float offsetX, float offsetY, float offsetZ);
    void deleteSelection();
    TerrainRegion getSelectedRegion();

    // Sea level
    void setSeaLevel(float height);
    float getSeaLevel();
    void enableWater(bool enable);

    // Convert parts to terrain
    void convertPartsToTerrain(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

    // Render terrain
    void renderTerrain(float camX, float camY, float camZ);
    void renderTerrainWireframe(bool show);

    // Chunk management
    void updateChunksAround(float x, float y, float z);
    int getActiveChunkCount();
    void forceRebuildAllChunks();

    // Material mapping
    std::string getMaterialTexturePath(TerrainMaterial mat, const std::string& contentVersion);
    u32 getMaterialColor(TerrainMaterial mat);

    // Cleanup
    void shutdown();
};