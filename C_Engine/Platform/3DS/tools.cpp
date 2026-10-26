#include "tools.h"
#include "render_pipeline.h"
#include "texture_loader.h"
#include "content_system.h"
#include "audio_manager.h"
#include "terrain.h"
#include "camera.h"
#include "font_manager.h"
#include <3ds.h>
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <string>

// ============================================================================
// Classic Roblox Studio Tools Implementation
// ============================================================================
namespace ClassicTools {

    // ---- Internal state ----
    static ToolType gCurrentTool = ToolType::None;
    static Selection gSelection;
    static TerrainCursor gTerrainCursor;
    static Terrain::TerrainMaterial gBrushMaterial = Terrain::TerrainMaterial::Grass;
    static BrushSize gBrushSize = BrushSize::Medium;
    static bool gToolToolbarVisible = true;

    // Tool icon names for rendering
    static const char* TOOL_NAMES[] = {
        "None", "Select", "Move", "Scale", "Rotate",
        "Delete", "Hammer", "Clone", "Dropper",
        "Mesh", "Terrain", "Part"
    };

    // Tool icon colors (RGB) for toolbar buttons
    static float TOOL_COLORS[][3] = {
        {0.3f, 0.3f, 0.3f},  // None
        {0.2f, 0.5f, 0.8f},  // Selection (blue)
        {0.8f, 0.5f, 0.2f},  // Move (orange)
        {0.4f, 0.8f, 0.4f},  // Scale (green)
        {0.6f, 0.3f, 0.8f},  // Rotate (purple)
        {0.9f, 0.2f, 0.2f},  // Delete (red)
        {0.7f, 0.7f, 0.2f},  // Hammer (yellow)
        {0.3f, 0.7f, 0.5f},  // Clone (teal)
        {0.8f, 0.4f, 0.6f},  // Dropper (pink)
        {0.5f, 0.5f, 0.5f},  // Mesh (gray)
        {0.5f, 0.7f, 0.2f},  // Terrain (lime)
        {0.4f, 0.4f, 0.8f},  // Part (indigo)
    };

    // ---- Initialization ----
    void init() {
        gCurrentTool = ToolType::None;
        gSelection.active = false;
        gSelection.selectedPart = nullptr;
        gSelection.mode = SelectionMode::None;
        gSelection.boxX = gSelection.boxY = gSelection.boxW = gSelection.boxH = 0;
        gSelection.dragging = false;
        gTerrainCursor.x = gTerrainCursor.y = gTerrainCursor.z = 0;
        gTerrainCursor.onTerrain = false;
        gTerrainCursor.onPart = false;
        gTerrainCursor.material = (int)Terrain::TerrainMaterial::Air;
        gBrushMaterial = Terrain::TerrainMaterial::Grass;
        gBrushSize = BrushSize::Medium;
        gToolToolbarVisible = true;
    }

    void shutdown() {
        gSelection.multiSelection.clear();
        gCurrentTool = ToolType::None;
    }

    // ---- Tool management ----
    void setTool(ToolType tool) {
        gCurrentTool = tool;
        if (tool == ToolType::None) clearSelection();
        // Play click sound
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/click.wav", 0, 0, 0);
    }

    ToolType getCurrentTool() { return gCurrentTool; }

    // ---- Selection ----
    Selection& getSelection() { return gSelection; }

    void selectPart(void* part) {
        gSelection.active = (part != nullptr);
        gSelection.selectedPart = part;
        gSelection.mode = SelectionMode::Single;
        gSelection.multiSelection.clear();
        if (part) gSelection.multiSelection.push_back(part);
    }

    void clearSelection() {
        gSelection.active = false;
        gSelection.selectedPart = nullptr;
        gSelection.multiSelection.clear();
        gSelection.mode = SelectionMode::None;
        gSelection.dragging = false;
    }

    void multiSelectBox(float x1, float y1, float x2, float y2) {
        gSelection.mode = SelectionMode::Box;
        gSelection.boxX = std::min(x1, x2);
        gSelection.boxY = std::min(y1, y2);
        gSelection.boxW = std::abs(x2 - x1);
        gSelection.boxH = std::abs(y2 - y1);
    }

    // ---- Tool actions ----
    void onAction() {
        switch (gCurrentTool) {
            case ToolType::Hammer:
                // Place terrain material at cursor position
                Terrain::setVoxel(gTerrainCursor.x, gTerrainCursor.y, gTerrainCursor.z, gBrushMaterial);
                AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/hammer.wav",
                    gTerrainCursor.x, gTerrainCursor.y, gTerrainCursor.z);
                break;

            case ToolType::Clone:
                cloneSelectedPart();
                break;

            case ToolType::Delete:
                deleteSelectedPart();
                break;

            case ToolType::Dropper:
                // Sample terrain material at cursor
                gBrushMaterial = Terrain::getVoxel(gTerrainCursor.x, gTerrainCursor.y, gTerrainCursor.z).material;
                AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/dropper.wav",
                    gTerrainCursor.x, gTerrainCursor.y, gTerrainCursor.z);
                break;

