#include "materials.h"
#include "content_system.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>

namespace Materials {

    static std::map<MaterialType, MaterialProperties> gMaterialProps;
    static std::map<SurfaceType, SurfaceMeshData> gSurfaceMeshes;

    // Initialize material properties with defaults
    static void initMaterialProperties() {
        // Plastic - default Roblox material
        MaterialProperties plastic;
        plastic.type = MaterialType::Plastic;
        plastic.name = "Plastic";
        plastic.roughness = 0.5f;
        plastic.metallic = 0.0f;
        plastic.reflectance = 0.3f;
        plastic.ambient = 0.8f;
        plastic.specular = 0.5f;
        plastic.defaultColor = 0xFF0000FF;
        plastic.hasNormalMap = false;
        plastic.hasDiffuseMap = false;
        plastic.friction = 0.7f;
        plastic.elasticity = 0.1f;
        plastic.density = 1.0f;
        gMaterialProps[MaterialType::Plastic] = plastic;

        // Wood
        MaterialProperties wood;
        wood.type = MaterialType::Wood;
        wood.name = "Wood";
        wood.roughness = 0.7f;
        wood.metallic = 0.0f;
        wood.reflectance = 0.1f;
        wood.ambient = 0.7f;
        wood.specular = 0.2f;
        wood.defaultColor = 0xFF8B4513;
        wood.hasNormalMap = true;
        wood.hasDiffuseMap = true;
        wood.friction = 0.8f;
        wood.elasticity = 0.05f;
        wood.density = 0.8f;
        gMaterialProps[MaterialType::Wood] = wood;

        // Stone (mapped to Slate in official Roblox enum)
        MaterialProperties stone;
        stone.type = MaterialType::Slate;
        stone.name = "Slate";
        stone.roughness = 0.8f;
        stone.metallic = 0.0f;
        stone.reflectance = 0.1f;
        stone.ambient = 0.6f;
        stone.specular = 0.1f;
        stone.defaultColor = 0xFF808080;
        stone.hasNormalMap = true;
        stone.hasDiffuseMap = true;
        stone.friction = 0.9f;
        stone.elasticity = 0.0f;
        stone.density = 2.0f;
        gMaterialProps[MaterialType::Slate] = stone;

        // Ice - very slippery, low friction
        MaterialProperties ice;
        ice.type = MaterialType::Ice;
        ice.name = "Ice";
        ice.roughness = 0.1f;
        ice.metallic = 0.0f;
        ice.reflectance = 0.7f;
        ice.ambient = 0.9f;
        ice.specular = 0.8f;
        ice.defaultColor = 0xFFADD8E6;
        ice.hasNormalMap = false;
        ice.hasDiffuseMap = false;
        ice.friction = 0.05f;
        ice.elasticity = 0.3f;
        ice.density = 0.5f;
        gMaterialProps[MaterialType::Ice] = ice;

        // Metal - bouncy
        MaterialProperties metal;
        metal.type = MaterialType::Metal;
        metal.name = "Metal";
        metal.roughness = 0.3f;
        metal.metallic = 1.0f;
        metal.reflectance = 0.6f;
        metal.ambient = 0.5f;
        metal.specular = 0.8f;
        metal.defaultColor = 0xFFB0B0B0;
        metal.hasNormalMap = true;
        metal.hasDiffuseMap = false;
        metal.friction = 0.4f;
        metal.elasticity = 0.3f;
        metal.density = 3.0f;
        gMaterialProps[MaterialType::Metal] = metal;

        // Glass - slippery
        MaterialProperties glass;
        glass.type = MaterialType::Glass;
        glass.name = "Glass";
        glass.roughness = 0.0f;
        glass.metallic = 0.0f;
        glass.reflectance = 0.9f;
        glass.ambient = 1.0f;
        glass.specular = 0.9f;
        glass.defaultColor = 0xFFE0E0E0;
        glass.hasNormalMap = false;
        glass.hasDiffuseMap = false;
        glass.friction = 0.1f;
        glass.elasticity = 0.2f;
        glass.density = 1.5f;
        gMaterialProps[MaterialType::Glass] = glass;

        // Grass - grippy
        MaterialProperties grass;
        grass.type = MaterialType::Grass;
        grass.name = "Grass";
        grass.roughness = 0.6f;
        grass.metallic = 0.0f;
        grass.reflectance = 0.0f;
        grass.ambient = 0.7f;
        grass.specular = 0.1f;
        grass.defaultColor = 0xFF228B22;
        grass.hasNormalMap = true;
        grass.hasDiffuseMap = true;
        grass.friction = 0.9f;
        grass.elasticity = 0.0f;
        grass.density = 0.5f;
        gMaterialProps[MaterialType::Grass] = grass;

        // Sand - grippy, heavy
        MaterialProperties sand;
        sand.type = MaterialType::Sand;
        sand.name = "Sand";
        sand.roughness = 0.9f;
        sand.metallic = 0.0f;
        sand.reflectance = 0.0f;
        sand.ambient = 0.7f;
        sand.specular = 0.0f;
        sand.defaultColor = 0xFFF4A460;
        sand.hasNormalMap = false;
        sand.hasDiffuseMap = true;
        sand.friction = 0.95f;
        sand.elasticity = 0.0f;
        sand.density = 1.5f;
        gMaterialProps[MaterialType::Sand] = sand;

        // Snow - somewhat slippery
        MaterialProperties snow;
        snow.type = MaterialType::Snow;
        snow.name = "Snow";
        snow.roughness = 0.5f;
        snow.metallic = 0.0f;
        snow.reflectance = 0.2f;
        snow.ambient = 0.9f;
        snow.specular = 0.3f;
        snow.defaultColor = 0xFFFFFFFF;
        snow.hasNormalMap = false;
        snow.hasDiffuseMap = false;
        snow.friction = 0.3f;
        snow.elasticity = 0.05f;
        snow.density = 0.3f;
        gMaterialProps[MaterialType::Snow] = snow;

        // Neon
        MaterialProperties neon;
        neon.type = MaterialType::Neon;
        neon.name = "Neon";
        neon.roughness = 0.0f;
        neon.metallic = 0.0f;
        neon.reflectance = 0.0f;
        neon.ambient = 1.0f;
        neon.specular = 1.0f;
        neon.defaultColor = 0xFFFF00FF;
        neon.hasNormalMap = false;
        neon.hasDiffuseMap = false;
        neon.friction = 0.5f;
        neon.elasticity = 0.2f;
        neon.density = 1.0f;
        gMaterialProps[MaterialType::Neon] = neon;

        // ForceField - non-colliding material
        MaterialProperties forceField;
        forceField.type = MaterialType::ForceField;
        forceField.name = "ForceField";
        forceField.roughness = 0.0f;
        forceField.metallic = 0.0f;
        forceField.reflectance = 0.8f;
        forceField.ambient = 1.0f;
        forceField.specular = 1.0f;
        forceField.defaultColor = 0xFF80C0FF;
        forceField.hasNormalMap = false;
        forceField.hasDiffuseMap = false;
        forceField.friction = 0.5f;
        forceField.elasticity = 0.5f;
        forceField.density = 0.1f;
        gMaterialProps[MaterialType::ForceField] = forceField;

        // Default for remaining materials
        // Format: {type, name, roughness, metallic, reflectance, ambient, specular, defaultColor, hasNormalMap, hasDiffuseMap, friction, elasticity, density}
        MaterialProperties defaults[] = {
            {MaterialType::SmoothPlastic, "SmoothPlastic", 0.3f, 0.0f, 0.3f, 0.8f, 0.5f, 0xFF666666, false, false, 0.4f, 0.3f, 1.0f},
            {MaterialType::Water, "Water", 0.0f, 0.0f, 0.8f, 0.8f, 0.9f, 0x800000FF, false, false, 0.1f, 0.05f, 1.0f},
            {MaterialType::Ground, "Ground", 0.7f, 0.0f, 0.1f, 0.6f, 0.1f, 0xFF8B4513, true, true, 0.8f, 0.0f, 1.2f},
            {MaterialType::Rock, "Rock", 0.8f, 0.0f, 0.1f, 0.5f, 0.1f, 0xFF808080, false, false, 0.9f, 0.0f, 2.5f},
            {MaterialType::Fabric, "Fabric", 0.8f, 0.0f, 0.0f, 0.7f, 0.1f, 0xFFFFB0D0, false, false, 0.85f, 0.0f, 0.5f},
            {MaterialType::Granite, "Granite", 0.7f, 0.0f, 0.1f, 0.6f, 0.1f, 0xFF808080, true, true, 0.9f, 0.0f, 2.5f},
            {MaterialType::Snow, "Snow", 0.5f, 0.0f, 0.2f, 0.9f, 0.3f, 0xFFFFFFFF, false, false, 0.3f, 0.05f, 0.3f},
            {MaterialType::Marble, "Marble", 0.2f, 0.0f, 0.3f, 0.7f, 0.4f, 0xFFF0F0F0, true, false, 0.3f, 0.2f, 2.7f},
            {MaterialType::Brick, "Brick", 0.7f, 0.0f, 0.1f, 0.6f, 0.1f, 0xFFB22222, true, true, 0.85f, 0.0f, 1.8f},
            {MaterialType::Slate, "Slate", 0.6f, 0.0f, 0.1f, 0.5f, 0.1f, 0xFF707070, true, true, 0.85f, 0.0f, 2.3f},
            {MaterialType::Mud, "Mud", 0.8f, 0.0f, 0.0f, 0.5f, 0.0f, 0xFF6B4226, true, true, 0.95f, 0.0f, 1.3f},
            {MaterialType::Basalt, "Basalt", 0.8f, 0.0f, 0.1f, 0.5f, 0.1f, 0xFF404040, false, false, 0.9f, 0.0f, 2.8f},
            {MaterialType::Ice, "Ice", 0.1f, 0.0f, 0.7f, 0.9f, 0.8f, 0xFFADD8E6, false, false, 0.05f, 0.3f, 0.5f},
            {MaterialType::LeafyGrass, "LeafyGrass", 0.6f, 0.0f, 0.0f, 0.7f, 0.1f, 0xFF228B22, true, true, 0.9f, 0.0f, 0.5f},
            {MaterialType::DiamondPlate, "DiamondPlate", 0.3f, 1.0f, 0.5f, 0.6f, 0.6f, 0xFFC0C0C0, true, false, 0.5f, 0.2f, 3.0f},
            {MaterialType::CrackedLava, "CrackedLava", 0.9f, 0.0f, 0.0f, 0.5f, 0.2f, 0xFF402000, true, false, 0.85f, 0.0f, 2.0f},
            {MaterialType::WoodPlanks, "WoodPlanks", 0.7f, 0.0f, 0.1f, 0.7f, 0.2f, 0xFFA0522D, true, true, 0.75f, 0.05f, 0.8f},
            {MaterialType::Sandstone, "Sandstone", 0.8f, 0.0f, 0.1f, 0.6f, 0.1f, 0xFFEEDC82, true, true, 0.85f, 0.0f, 2.2f},
            {MaterialType::Concrete, "Concrete", 0.8f, 0.0f, 0.1f, 0.6f, 0.1f, 0xFFAAAAAA, true, true, 0.85f, 0.0f, 2.4f},
            {MaterialType::Limestone, "Limestone", 0.8f, 0.0f, 0.1f, 0.6f, 0.1f, 0xFFDCDCB0, true, true, 0.85f, 0.0f, 2.6f},
            {MaterialType::Pavement, "Pavement", 0.7f, 0.0f, 0.1f, 0.6f, 0.1f, 0xFF696969, true, true, 0.9f, 0.0f, 2.4f},
            {MaterialType::Pebble, "Pebble", 0.8f, 0.0f, 0.1f, 0.5f, 0.1f, 0xFFA9A9A9, true, true, 0.85f, 0.0f, 2.4f},
            {MaterialType::Cobblestone, "Cobblestone", 0.9f, 0.0f, 0.1f, 0.5f, 0.1f, 0xFF696969, true, true, 0.9f, 0.0f, 2.4f},
            {MaterialType::CorrodedMetal, "CorrodedMetal", 0.6f, 0.8f, 0.4f, 0.5f, 0.4f, 0xFF4B5320, true, false, 0.5f, 0.1f, 2.8f},
            {MaterialType::Foil, "Foil", 0.2f, 1.0f, 0.8f, 0.5f, 0.9f, 0xFFD3D3D3, true, false, 0.3f, 0.3f, 2.0f},
            {MaterialType::Grass, "Grass", 0.6f, 0.0f, 0.0f, 0.7f, 0.1f, 0xFF32CD32, false, false, 0.9f, 0.0f, 0.5f},
            {MaterialType::Sand, "Sand", 0.9f, 0.0f, 0.0f, 0.7f, 0.1f, 0xFFF4A460, false, false, 0.7f, 0.0f, 1.5f},
            {MaterialType::Asphalt, "Asphalt", 0.8f, 0.0f, 0.0f, 0.5f, 0.1f, 0xFF2F4F4F, false, false, 0.9f, 0.0f, 2.0f},
            {MaterialType::Salt, "Salt", 0.7f, 0.0f, 0.2f, 0.8f, 0.3f, 0xFFFFFFFF, true, true, 0.5f, 0.0f, 2.1f},
            {MaterialType::Glacier, "Glacier", 0.2f, 0.0f, 0.5f, 0.8f, 0.7f, 0xFFB0E0E6, false, false, 0.15f, 0.1f, 0.6f},
            {MaterialType::Air, "Air", 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0x00FFFFFF, false, false, 0.0f, 0.0f, 0.0f},
            {MaterialType::Cardboard, "Cardboard", 0.7f, 0.0f, 0.0f, 0.6f, 0.1f, 0xFF964B00, true, true, 0.8f, 0.0f, 0.5f},
            {MaterialType::Carpet, "Carpet", 0.8f, 0.0f, 0.0f, 0.7f, 0.1f, 0xFFFF0000, true, true, 0.85f, 0.0f, 0.3f},
            {MaterialType::CeramicTiles, "CeramicTiles", 0.2f, 0.0f, 0.5f, 0.6f, 0.6f, 0xFFD0D0D0, true, false, 0.3f, 0.4f, 2.5f},
            {MaterialType::ClayRoofTiles, "ClayRoofTiles", 0.8f, 0.0f, 0.1f, 0.5f, 0.1f, 0xFFB22222, true, true, 0.85f, 0.0f, 2.0f},
            {MaterialType::RoofShingles, "RoofShingles", 0.9f, 0.0f, 0.0f, 0.5f, 0.1f, 0xFF333333, true, true, 0.9f, 0.0f, 1.5f},
            {MaterialType::Leather, "Leather", 0.7f, 0.1f, 0.15f, 0.6f, 0.2f, 0xFF654321, false, false, 0.6f, 0.2f, 1.2f},
            {MaterialType::Plaster, "Plaster", 0.8f, 0.0f, 0.0f, 0.7f, 0.1f, 0xFFF5F5DC, true, true, 0.8f, 0.0f, 1.5f},
            {MaterialType::Rubber, "Rubber", 0.6f, 0.0f, 0.0f, 0.5f, 0.1f, 0xFF000000, false, false, 0.9f, 0.03f, 1.1f},
        };

        for (const auto& def : defaults) {
            gMaterialProps[def.type] = def;
        }

        printf("Material properties initialized: %d types\n", (int)gMaterialProps.size());
    }

