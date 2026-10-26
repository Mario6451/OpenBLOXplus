#pragma once

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <3ds/types.h>
#include "../CoreEngine/Node/nodes.h"

// ============================================================================
// Classic Roblox Studio Tools (Hammer, Clone, Dropper, Move, Scale, etc.)
// ============================================================================
namespace ClassicTools {

    enum class ToolType {
        None,
        Selection,    // Click to select parts
        Move,         // Reposition selected part
        Scale,        // Resize selected part
        Rotate,       // Rotate selected part
        Delete,       // Remove selected part
        Hammer,       // Edit terrain
        Clone,        // Duplicate selected part
        Dropper,      // Pick terrain material
        Mesh,         // Basic mesh manipulation
        Terrain,      // Terrain edit mode
        Part          // Place a new part
    };

    enum class SelectionMode {
        None,
        Single,       // Select one part at a time
        Box,          // Drag to select multiple parts
        Lasso         // Freeform selection (future)
    };

    // Brush size for terrain hammer
    enum class BrushSize {
        Small = 1,
        Medium = 3,
        Large = 5,
        XLarge = 8
    };

    struct Selection {
        bool active;
        void* selectedPart;       // Pointer to selected part node
        std::vector<void*> multiSelection;
        SelectionMode mode;
        // Box selection state
        float boxX, boxY, boxW, boxH;
        bool dragging;
    };

    // Terrain brush cursor
    struct TerrainCursor {
        float x, y, z;            // Hit position
        bool onTerrain;
        bool onPart;
        int material;             // Terrain material ID
    };

    // Initialize classic tools
    void init();

    // Set current tool
    void setTool(ToolType tool);
    ToolType getCurrentTool();

    // Selection
    Selection& getSelection();
    void selectPart(void* part);
    void clearSelection();
    void multiSelectBox(float x1, float y1, float x2, float y2);

    // Tool actions (called when A button pressed)
    void onAction();
    void onActionSecondary();    // B button (delete/erase)

    // Tool input handling
    void processInput();
    void processTouch(void* pos);

    // Set terrain brush material
    void setBrushMaterial(int material);
    int getBrushMaterial();

    // Set brush size
    void setBrushSize(BrushSize size);
    BrushSize getBrushSize();

    // Terrain cursor update
    void updateTerrainCursor(float camX, float camY, float camZ);
    TerrainCursor getTerrainCursor();

    // Render tool UI (toolbar + cursor)
    void renderToolbar(float x, float y, float width, float height);
    void renderCursorOverlay();
    void renderSelectionBox();

    // Part manipulation
    void createPart(float x, float y, float z, float w, float h, float d, u32 color);
    void cloneSelectedPart();
    void deleteSelectedPart();
    void moveSelectedPart(float dx, float dy, float dz);
    void scaleSelectedPart(float sx, float sy, float sz);
    void rotateSelectedPart(float axis, float angle);

    // Shutdown
    void shutdown();
}

namespace ToolSystem {

    // Tool types
    enum class ToolType {
        None,
        Sword,
        Bomb,
        Rocket,
        PaintballGun,
        Slingshot,
        Laser,
        Hammer,
        CloneWand,
        FireWand,
        Count
    };

    struct Tool {
        ToolType type;
        std::string name;
        void *icon;            // icon texture
        bool equipped;
        float cooldown;        // seconds between uses
        float cooldownTimer;
        float damage;
        float range;
    };

    // Initialize tool system
    void init();

    // Shutdown
    void shutdown();

    // Get player's tool slots (10 slots)
    std::vector<Tool>& getToolSlots();

    // Equip tool by slot index (0-9)
    void equipTool(int slot);

    // Unequip all tools
    void unequipAll();

    // Add tool to next available slot
    int addTool(ToolType type, const std::string &name);

    // Remove tool from slot
    void removeTool(int slot);

    // Use equipped tool (called when action button pressed)
    void useEquippedTool(float px, float py, float pz);

    // Update tool cooldowns
    void updateTools(float dt);

    // Get currently equipped slot (-1 if none)
    int getEquippedSlot();