            case ToolType::Part:
                // Place a new part at cursor position
                createPart(gTerrainCursor.x, gTerrainCursor.y, gTerrainCursor.z,
                    4.0f, 4.0f, 4.0f, 0xFFFF0000); // Default red 4x4x4 part
                break;

            case ToolType::Selection:
                // Select part at cursor (raycast would go here)
                break;

            case ToolType::Move:
            case ToolType::Scale:
            case ToolType::Rotate:
                // These tools require drag input, handled in processInput
                break;

            default:
                break;
        }
    }

    void onActionSecondary() {
        switch (gCurrentTool) {
            case ToolType::Hammer:
                // Erase terrain (set to Air)
                Terrain::setVoxel(gTerrainCursor.x, gTerrainCursor.y, gTerrainCursor.z,
                    Terrain::TerrainMaterial::Air);
                AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/hammer.wav",
                    gTerrainCursor.x, gTerrainCursor.y, gTerrainCursor.z);
                break;

            case ToolType::Delete:
                deleteSelectedPart();
                break;

            default:
                clearSelection();
                break;
        }
    }

    // ---- Input handling ----
    void processInput() {
        hidScanInput();
        u32 kDown = hidKeysDown();

        // Tool switching with d-pad
        if (kDown & KEY_DUP) {
            int next = (int)gCurrentTool + 1;
            if (next >= (int)ToolType::Part) next = (int)ToolType::None;
            setTool((ToolType)next);
        }
        if (kDown & KEY_DDOWN) {
            int prev = (int)gCurrentTool - 1;
            if (prev < (int)ToolType::None) prev = (int)ToolType::Part;
            setTool((ToolType)prev);
        }

        // Brush size with L/R
        if (kDown & KEY_L) {
            int size = (int)gBrushSize - 1;
            if (size < (int)BrushSize::Small) size = (int)BrushSize::XLarge;
            gBrushSize = (BrushSize)size;
        }
        if (kDown & KEY_R) {
            int size = (int)gBrushSize + 1;
            if (size > (int)BrushSize::XLarge) size = (int)BrushSize::Small;
            gBrushSize = (BrushSize)size;
        }

        // Move tool with d-pad when moving
        if (gCurrentTool == ToolType::Move && gSelection.selectedPart) {
            float speed = 0.5f;
            if (kDown & KEY_DRIGHT) moveSelectedPart(speed, 0, 0);
            if (kDown & KEY_DLEFT)  moveSelectedPart(-speed, 0, 0);
            if (kDown & KEY_DUP)    moveSelectedPart(0, 0, speed);
            if (kDown & KEY_DDOWN)  moveSelectedPart(0, 0, -speed);
        }

        // Rotate tool
        if (gCurrentTool == ToolType::Rotate && gSelection.selectedPart) {
            float angle = 15.0f;
            if (kDown & KEY_DRIGHT) rotateSelectedPart(2, angle);    // Y-axis
            if (kDown & KEY_DLEFT)  rotateSelectedPart(2, -angle);
        }

        // Scale tool
        if (gCurrentTool == ToolType::Scale && gSelection.selectedPart) {
            float factor = 0.1f;
            if (kDown & KEY_DRIGHT) scaleSelectedPart(1 + factor, 1 + factor, 1 + factor);
            if (kDown & KEY_DLEFT)  scaleSelectedPart(1 - factor, 1 - factor, 1 - factor);
        }

        // Clear selection with B
        if (kDown & KEY_B) {
            if (gSelection.active) {
                clearSelection();
            } else {
                onActionSecondary();
            }
        }
    }

    void processTouch(void* pos_void) {
        // Touch-based tool interaction for bottom screen
        touchPosition* pos = (touchPosition*)pos_void;
        if (gSelection.dragging) {
            gSelection.boxW = std::abs((float)pos->px - gSelection.boxX);
            gSelection.boxH = std::abs((float)pos->py - gSelection.boxY);
        }
    }

    // ---- Terrain cursor ----
    void updateTerrainCursor(float camX, float camY, float camZ) {
        // Simple raycast from camera center to find terrain intersection
        // This is a simplified version - actual implementation would use proper raycasting
        float rayX = camX, rayY = camY, rayZ = camZ;
        // Cast ray downward to find terrain surface
        for (int i = 0; i < 100; i++) {
            rayY -= 0.5f;
            if (Terrain::isVoxelSolid(rayX, rayY, rayZ)) {
                gTerrainCursor.x = rayX;
                gTerrainCursor.y = rayY;
                gTerrainCursor.z = rayZ;
                gTerrainCursor.onTerrain = true;
                gTerrainCursor.material = (int)Terrain::getVoxel(rayX, rayY, rayZ).material;
                return;
            }
        }
        gTerrainCursor.onTerrain = false;
    }

    TerrainCursor getTerrainCursor() { return gTerrainCursor; }

    // ---- Brush settings ----
    void setBrushMaterial(int material) {
        gBrushMaterial = (Terrain::TerrainMaterial)material;
    }

    int getBrushMaterial() {
        return (int)gBrushMaterial;
    }

    void setBrushSize(BrushSize size) {
        gBrushSize = size;
    }

    BrushSize getBrushSize() {
        return gBrushSize;
    }

    // ---- Part manipulation ----
    void createPart(float x, float y, float z, float w, float h, float d, u32 color) {
        // Create a new part node and add to workspace
        // This is a placeholder - actual implementation depends on the Node system
        printf("Created part at (%.1f, %.1f, %.1f) size (%.1f, %.1f, %.1f)\n",
            x, y, z, w, h, d);
    }

    void cloneSelectedPart() {
        if (!gSelection.selectedPart) return;
        // Clone the selected part with a slight offset
        printf("Cloning selected part\n");
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/clone.wav", 0, 0, 0);
    }

    void deleteSelectedPart() {
        if (!gSelection.selectedPart) return;
        printf("Deleting selected part\n");
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/delete.wav", 0, 0, 0);
        clearSelection();
    }

    void moveSelectedPart(float dx, float dy, float dz) {
        if (!gSelection.selectedPart) return;
        printf("Moving part by (%.2f, %.2f, %.2f)\n", dx, dy, dz);
    }

    void scaleSelectedPart(float sx, float sy, float sz) {
        if (!gSelection.selectedPart) return;
        printf("Scaling part by (%.2f, %.2f, %.2f)\n", sx, sy, sz);
    }

    void rotateSelectedPart(float axis, float angle) {
        if (!gSelection.selectedPart) return;
        printf("Rotating part on axis %.0f by %.1f degrees\n", axis, angle);
    }

    // ---- Rendering ----
    void renderToolbar(float x, float y, float width, float height) {
        if (!gToolToolbarVisible) return;

        int toolCount = (int)ToolType::Part;
        float slotW = width / toolCount;
        float slotH = height;

        for (int i = 0; i <= (int)ToolType::Part; i++) {
            ToolType tool = (ToolType)i;
            float sx = x + i * slotW;
            float* color = TOOL_COLORS[i];
            int r = (int)(color[0] * 255);
            int g = (int)(color[1] * 255);
            int b = (int)(color[2] * 255);

            // Background
            if (tool == gCurrentTool) {
                RenderPipeline::drawRect(sx, y, slotW, slotH,
                    color[0] * 1.5f, color[1] * 1.5f, color[2] * 1.5f);
            } else {
                RenderPipeline::drawRect(sx, y, slotW, slotH, 0.2f, 0.2f, 0.2f);
            }

            // Tool name (using FontManager)
            FontManager::drawText(TOOL_NAMES[i], (int)sx + 2, (int)(y + slotH * 0.3f),
                255, 255, 255, FontSize::Size12);

            // Border
            RenderPipeline::drawRect(sx, y, slotW, 1, 0.5f, 0.5f, 0.5f);
            RenderPipeline::drawRect(sx, y + slotH - 1, slotW, 1, 0.5f, 0.5f, 0.5f);
            RenderPipeline::drawRect(sx, y, 1, slotH, 0.5f, 0.5f, 0.5f);
            RenderPipeline::drawRect(sx + slotW - 1, y, 1, slotH, 0.5f, 0.5f, 0.5f);
        }

        // Brush size indicator
        const char* sizes[] = {"S", "M", "L", "XL"};
        FontManager::drawText(sizes[(int)gBrushSize], (int)(x + width + 4), (int)y + 2,
            200, 200, 200, FontSize::Size12);
    }

    void renderCursorOverlay() {
        if (gCurrentTool == ToolType::Hammer || gCurrentTool == ToolType::Terrain) {
            // Render terrain brush cursor (wireframe box at cursor position)
            float size = (float)gBrushSize;
            // Simplified - actual implementation would use 3D rendering
        }
    }

    void renderSelectionBox() {
        if (gSelection.mode == SelectionMode::Box && gSelection.dragging) {
            RenderPipeline::drawRect(gSelection.boxX, gSelection.boxY,
                gSelection.boxW, gSelection.boxH,
                0.2f, 0.5f, 0.8f, 0.3f); // Semi-transparent blue
            // Border
            RenderPipeline::drawRect(gSelection.boxX, gSelection.boxY,
                gSelection.boxW, 1, 0.5f, 0.8f, 1.0f);
            RenderPipeline::drawRect(gSelection.boxX + gSelection.boxW, gSelection.boxY,
                1, gSelection.boxH, 0.5f, 0.8f, 1.0f);
            RenderPipeline::drawRect(gSelection.boxX, gSelection.boxY + gSelection.boxH,
                gSelection.boxW, 1, 0.5f, 0.8f, 1.0f);
            RenderPipeline::drawRect(gSelection.boxX, gSelection.boxY,
                1, gSelection.boxH, 0.5f, 0.8f, 1.0f);
        }
    }
}