    void init() {
        initMaterialProperties();
        gSurfaceMeshes.clear();
        printf("Materials system initialized\n");
    }

    const MaterialProperties& getMaterialProperties(MaterialType type) {
        auto it = gMaterialProps.find(type);
        if (it != gMaterialProps.end()) {
            return it->second;
        }
        return gMaterialProps[MaterialType::Plastic];
    }

    void setMaterialProperties(MaterialType type, const MaterialProperties& props) {
        gMaterialProps[type] = props;
    }

    MaterialInstance createMaterialInstance(MaterialType type, u32 color) {
        MaterialInstance instance;
        instance.type = type;
        instance.properties = getMaterialProperties(type);
        instance.color = color;
        instance.transparency = 0.0f;
        instance.reflectance = instance.properties.reflectance;
        return instance;
    }

    void addTextureLayer(MaterialInstance& instance, const std::string& texturePath, const std::string& normalMapPath) {
        TextureLayer layer;
        layer.texturePath = texturePath;
        layer.normalMapPath = normalMapPath;
        layer.scale = 1.0f;
        layer.offset = 0.0f;
        layer.rotation = 0.0f;
        layer.enabled = true;
        instance.layers.push_back(layer);
    }

    std::string getDefaultTexture(MaterialType type) {
        // Get texture from content system
        std::string texName = "";
        switch (type) {
            case MaterialType::Wood:       texName = "WoodFallback.dds"; break;
            case MaterialType::Slate:      texName = "SlateTile.dds"; break;
            case MaterialType::Ice:        texName = "IceFallback.dds"; break;
            case MaterialType::Metal:      texName = "DiamondPlateFallback.dds"; break;
            case MaterialType::Grass:      texName = "Grass_Texture.jpg"; break;
            case MaterialType::Sand:       texName = "Sand_Normal.dds"; break;
            default:                       texName = ""; break;
        }

        if (!texName.empty()) {
            std::string path = ContentSystem::resolveAsset(texName, ContentSystem::ContentType::Texture);
            if (!path.empty()) return path;
        }

        return "";
    }

