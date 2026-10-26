#include "player_controller.h"
#include "../../ThirdParty/TinyPhysicsEngine/tinyphysicsengine.h"
#include "../../importantIncludes.h"
#include <3ds.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using namespace std;

// Static buffers for player body (box has 8 joints, 16 connections)
static TPE_Joint playerJoints[8];
static TPE_Connection playerConnections[16];
static TPE_Body playerBodyStruct;
static TPE_Body* playerBody = nullptr;

// TPE uses fixed-point units. TPE_F (512) = 1 unit.
#define TPE_TO_FIXED(v) ((TPE_Unit)((v) * TPE_F))
#define TPE_FROM_FIXED(v) ((float)(v) / TPE_F)

// --- Classic Roblox physics constants (2007-2011 era) ---
// Gravity: -196.2 studs/sec² (classic Roblox default)
// WalkSpeed: 16 studs/sec (classic Roblox default)
// JumpPower: 40 studs/sec² (classic Roblox default)
// TPE scales: TPE_F=512 units per stud, physics runs at lower scale

// Per-material friction values (matching old Roblox 2007-2011)
// These match the MaterialProperties.friction values for consistency
static const float MATERIAL_FRICTION[] = {
    0.7f,  // SmoothPlastic
    0.8f,  // Wood
    0.4f,  // Metal
    0.05f, // Ice
    0.5f,  // Neon
    0.9f,  // Grass
    0.7f,  // Concrete
    0.8f,  // Fabric
    0.9f,  // Stone
    0.1f,  // Slime
};

// Per-material elasticity (bounciness)
// These match the MaterialProperties.elasticity values for consistency
static const float MATERIAL_ELASTICITY[] = {
    0.1f,   // SmoothPlastic
    0.05f,  // Wood
    0.3f,   // Metal
    0.3f,   // Ice
    0.2f,   // Neon
    0.0f,   // Grass
    0.0f,   // Concrete
    0.1f,   // Fabric
    0.0f,   // Stone
    0.5f,   // Slime
};

// Classic Roblox constants (TPE-scaled)
static const float CLASSIC_GRAVITY       = 196.2f;    // studs/sec² (classic Roblox)
static const float CLASSIC_WALK_SPEED    = 16.0f;     // studs/sec (classic Roblox)
static const float CLASSIC_JUMP_POWER    = 40.0f;     // studs/sec² (classic Roblox)

// Velocity damping factors
static const float AIR_RESISTANCE        = 0.98f;     // velocity *= 0.98 each frame in air
static const float GROUND_FRICTION_BASE  = 0.85f;     // velocity *= 0.85 each frame on ground

// Velocity limiting (prevents tunneling)
static const float MAX_HORIZONTAL_SPEED  = 20.0f;     // Max horizontal speed (studs/sec)
static const float MAX_VERTICAL_SPEED    = 50.0f;     // Max vertical speed (studs/sec)

// Jump settings (floaty old Roblox feel)
static const float JUMP_VELOCITY         = 6.0f;      // TPE-scaled jump velocity
static const float JUMP_GRAVITY_SCALE    = 0.85f;     // Slightly reduced gravity during ascent for "floaty" feel
static const float GRAVITY_Y             = 19.62f;    // TPE-scaled gravity (classic 196.2 / 10)
static const float DOUBLE_TAP_WINDOW     = 0.3f;      // Seconds between A presses for double-tap jump
static const float DOUBLE_TAP_BOOST      = 1.4f;      // Multiplier for double-tap jump height

// Wall stick settings
static const float WALL_STICK_DURATION = 0.15f;  // How long wall stick lasts
static const float WALL_STICK_DIST     = 1.0f;   // Distance to wall to trigger stick

// Player state
static bool playerOnGround = false;
static float lastJumpTime = 0.0f;
static float wallStickTimer = 0.0f;
static bool wallStickActive = false;
static float currentMaterialFriction = 0.3f;
static float currentMaterialElasticity = 0.0f;

// Buoyancy settings
static const float WATER_DENSITY = 1.0f;
static const float WATER_LEVEL = 0.0f; // Water surface Y level
static const float BUOYANCY_FACTOR = 0.8f;
static const float WATER_DRAG = 0.9f;