// ==================== ToolSystem ====================
namespace ToolSystem {

    static std::vector<Tool> gToolSlots;
    static int gEquippedSlot = -1;

    void init() {
        gToolSlots.resize(10);
        for (auto &t : gToolSlots) {
            t.type = ToolType::None;
            t.equipped = false;
            t.cooldown = 1.0f;
            t.cooldownTimer = 0;
            t.damage = 0;
            t.range = 0;
            t.icon = nullptr;
        }
        gEquippedSlot = -1;
    }

    void shutdown() {
        gToolSlots.clear();
        gEquippedSlot = -1;
    }

    std::vector<Tool>& getToolSlots() { return gToolSlots; }

    void equipTool(int slot) {
        if (slot < 0 || slot >= (int)gToolSlots.size() || gToolSlots[slot].type == ToolType::None) return;
        for (auto &t : gToolSlots) t.equipped = false;
        gToolSlots[slot].equipped = true;
        gEquippedSlot = slot;
    }

    void unequipAll() {
        for (auto &t : gToolSlots) t.equipped = false;
        gEquippedSlot = -1;
    }

    int addTool(ToolType type, const std::string &name) {
        for (int i = 0; i < 10; i++) {
            if (gToolSlots[i].type == ToolType::None) {
                gToolSlots[i].type = type;
                gToolSlots[i].name = name;
                gToolSlots[i].equipped = false;
                gToolSlots[i].icon = loadToolIcon(type);

                // Set default stats
                switch (type) {
                    case ToolType::Sword:
                        gToolSlots[i].damage = 5; gToolSlots[i].range = 3; gToolSlots[i].cooldown = 0.5f; break;
                    case ToolType::Bomb:
                        gToolSlots[i].damage = 20; gToolSlots[i].range = 15; gToolSlots[i].cooldown = 2.0f; break;
                    case ToolType::Rocket:
                        gToolSlots[i].damage = 25; gToolSlots[i].range = 30; gToolSlots[i].cooldown = 1.5f; break;
                    case ToolType::PaintballGun:
                        gToolSlots[i].damage = 2; gToolSlots[i].range = 25; gToolSlots[i].cooldown = 0.3f; break;
                    case ToolType::Slingshot:
                        gToolSlots[i].damage = 3; gToolSlots[i].range = 20; gToolSlots[i].cooldown = 0.8f; break;
                    case ToolType::Laser:
                        gToolSlots[i].damage = 10; gToolSlots[i].range = 40; gToolSlots[i].cooldown = 0.2f; break;
                    default:
                        gToolSlots[i].damage = 1; gToolSlots[i].range = 5; gToolSlots[i].cooldown = 1.0f;
                }
                return i;
            }
        }
        return -1; // No free slot
    }