    std::string getDefaultNormalMap(MaterialType type) {
        std::string normName = "";
        switch (type) {
            case MaterialType::Wood:       normName = "DPlate_Normal.dds"; break;
            case MaterialType::Slate:      normName = "SlateBorder.dds"; break;
            case MaterialType::Metal:      normName = "DPlate_Normal.dds"; break;
            case MaterialType::Grass:      normName = "Grass_Normal.dds"; break;
            default:                       normName = ""; break;
        }

        if (!normName.empty()) {
            std::string path = ContentSystem::resolveAsset(normName, ContentSystem::ContentType::Texture);
            if (!path.empty()) return path;
        }

        return "";
    }

    // Generate stud cap mesh for a face
    static void generateStudCapMesh(SurfaceType type) {
        // Simple stud cap geometry: 6 vertices (quad) + 4 indices (2 triangles)
        // Stud radius: 0.35 stud, height: 0.1 stud
        const float studRadius = 0.35f;
        const float studHeight = 0.1f;

        // Quad vertices for the stud face
        const float s = studRadius;
        float verts[72] = {
            // Top face quad
            -s, studHeight, -s,  s, studHeight, -s,  s, studHeight, s, -s, studHeight, s,
            // Bottom face quad
            -s, 0.0f, -s,  s, 0.0f, -s,  s, 0.0f, s, -s, 0.0f, s,
            // Front face
            -s, studHeight, -s, -s, 0.0f, -s,  s, 0.0f, -s,  s, studHeight, -s,
            // Back face
            -s, studHeight, s,  s, studHeight, s,  s, 0.0f, s, -s, 0.0f, s,
            // Right face
            s, studHeight, -s,  s, 0.0f, -s,  s, 0.0f, s,  s, studHeight, s,
            // Left face
            -s, studHeight, -s, -s, 0.0f, -s, -s, 0.0f, s, -s, studHeight, s,
        };

        float normals[72] = {
            0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
            0,-1, 0, 0,-1, 0, 0,-1, 0, 0,-1, 0,
            0, 0,-1, 0, 0,-1, 0, 0,-1, 0, 0,-1,
            0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
           -1, 0, 0,-1, 0, 0,-1, 0, 0,-1, 0, 0,
        };

        float uvs[48] = {
            0, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 1, 0, 1, 1, 0, 1,
        };

        int indices[36] = {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            8, 9, 10, 8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23,
        };

        SurfaceMeshData* mesh = new SurfaceMeshData();
        mesh->type = type;
        mesh->vertexCount = 24;
        mesh->indexCount = 36;

        mesh->vertices = new float[24 * 3];
        memcpy(mesh->vertices, verts, sizeof(verts));

        mesh->normals = new float[24 * 3];
        memcpy(mesh->normals, normals, sizeof(normals));

        mesh->uvs = new float[24 * 2];
        memcpy(mesh->uvs, uvs, sizeof(uvs));

        mesh->indices = new int[36];
        memcpy(mesh->indices, indices, sizeof(indices));

        gSurfaceMeshes[type] = *mesh;
        delete mesh;
    }