void PlayerController::initPlayer(float x, float y, float z) {
    if (playerBody) return;

    // Load the character model from content/fonts/character.rbxm
    FILE* f = fopen("content/fonts/character.rbxm", "rb");
    if (f) {
        Node* characterModel = parseRBXMf(f, false);
        fclose(f);

        if (characterModel) {
            print_message("Loaded character.rbxm successfully.\n");
        } else {
            print_message("Failed to parse character.rbxm, using fallback box body.\n");
        }
    } else {
        print_message("Failed to load character.rbxm, using fallback box body.\n");
    }

    // Create box geometry: width=1.0, depth=0.5, height=2.0
    TPE_Unit width  = TPE_TO_FIXED(1.0f);
    TPE_Unit depth  = TPE_TO_FIXED(0.5f);
    TPE_Unit height = TPE_TO_FIXED(2.0f);
    TPE_Unit jointSize = TPE_TO_FIXED(0.1f);

    TPE_makeBox(playerJoints, playerConnections, width, depth, height, jointSize);

    // Initialize body: mass=5.0 fixed point
    TPE_bodyInit(&playerBodyStruct, playerJoints, 8, playerConnections, 16, TPE_TO_FIXED(5.0f));
    playerBodyStruct.friction   = 128;
    playerBodyStruct.elasticity = 0;
    playerBodyStruct.flags      = TPE_BODY_FLAG_NONROTATING | TPE_BODY_FLAG_ALWAYS_ACTIVE;

    // Move body to desired position
    TPE_Vec3 center = TPE_vec3(TPE_TO_FIXED(x), TPE_TO_FIXED(y), TPE_TO_FIXED(z));
    TPE_bodyMoveTo(&playerBodyStruct, center);

    playerBody = &playerBodyStruct;
    playerOnGround = false;
    lastJumpTime = -1.0f;
    wallStickTimer = 0.0f;
    wallStickActive = false;
    currentMaterialFriction = MATERIAL_FRICTION[0]; // Default SmoothPlastic
    currentMaterialElasticity = MATERIAL_ELASTICITY[0];

    // Add player body to the world
    if (world.bodyCount == 0) {
        world.bodies = playerBody;
        world.bodyCount = 1;
    } else {
        TPE_Body* newBodies = (TPE_Body*)malloc(sizeof(TPE_Body) * (world.bodyCount + 1));
        if (newBodies) {
            for (uint16_t i = 0; i < world.bodyCount; i++)
                newBodies[i] = world.bodies[i];
            newBodies[world.bodyCount] = *playerBody;
            world.bodies = newBodies;
            playerBody = &newBodies[world.bodyCount];
            world.bodyCount++;
        }
    }
}

void PlayerController::setMaterial(int materialIndex) {
    if (materialIndex >= 0 && materialIndex < 10) {
        currentMaterialFriction = MATERIAL_FRICTION[materialIndex];
        currentMaterialElasticity = MATERIAL_ELASTICITY[materialIndex];
    }
}