    void removeTool(int slot) {
        if (slot < 0 || slot >= (int)gToolSlots.size()) return;
        if (gToolSlots[slot].equipped) {
            gEquippedSlot = -1;
        }
        gToolSlots[slot].type = ToolType::None;
        gToolSlots[slot].name.clear();
        gToolSlots[slot].icon = nullptr;
        gToolSlots[slot].equipped = false;
    }

    void useEquippedTool(float px, float py, float pz) {
        if (gEquippedSlot < 0 || gEquippedSlot >= (int)gToolSlots.size()) return;
        Tool &t = gToolSlots[gEquippedSlot];
        if (t.cooldownTimer > 0) return;
        t.cooldownTimer = t.cooldown;

        // Play swing/use sound
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/collide.wav", px, py, pz);

        // Tool-specific effects
        switch (t.type) {
            case ToolType::Bomb:
                // Spawn explosion effect
                break;
            case ToolType::Rocket:
                // Fire rocket projectile
                AudioManager::playSoundFile("sdmc:/OpenBLOX/content/sounds/ufofly.wav");
                break;
            default:
                break;
        }
    }

    void updateTools(float dt) {
        for (int i = 0; i < (int)gToolSlots.size(); i++) {
            Tool &t = gToolSlots[i];
            if (t.cooldownTimer > 0) {
                t.cooldownTimer -= dt;
                if (t.cooldownTimer < 0) t.cooldownTimer = 0;
            }
        }
    }

    int getEquippedSlot() { return gEquippedSlot; }

    void renderHotbar(float x, float y, float width, float height) {
        int slots = 10;
        float slotW = width / slots;
        float slotH = height;
        for (int i = 0; i < slots; i++) {
            float sx = x + i * slotW;
            // Slot background
            if (gToolSlots[i].type != ToolType::None) {
                if (gToolSlots[i].equipped) {
                    RenderPipeline::drawRect(sx, y, slotW, slotH, 0.3f, 0.6f, 1.0f);
                } else {
                    RenderPipeline::drawRect(sx, y, slotW, slotH, 0.2f, 0.2f, 0.2f);
                }
                // Draw icon if available
                if (gToolSlots[i].icon) {
                    float iconSize = std::min(slotW, slotH) * 0.7f;
                    float cx = sx + (slotW - iconSize) / 2;
                    float cy = y + (slotH - iconSize) / 2;
                    RenderPipeline::drawTexturedRect(cx, cy, iconSize, iconSize, gToolSlots[i].icon, 1, 1, 1);
                }
                // Cooldown overlay
                if (gToolSlots[i].cooldownTimer > 0) {
                    float cdRatio = gToolSlots[i].cooldownTimer / gToolSlots[i].cooldown;
                    float cdH = slotH * cdRatio;
                    RenderPipeline::drawRect(sx, y + slotH - cdH, slotW, cdH, 0, 0, 0, 0.5f);
                }
            } else {
                RenderPipeline::drawRect(sx, y, slotW, slotH, 0.15f, 0.15f, 0.15f);
            }
            // Slot border
            RenderPipeline::drawRect(sx, y, slotW, 1, 0.5f, 0.5f, 0.5f);
            RenderPipeline::drawRect(sx, y, 1, slotH, 0.5f, 0.5f, 0.5f);
        }
    }