    SurfaceMeshData* getSurfaceMesh(SurfaceType type) {
        auto it = gSurfaceMeshes.find(type);
        if (it != gSurfaceMeshes.end()) {
            return &it->second;
        }
        return nullptr;
    }

    void generateSurfaceMesh(SurfaceType type) {
        if (type == SurfaceType::Smooth) return;

        // Generate stud/inlet mesh based on surface type
        generateStudCapMesh(type);
    }

    std::string getStudCapTexture() {
        std::string path = ContentSystem::resolveAsset("unified_new.png", ContentSystem::ContentType::Texture);
        if (path.empty()) {
            path = ContentSystem::resolveAsset("unified_studs.png", ContentSystem::ContentType::Texture);
        }
        return path;
    }

    void applyMaterialToMesh(float* vertices, int vertexCount, const MaterialInstance& instance) {
        // Apply texture scale to UV coordinates
        float scale = getTextureScale(instance.type);
        for (int i = 0; i < vertexCount * 2; i += 2) {
            vertices[i] *= scale;     // U
            vertices[i + 1] *= scale; // V
        }
    }

    u32 getMaterialColor(int brickColorId) {
        // Simple brick color to RGBA conversion
        // Use BrickColor enum from character_editor if available
        // For now, return a default blue
        if (brickColorId <= 0 || brickColorId > 255) {
            return 0xFF0000FF; // Default blue
        }

        // Simple color table lookup
        static const u32 colorTable[] = {
            0xFF0000FF,  // 1: BrightBlue
            0xFF78BEFF,  // 2: LightBlue
            0xFF32CD32,  // 3: BrightGreen
            0xFFFFEB00,  // 4: BrightYellow
            0xFFFF3232,  // 5: BrightRed
            0xFFB48C00,  // 6: DarkNoyellow
            0xFF0000BF,  // 7: MediumBlue
            0xFF2C7C5E,  // 8: MediumGreen
            0xFF000000,  // 9: Black
            0xFF464B50,  // 10: DarkGray
            0xFF969696,  // 11: Gray
            0xFFFFFFAA,  // 12: LightYellow
            0xFF8B0000,  // 13: DarkRed
            0xFF00008B,  // 14: DarkBlue
            0xFF006400,  // 15: DarkGreen
            0xFFC800C8,  // 16: BrightViolet
        };

        int idx = brickColorId - 1;
        if (idx >= 0 && idx < (int)sizeof(colorTable) / sizeof(colorTable[0])) {
            return colorTable[idx];
        }

        return 0xFF808080; // Default gray
    }