void PlayerController::updatePlayer(float dt) {
    if (!playerBody) return;

    float vx = horizontal_axis * 5.0f;
    float vz = vertical_axis * 5.0f;

    // --- Velocity damping (air resistance / ground friction) ---
    TPE_Joint* joints = playerBody->joints;
    bool wasOnGround = playerOnGround;

    // Check if player is near ground (any joint Y velocity is very low)
    float avgYVel = 0;
    for (uint16_t i = 0; i < playerBody->jointCount; i++) {
        avgYVel += TPE_FROM_FIXED(joints[i].velocity[1]);
    }
    avgYVel /= playerBody->jointCount;

    TPE_Vec3 pos = TPE_bodyGetCenterOfMass(playerBody);
    float playerY = TPE_FROM_FIXED(pos.y);

    // Ground detection: low Y position and low vertical velocity
    playerOnGround = (playerY <= 1.05f && fabsf(avgYVel) < 0.5f);

    // Apply damping based on ground state
    float dampingFactor = playerOnGround ? GROUND_FRICTION_BASE : AIR_RESISTANCE;

    // Apply damping to horizontal velocity only (preserve gravity effect on Y)
    for (uint16_t i = 0; i < playerBody->jointCount; i++) {
        // Horizontal damping
        joints[i].velocity[0] = (TPE_Unit)((TPE_FROM_FIXED(joints[i].velocity[0]) * dampingFactor) * TPE_F);
        joints[i].velocity[2] = (TPE_Unit)((TPE_FROM_FIXED(joints[i].velocity[2]) * dampingFactor) * TPE_F);

        // Apply material-based friction when on ground
        if (playerOnGround) {
            float frictionDamping = 1.0f - (currentMaterialFriction * 0.1f);
            joints[i].velocity[0] = (TPE_Unit)((TPE_FROM_FIXED(joints[i].velocity[0]) * frictionDamping) * TPE_F);
            joints[i].velocity[2] = (TPE_Unit)((TPE_FROM_FIXED(joints[i].velocity[2]) * frictionDamping) * TPE_F);
        }

        // Velocity limiting (prevents tunneling)
        float velX = TPE_FROM_FIXED(joints[i].velocity[0]);
        float velY = TPE_FROM_FIXED(joints[i].velocity[1]);
        float velZ = TPE_FROM_FIXED(joints[i].velocity[2]);

        float horizontalSpeed = sqrtf(velX * velX + velZ * velZ);
        if (horizontalSpeed > MAX_HORIZONTAL_SPEED) {
            float scale = MAX_HORIZONTAL_SPEED / horizontalSpeed;
            velX *= scale;
            velZ *= scale;
            joints[i].velocity[0] = TPE_TO_FIXED(velX);
            joints[i].velocity[2] = TPE_TO_FIXED(velZ);
        }

        if (velY > MAX_VERTICAL_SPEED) {
            joints[i].velocity[1] = TPE_TO_FIXED(MAX_VERTICAL_SPEED);
        } else if (velY < -MAX_VERTICAL_SPEED) {
            joints[i].velocity[1] = TPE_TO_FIXED(-MAX_VERTICAL_SPEED);
        }
    }

    // Apply velocity as acceleration
    TPE_Vec3 accel = TPE_vec3(TPE_TO_FIXED(vx), 0, TPE_TO_FIXED(vz));
    TPE_bodyAccelerate(playerBody, accel);

    // --- Jump handling with double-tap detection ---
    if (jump) {
        float currentTime = (float)osGetTime() / 1000000000.0f;
        bool isDoubleTap = (currentTime - lastJumpTime < DOUBLE_TAP_WINDOW);

        float jumpForce = JUMP_VELOCITY;
        if (isDoubleTap) {
            jumpForce *= DOUBLE_TAP_BOOST;
            wallStickActive = false;
        }

        // Wall stick behavior: if near a wall, redirect some velocity toward wall
        if (wallStickActive) {
            jumpForce *= 0.8f; // Slightly reduced jump when wall-sticking
            wallStickActive = false;
            wallStickTimer = 0.0f;
        }

        TPE_Vec3 jumpVel = TPE_vec3(0, TPE_TO_FIXED(jumpForce), 0);
        TPE_bodyAccelerate(playerBody, jumpVel);

        lastJumpTime = currentTime;
        playerOnGround = false;
        jump = 0;
    }

    // --- Wall stick timer decay ---
    if (wallStickActive) {
        wallStickTimer -= dt;
        if (wallStickTimer <= 0.0f) {
            wallStickActive = false;
        }
    }

    // --- Buoyancy check ---
    if (playerY < WATER_LEVEL) {
        float submerged = WATER_LEVEL - playerY;
        float buoyancyForce = submerged * BUOYANCY_FACTOR * WATER_DENSITY;
        TPE_Vec3 buoyancy = TPE_vec3(0, TPE_TO_FIXED(buoyancyForce), 0);
        TPE_bodyAccelerate(playerBody, buoyancy);

        // Extra drag when underwater
        for (uint16_t i = 0; i < playerBody->jointCount; i++) {
            joints[i].velocity[0] = (TPE_Unit)((TPE_FROM_FIXED(joints[i].velocity[0]) * WATER_DRAG) * TPE_F);
            joints[i].velocity[1] = (TPE_Unit)((TPE_FROM_FIXED(joints[i].velocity[1]) * WATER_DRAG) * TPE_F);
            joints[i].velocity[2] = (TPE_Unit)((TPE_FROM_FIXED(joints[i].velocity[2]) * WATER_DRAG) * TPE_F);
        }
    }

    // --- Wall stick detection (check proximity to vertical surfaces) ---
    // Check adjacent parts for wall proximity
    float playerX = TPE_FROM_FIXED(pos.x);
    float playerZ = TPE_FROM_FIXED(pos.z);
    bool nearWall = false;
    for (uint16_t i = 0; i < world.bodyCount; i++) {
        TPE_Vec3 partPos = TPE_bodyGetCenterOfMass(&world.bodies[i]);
        float dx = fabsf(playerX - TPE_FROM_FIXED(partPos.x));
        float dz = fabsf(playerZ - TPE_FROM_FIXED(partPos.z));
        if (dx < WALL_STICK_DIST || dz < WALL_STICK_DIST) {
            nearWall = true;
            break;
        }
    }
    if (nearWall && !playerOnGround) {
        wallStickActive = true;
        wallStickTimer = WALL_STICK_DURATION;
    }

    // Sync camera to player position
    camera_x = TPE_FROM_FIXED(pos.x);
    camera_y = TPE_FROM_FIXED(pos.y);
    camera_z = TPE_FROM_FIXED(pos.z);
}

void PlayerController::shutdownPlayer() {
    if (playerBody) {
        playerBody->flags |= TPE_BODY_FLAG_DISABLED;
        playerBody = nullptr;
    }
    playerOnGround = false;
    wallStickActive = false;
}