    void *loadToolIcon(ToolType type) {
        // Stub: icon loading not implemented yet
        // ContentSystem::findAsset and TextureLoader::loadTexture don't exist yet
        (void)type;
        return nullptr;
    }
}

// ==================== HealthSystem ====================
namespace HealthSystem {

    static PlayerHealth gHealth;
    static float gRespawnX = 0, gRespawnY = 10, gRespawnZ = 0;

    void init() {
        gHealth.current = 100;
        gHealth.maximum = 100;
        gHealth.walkSpeed = 16;
        gHealth.jumpPower = 30;
        gHealth.dead = false;
        gHealth.respawnTimer = 0;
        gHealth.damageFlashTimer = 0;
    }

    void setMaxHealth(float max) {
        gHealth.maximum = max;
        if (gHealth.current > max) gHealth.current = max;
    }

    void takeDamage(float amount, float x, float y, float z) {
        if (gHealth.dead) return;
        gHealth.current -= amount;
        gHealth.damageFlashTimer = 0.3f;
        if (gHealth.current <= 0) {
            gHealth.current = 0;
            gHealth.dead = true;
            gHealth.respawnTimer = 3.0f;
            gRespawnX = x; gRespawnY = y; gRespawnZ = z;
            // Death sound
            AudioManager::playSoundFile("sdmc:/OpenBLOX/content/sounds/Kid saying Ouch.wav");
        }
    }

    void heal(float amount) {
        gHealth.current += amount;
        if (gHealth.current > gHealth.maximum) gHealth.current = gHealth.maximum;
    }

    float getHealth() { return gHealth.current; }
    float getMaxHealth() { return gHealth.maximum; }
    bool isDead() { return gHealth.dead; }

    void update(float dt) {
        if (gHealth.damageFlashTimer > 0) {
            gHealth.damageFlashTimer -= dt;
        }
        if (gHealth.dead) {
            gHealth.respawnTimer -= dt;
            if (gHealth.respawnTimer <= 0) {
                respawn(gRespawnX, gRespawnY, gRespawnZ);
            }
        }
    }

    void respawn(float x, float y, float z) {
        gHealth.dead = false;
        gHealth.current = gHealth.maximum;
        gHealth.respawnTimer = 0;
        gRespawnX = x; gRespawnY = y; gRespawnZ = z;
    }

    void renderHealthBar(float x, float y, float width, float height) {
        // Background
        RenderPipeline::drawRect(x, y, width, height, 0.2f, 0.2f, 0.2f);
        // Health fill
        float ratio = gHealth.current / gHealth.maximum;
        float r, g, b;
        if (ratio > 0.5f) { r = 0; g = 1; b = 0; }
        else if (ratio > 0.25f) { r = 1; g = 0.5f; b = 0; }
        else { r = 1; g = 0; b = 0; }
        if (!gHealth.dead && ratio > 0) {
            RenderPipeline::drawRect(x, y, width * ratio, height, r, g, b);
        }
        // Border
        RenderPipeline::drawRect(x, y, width, 1, 0.5f, 0.5f, 0.5f);
        RenderPipeline::drawRect(x, y + height - 1, width, 1, 0.5f, 0.5f, 0.5f);
        RenderPipeline::drawRect(x, y, 1, height, 0.5f, 0.5f, 0.5f);
        RenderPipeline::drawRect(x + width - 1, y, 1, height, 0.5f, 0.5f, 0.5f);
    }
}

// ==================== AnimationSystem ====================
namespace AnimationSystem {

    static Animation gAnimations[(int)AnimationType::Count];
    static AnimationType gCurrentAnimation = AnimationType::Idle;
    static float gWalkSpeed = 16.0f;
    static float gJumpPower = 30.0f;
    static float gBoneTransforms[16][16]; // 16 bones, 4x4 transforms