    // Render hotbar on bottom screen
    void renderHotbar(float x, float y, float width, float height);

    // Load tool icon from content
    void *loadToolIcon(ToolType type);
}

namespace HealthSystem {

    struct PlayerHealth {
        float current;
        float maximum;
        float walkSpeed;
        float jumpPower;
        bool dead;
        float respawnTimer;
        float damageFlashTimer;
    };

    // Initialize health system
    void init();

    // Set max health
    void setMaxHealth(float max);

    // Take damage
    void takeDamage(float amount, float x, float y, float z);

    // Heal
    void heal(float amount);

    // Get health
    float getHealth();
    float getMaxHealth();

    // Is player dead?
    bool isDead();

    // Update (respawn, damage flash)
    void update(float dt);

    // Respawn at position
    void respawn(float x, float y, float z);

    // Render health bar on bottom screen
    void renderHealthBar(float x, float y, float width, float height);
}

namespace AnimationSystem {

    enum class AnimationType {
        Idle,
        Walk,
        Run,
        Jump,
        Fall,
        Death,
        Swim,
        Climbing,
        Count
    };

    struct Animation {
        AnimationType type;
        float duration;           // total animation duration
        float currentTime;       // current playback time
        bool looping;
        int frameCount;          // number of frames
        int currentFrame;        // current frame
        float frameRate;         // frames per second
        void *meshData;          // custom mesh data (optional)
    };

    // Initialize
    void init();

    // Set current animation
    void setAnimation(AnimationType type);

    // Get current animation
    AnimationType getCurrentAnimation();

    // Update animation (advance frames)
    void updateAnimation(float dt);

    // Get current bone transforms for rendering
    void getBoneTransforms(float *transforms, int count);

    // Walk speed
    void setWalkSpeed(float speed);
    float getWalkSpeed();

    // Jump power
    void setJumpPower(float power);
    float getJumpPower();
}

// ==================== Classic Studio Tools ====================
// Classic Roblox Studio/Classic Mode editing tools

namespace ClassicStudioTools {

    enum class ClassicToolType {
        None,
        Selection,    // Select and move parts
        Move,         // Drag to move parts
        Scale,        // Resize parts
        Rotate,       // Rotate parts
        Delete,       // Remove parts
        Hammer,       // Edit terrain
        Clone,        // Duplicate parts
        Dropper,      // Pick material/color
        PartBuilder,  // Place new parts
        Mesh,         // Advanced mesh tool
        Anchor,       // Toggle anchored state
        NoTouch,      // Toggle canCollide state
        Transparency, // Adjust part transparency
        Welder,       // Weld parts together
        NoCollision,  // Disable collision between parts
        Color,        // Paint part color
        Count
    };

    struct PartSelection {
        void* part;           // pointer to selected part/node
        float x, y, z;        // world position
        float w, h, d;        // dimensions
        u32 color;
        float originalX, originalY, originalZ; // for undo/move preview
        float originalW, originalH, originalD;
        bool isSelected;
    };

    struct ToolSettings {
        float partSize;       // default size for PartBuilder
        u32 partColor;        // default color
        ClassicToolType activeTool;
        bool isEditing;
    };

    // Initialize classic tools
    void init();

    // Set active tool
    void setActiveTool(ClassicToolType tool);
    ClassicToolType getActiveTool();

    // Tool actions (call when A button pressed)
    void useTool(float hitX, float hitY, float hitZ, float hitNX, float hitNY, float hitNZ);

    // Tool-specific continuous actions (hold B to drag/move)
    void updateTool(float dt, float camX, float camY, float camZ);

    // Get current selection
    PartSelection& getSelection();

    // Clear selection
    void clearSelection();

    // Select part at position (raycast result)
    void selectPart(void* part, float x, float y, float z, float w, float h, float d, u32 color);

    // Render tool UI on bottom screen
    void renderToolBar(float x, float y, float width, float height);

    // Render selection box on top screen (in 3D)
    void renderSelectionBox();

    // Render ghost preview (what tool will do)
    void renderGhostPreview();

    // Get tool settings
    ToolSettings& getSettings();

    // Shutdown
    void shutdown();
}