    float getTextureScale(MaterialType type) {
        switch (type) {
            case MaterialType::Wood:       return 2.0f;
            case MaterialType::WoodPlanks: return 2.0f;
            case MaterialType::Sandstone:  return 2.0f;
            case MaterialType::Slate:      return 2.0f;
            case MaterialType::Concrete:   return 2.0f;
            case MaterialType::Marble:     return 2.0f;
            case MaterialType::Brick:      return 2.0f;
            case MaterialType::Cobblestone:return 2.0f;
            case MaterialType::Limestone:  return 2.0f;
            case MaterialType::Pavement:   return 2.0f;
            case MaterialType::Grass:      return 2.0f;
            case MaterialType::LeafyGrass: return 2.0f;
            case MaterialType::Sand:       return 2.0f;
            case MaterialType::Asphalt:    return 2.0f;
            case MaterialType::Metal:      return 1.0f;
            case MaterialType::DiamondPlate: return 1.5f;
            case MaterialType::CorrodedMetal: return 1.5f;
            case MaterialType::Foil:       return 1.0f;
            case MaterialType::Ice:        return 1.5f;
            case MaterialType::Glass:      return 0.5f;
            case MaterialType::Fabric:     return 2.0f;
            case MaterialType::Snow:       return 2.0f;
            case MaterialType::Mud:        return 2.0f;
            case MaterialType::Ground:     return 2.0f;
            case MaterialType::Rock:       return 2.0f;
            default:                       return 1.0f;
        }
    }

    void shutdown() {
        gMaterialProps.clear();
        gSurfaceMeshes.clear();
        printf("Materials system shutdown\n");
    }

}