    void init() {
        // Initialize animation presets
        gAnimations[(int)AnimationType::Idle].type = AnimationType::Idle;
        gAnimations[(int)AnimationType::Idle].duration = 1.0f;
        gAnimations[(int)AnimationType::Idle].looping = true;
        gAnimations[(int)AnimationType::Idle].frameCount = 1;
        gAnimations[(int)AnimationType::Idle].frameRate = 1;

        gAnimations[(int)AnimationType::Walk].type = AnimationType::Walk;
        gAnimations[(int)AnimationType::Walk].duration = 0.8f;
        gAnimations[(int)AnimationType::Walk].looping = true;
        gAnimations[(int)AnimationType::Walk].frameCount = 8;
        gAnimations[(int)AnimationType::Walk].frameRate = 10;

        gAnimations[(int)AnimationType::Run].type = AnimationType::Run;
        gAnimations[(int)AnimationType::Run].duration = 0.6f;
        gAnimations[(int)AnimationType::Run].looping = true;
        gAnimations[(int)AnimationType::Run].frameCount = 8;
        gAnimations[(int)AnimationType::Run].frameRate = 15;

        gAnimations[(int)AnimationType::Jump].type = AnimationType::Jump;
        gAnimations[(int)AnimationType::Jump].duration = 0.5f;
        gAnimations[(int)AnimationType::Jump].looping = false;
        gAnimations[(int)AnimationType::Jump].frameCount = 4;
        gAnimations[(int)AnimationType::Jump].frameRate = 8;

        gAnimations[(int)AnimationType::Death].type = AnimationType::Death;
        gAnimations[(int)AnimationType::Death].duration = 1.0f;
        gAnimations[(int)AnimationType::Death].looping = false;
        gAnimations[(int)AnimationType::Death].frameCount = 12;
        gAnimations[(int)AnimationType::Death].frameRate = 10;

        for (int i = 0; i < (int)AnimationType::Count; i++) {
            gAnimations[i].currentTime = 0;
            gAnimations[i].currentFrame = 0;
            gAnimations[i].meshData = nullptr;
        }

        // Identity bone transforms
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                gBoneTransforms[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }

    void setAnimation(AnimationType type) {
        gCurrentAnimation = type;
        gAnimations[(int)type].currentTime = 0;
        gAnimations[(int)type].currentFrame = 0;
    }

    AnimationType getCurrentAnimation() { return gCurrentAnimation; }

    void updateAnimation(float dt) {
        Animation &anim = gAnimations[(int)gCurrentAnimation];
        anim.currentTime += dt;
        if (anim.currentTime >= anim.duration) {
            if (anim.looping) {
                anim.currentTime = fmodf(anim.currentTime, anim.duration);
            } else {
                anim.currentTime = anim.duration;
            }
        }
        anim.currentFrame = (int)((anim.currentTime / anim.duration) * anim.frameCount);
        if (anim.currentFrame >= anim.frameCount) anim.currentFrame = anim.frameCount - 1;
    }

    void getBoneTransforms(float *transforms, int count) {
        // Returns bone transform data for rendering character meshes
        // Each bone is a 16-element 4x4 matrix (row-major)
        (void)count;
        memcpy(transforms, gBoneTransforms, sizeof(gBoneTransforms));
    }

    void setWalkSpeed(float speed) { gWalkSpeed = speed; }
    float getWalkSpeed() { return gWalkSpeed; }
    void setJumpPower(float power) { gJumpPower = power; }
    float getJumpPower() { return gJumpPower; }
}

// ============================================================================
// Classic Studio Tools (Hammer, Clone, Dropper, Selection, Move, Scale, etc.)
// ============================================================================
namespace ClassicStudioTools {

    // ---- Internal state ----
    static PartSelection gSelection;
    static ToolSettings gSettings;
    static float gGhostPreviewX = 0, gGhostPreviewY = 0, gGhostPreviewZ = 0;
    static bool gShowGhost = false;

    // Forward declarations for functions defined later
    void cloneSelectedPart();
    void deleteSelectedPart();
    void createPart(float x, float y, float z, float w, float h, float d, u32 color);

    void init() {
        gSelection.part = nullptr;
        gSelection.x = gSelection.y = gSelection.z = 0;
        gSelection.w = gSelection.h = gSelection.d = 4;
        gSelection.color = 0xFFFF0000;
        gSelection.originalX = gSelection.originalY = gSelection.originalZ = 0;
        gSelection.originalW = gSelection.originalH = gSelection.originalD = 4;
        gSelection.isSelected = false;

        gSettings.partSize = 4.0f;
        gSettings.partColor = 0xFFFF0000; // Bright red
        gSettings.activeTool = ClassicToolType::None;
        gSettings.isEditing = false;
    }

    void setActiveTool(ClassicToolType tool) {
        gSettings.activeTool = tool;
        if (tool != ClassicToolType::Selection) clearSelection();
        gSettings.isEditing = (tool != ClassicToolType::None);
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/click.wav", 0, 0, 0);
    }

    ClassicToolType getActiveTool() { return gSettings.activeTool; }

    PartSelection& getSelection() { return gSelection; }

    void clearSelection() {
        gSelection.part = nullptr;
        gSelection.isSelected = false;
        gShowGhost = false;
    }

    void selectPart(void* part, float x, float y, float z, float w, float h, float d, u32 color) {
        gSelection.part = part;
        gSelection.x = x; gSelection.y = y; gSelection.z = z;
        gSelection.w = w; gSelection.h = h; gSelection.d = d;
        gSelection.color = color;
        gSelection.originalX = x; gSelection.originalY = y; gSelection.originalZ = z;
        gSelection.originalW = w; gSelection.originalH = h; gSelection.originalD = d;
        gSelection.isSelected = true;
        gShowGhost = true;
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/select.wav", x, y, z);
    }

