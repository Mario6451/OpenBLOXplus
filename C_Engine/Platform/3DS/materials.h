#pragma once

#include <string>
#include <vector>
#include <map>
#include <3ds.h>

// Materials, Surfaces & Textures for 3DS OpenBLOX
// Surface type rendering (studs/inlets), material textures, diffuse maps, normal maps

namespace Materials {

    // Surface types (Roblox Enum)
    enum class SurfaceType {
        Smooth,
        Studs,
        StudsPrev,
        StudsFront,
        StudsBack,
        StudsTop,
        StudsBottom,
        StudsRight,
        StudsLeft,
        Inlets,
        InletsFront,
        InletsBack,
        InletsTop,
        InletsBottom,
        InletsRight,
        InletsLeft,
        Corner,
        NoRing,
        NoRingFront,
        NoRingBack,
        NoRingTop,
        NoRingBottom,
        NoRingRight,
        NoRingLeft,
        FrontStuds,
        BackStuds,
        TopStuds,
        BottomStuds,
        RightStuds,
        LeftStuds,
        FrontInlets,
        BackInlets,
        TopInlets,
        BottomInlets,
        RightInlets,
        LeftInlets,
        FrontStudsBackInlets,
        BackStudsFrontInlets,
        TopStudsBottomInlets,
        BottomStudsTopInlets,
        RightStudsLeftInlets,
        LeftStudsRightInlets
    };

    // Material types (matches Roblox Enum.Material exactly)
    // Values from https://create.roblox.com/docs/en-us/reference/engine/enums/Material
    enum class MaterialType {
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

    // Material properties
    struct MaterialProperties {
        MaterialType type;
        std::string name;
        float roughness;      // 0-1 (0=mirror, 1=very rough)
        float metallic;       // 0-1 (0=non-metal, 1=metal)
        float reflectance;    // 0-1 (0=no reflection, 1=full mirror)
        float ambient;        // 0-1 (0=dark, 1=full ambient)
        float specular;       // 0-1 (0=no specular, 1=full specular)
        u32 defaultColor;     // RGBA
        bool hasNormalMap;
        bool hasDiffuseMap;
        // Physics properties for classic Roblox feel
        float friction;       // 0-1 (0=slippery like ice, 1=grippy)
        float elasticity;     // 0-1 (0=no bounce, 1=very bouncy)
        float density;        // Mass per unit volume (for collision response)
    };

    // Texture layer
    struct TextureLayer {
        std::string texturePath;
        std::string normalMapPath;
        float scale;          // UV scale
        float offset;         // UV offset
        float rotation;       // UV rotation
        bool enabled;
    };

    // Material instance
    struct MaterialInstance {
        MaterialType type;
        MaterialProperties properties;
        std::vector<TextureLayer> layers;
        u32 color;            // Material color override
        float transparency;   // 0-1 (0=opaque, 1=fully transparent)
        bool reflectance;
    };

    // Surface mesh data for studs/inlets
    struct SurfaceMeshData {
        float* vertices;
        float* normals;
        float* uvs;
        int* indices;
        int vertexCount;
        int indexCount;
        SurfaceType type;
    };

    // Initialize materials system
    void init();

    // Get material properties by type
    const MaterialProperties& getMaterialProperties(MaterialType type);

    // Set material properties
    void setMaterialProperties(MaterialType type, const MaterialProperties& props);

    // Create material instance
    MaterialInstance createMaterialInstance(MaterialType type, u32 color = 0xFFFFFFFF);

    // Add texture layer to material instance
    void addTextureLayer(MaterialInstance& instance, const std::string& texturePath, const std::string& normalMapPath = "");

    // Get default texture path for a material
    std::string getDefaultTexture(MaterialType type);

    // Get default normal map path for a material
    std::string getDefaultNormalMap(MaterialType type);

    // Get surface mesh data for a surface type
    SurfaceMeshData* getSurfaceMesh(SurfaceType type);

    // Generate surface mesh for studs/inlets
    void generateSurfaceMesh(SurfaceType type);

    // Get stud cap texture
    std::string getStudCapTexture();

    // Apply material to mesh (UV coordinates for material texture)
    void applyMaterialToMesh(float* vertices, int vertexCount, const MaterialInstance& instance);

    // Get material color from BrickColor
    u32 getMaterialColor(int brickColorId);

    // Get texture scale for a material
    float getTextureScale(MaterialType type);

    // Shutdown
    void shutdown();
}