    // ---- Tool actions ----
    void useTool(float hitX, float hitY, float hitZ, float hitNX, float hitNY, float hitNZ) {
        switch (gSettings.activeTool) {
            case ClassicToolType::Selection:
                // Select part at hit position (caller provides part pointer)
                gShowGhost = true;
                gGhostPreviewX = hitX; gGhostPreviewY = hitY; gGhostPreviewZ = hitZ;
                break;

            case ClassicToolType::Hammer:
                // Place terrain at hit position
                Terrain::BrushSettings brush;
                brush.type = Terrain::BrushType::Add;
                brush.radius = 4.0f;
                brush.strength = 1.0f;
                brush.material = Terrain::TerrainMaterial::Grass;
                brush.smooth = false;
                Terrain::applyBrush(hitX, hitY, hitZ, brush);
                AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/hammer.wav", hitX, hitY, hitZ);
                break;

            case ClassicToolType::Clone:
                if (gSelection.isSelected) {
                    cloneSelectedPart();
                }
                break;

            case ClassicToolType::Delete:
                if (gSelection.isSelected) {
                    deleteSelectedPart();
                }
                break;

            case ClassicToolType::Dropper: {
                // Sample terrain material at hit position
                Terrain::Voxel voxel = Terrain::getVoxel(hitX, hitY, hitZ);
                if (voxel.isSolid) {
                    gSettings.partColor = voxel.color;
                    AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/dropper.wav", hitX, hitY, hitZ);
                }
                break;
            }

            case ClassicToolType::PartBuilder:
                // Place a new part at hit position
                createPart(hitX, hitY, hitZ,
                    gSettings.partSize, gSettings.partSize, gSettings.partSize,
                    gSettings.partColor);
                break;

            case ClassicToolType::Move:
                // Start moving selected part
                if (gSelection.isSelected) {
                    gSelection.originalX = gSelection.x;
                    gSelection.originalY = gSelection.y;
                    gSelection.originalZ = gSelection.z;
                }
                break;

            case ClassicToolType::Scale:
                if (gSelection.isSelected) {
                    gSelection.originalW = gSelection.w;
                    gSelection.originalH = gSelection.h;
                    gSelection.originalD = gSelection.d;
                }
                break;

            case ClassicToolType::Rotate:
                if (gSelection.isSelected) {
                    // Start rotation (would track angle from right stick)
                    printf("[Studio] Rotation tool: selected part at (%.1f, %.1f, %.1f)\n",
                        gSelection.x, gSelection.y, gSelection.z);
                }
                break;

            case ClassicToolType::Anchor:
                if (gSelection.isSelected && gSelection.part) {
                    Node* part = (Node*)gSelection.part;
                    part->anchored = !part->anchored;
                    printf("[Studio] Anchored: %s\n", part->anchored ? "true" : "false");
                    AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/click.wav", 0, 0, 0);
                }
                break;

            case ClassicToolType::NoTouch:
                if (gSelection.isSelected && gSelection.part) {
                    Node* part = (Node*)gSelection.part;
                    part->canCollide = !part->canCollide;
                    printf("[Studio] CanCollide: %s\n", part->canCollide ? "true" : "false");
                    AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/click.wav", 0, 0, 0);
                }
                break;

            case ClassicToolType::Transparency:
                if (gSelection.isSelected && gSelection.part) {
                    Part* part = (Part*)gSelection.part;
                    part->transparency += 0.1f;
                    if (part->transparency > 1.0f) part->transparency = 0.0f;
                    printf("[Studio] Transparency: %.1f\n", part->transparency);
                    AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/click.wav", 0, 0, 0);
                }
                break;

            case ClassicToolType::Welder:
                if (gSelection.isSelected) {
                    printf("[Studio] Welder tool: would weld selected part to neighbor\n");
                    AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/weld.wav", 0, 0, 0);
                }
                break;

            case ClassicToolType::NoCollision:
                if (gSelection.isSelected) {
                    printf("[Studio] NoCollision: would disable collision for selected part\n");
                    AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/click.wav", 0, 0, 0);
                }
                break;

            case ClassicToolType::Color:
                if (gSelection.isSelected && gSelection.part) {
                    Part* part = (Part*)gSelection.part;
                    part->color = (gSettings.partColor >> 16) & 0xFF;
                    printf("[Studio] Color set: %d\n", part->color);
                    AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/paint.wav", 0, 0, 0);
                }
                break;

            default:
                break;
        }
    }

    void updateTool(float dt, float camX, float camY, float camZ) {
        // Continuous tool actions (e.g., move while holding button)
        (void)dt;
        (void)camX;
        (void)camY;
        (void)camZ;
    }

    // ---- Part manipulation ----
    void createPart(float x, float y, float z, float w, float h, float d, u32 color) {
        printf("[Studio] Created part at (%.1f, %.1f, %.1f) size (%.1f, %.1f, %.1f)\n",
            x, y, z, w, h, d);
    }

    void cloneSelectedPart() {
        if (!gSelection.isSelected) return;
        printf("[Studio] Cloning part at (%.1f, %.1f, %.1f)\n",
            gSelection.x, gSelection.y, gSelection.z);
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/clone.wav", 0, 0, 0);
    }

    void deleteSelectedPart() {
        if (!gSelection.isSelected) return;
        printf("[Studio] Deleting part at (%.1f, %.1f, %.1f)\n",
            gSelection.x, gSelection.y, gSelection.z);
        AudioManager::playSound3D("sdmc:/OpenBLOX/content/sounds/delete.wav", 0, 0, 0);
        clearSelection();
    }

    // ---- Rendering ----
    void renderToolBar(float x, float y, float width, float height) {
        // Tool names and colors for each tool
        static const char* TOOL_NAMES[] = {
            "", "Select", "Move", "Scale", "Rot", "Del",
            "Hammer", "Clone", "Drop", "Part", "Mesh",
            "Anchor", "NoTouch", "Trans", "Weld", "NoCol", "Paint"
        };
        static const int TOOL_COLORS_R[] = {
            40, 70, 180, 120, 140, 200,
            180, 80, 200, 100, 100,
            200, 100, 150, 160, 80, 180
        };
        static const int TOOL_COLORS_G[] = {
            40, 150, 80, 70, 80, 50,
            180, 200, 80, 180, 100,
            100, 80, 50, 120, 160, 80
        };
        static const int TOOL_COLORS_B[] = {
            40, 220, 60, 120, 200, 50,
            40, 120, 200, 220, 100,
            50, 160, 220, 80, 100, 120
        };

        int toolCount = (int)ClassicToolType::Count - 1; // Exclude None
        float slotW = width / toolCount;
        float slotH = height;

        for (int i = 1; i <= toolCount; i++) {
            ClassicToolType tool = (ClassicToolType)i;
            float sx = x + (i - 1) * slotW;

            bool isActive = (tool == gSettings.activeTool);

            // Background
            if (isActive) {
                RenderPipeline::drawRect(sx + 1, y + 1, slotW - 2, slotH - 2,
                    (float)TOOL_COLORS_R[i] / 255.0f,
                    (float)TOOL_COLORS_G[i] / 255.0f,
                    (float)TOOL_COLORS_B[i] / 255.0f);
            } else {
                RenderPipeline::drawRect(sx + 1, y + 1, slotW - 2, slotH - 2,
                    0.15f, 0.15f, 0.15f);
            }

            // Tool name
            FontManager::drawText(TOOL_NAMES[i],
                (int)sx + 1, (int)(y + slotH * 0.25f),
                isActive ? 255 : 150, isActive ? 255 : 150, isActive ? 255 : 150,
                FontSize::Size12);

            // Border
            RenderPipeline::drawRect(sx, y, slotW, 1, 0.4f, 0.4f, 0.4f);
            RenderPipeline::drawRect(sx, y + slotH - 1, slotW, 1, 0.4f, 0.4f, 0.4f);
            RenderPipeline::drawRect(sx, y, 1, slotH, 0.4f, 0.4f, 0.4f);
            RenderPipeline::drawRect(sx + slotW - 1, y, 1, slotH, 0.4f, 0.4f, 0.4f);
        }

        // Part size indicator
        char sizeText[32];
        snprintf(sizeText, sizeof(sizeText), "Sz:%.0f", gSettings.partSize);
        FontManager::drawText(sizeText, (int)(x + width + 4), (int)y + 2,
            180, 180, 180, FontSize::Size12);
    }

    void renderSelectionBox() {
        if (!gSelection.isSelected) return;

        // 3D selection box around selected part
        float x = gSelection.x - gSelection.w / 2;
        float y = gSelection.y - gSelection.h / 2;
        float z = gSelection.z - gSelection.d / 2;

        // Draw wireframe box edges
        RenderPipeline::setBlend(true);
        // Bottom face wireframe
        RenderPipeline::drawBox(x, y, z, gSelection.w, 0.05f, gSelection.d, 1.0f, 0.4f, 0.0f, 0.6f);
        // Top face wireframe
        RenderPipeline::drawBox(x, y + gSelection.h, z, gSelection.w, 0.05f, gSelection.d, 1.0f, 0.4f, 0.0f, 0.6f);
        RenderPipeline::setBlend(false);
    }

    void renderGhostPreview() {
        if (!gShowGhost) return;

        // Semi-transparent preview of what tool will do
        RenderPipeline::setBlend(true);
        RenderPipeline::drawBox(gGhostPreviewX, gGhostPreviewY, gGhostPreviewZ,
            gSettings.partSize, gSettings.partSize, gSettings.partSize,
            (gSettings.partColor >> 16) / 255.0f,
            (gSettings.partColor >> 8) / 255.0f,
            (gSettings.partColor & 0xFF) / 255.0f,
            0.3f); // 30% opacity
        RenderPipeline::setBlend(false);
    }

    ToolSettings& getSettings() { return gSettings; }

    void shutdown() {
        clearSelection();
        gSettings.activeTool = ClassicToolType::None;
    }
}