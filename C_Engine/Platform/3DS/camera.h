#pragma once

#include <3ds.h>
#include <vector>

// Camera system with trackpad control, multiple modes, water/terrain support

namespace CameraSystem {

    enum class CameraMode {
        ThirdPerson,
        FirstPerson,
        ChaseCamera
    };

    struct Camera {
        float x, y, z;              // position
        float pitch;                // vertical angle (degrees)
        float yaw;                  // horizontal angle (degrees)
        float distance;             // third-person distance
        float targetDistance;       // target distance for smooth zoom interpolation
        float zoomSpeed;            // zoom speed
        float zoomLerpSpeed;        // smooth zoom interpolation speed
        float rotationSpeed;        // camera rotation speed
        CameraMode mode;
        float fov;                  // field of view (degrees)
        float nearClip;             // near clipping plane
        float farClip;              // far clipping plane (render distance)
        float minDistance;          // minimum zoom distance
        float maxDistance;          // maximum zoom distance
        float firstPersonThreshold; // auto-switch to FP when zoom closer than this
        float headOffsetY;          // first-person camera height
        bool autoFirstPerson;       // enable auto-switching
    };

    struct TrackpadState {
        bool active;
        float startX, startY;       // initial touch position
        float lastX, lastY;         // last touch position
        float deltaX, deltaY;       // delta movement
    };

    struct WaterSurface {
        float y;                    // water height
        float waveHeight;           // wave amplitude
        float waveSpeed;            // wave frequency
        float r, g, b, a;          // water color
        float reflectivity;         // reflectivity (0-1)
    };

    struct TerrainVoxel {
        int x, y, z;                // voxel coordinates
        float data;                 // density/hardness
        int material;               // material type index
    };

    // Initialize camera
    void init();

    // Set camera position
    void setPosition(float x, float y, float z);

    // Get camera position
    void getPosition(float *x, float *y, float *z);

    // Get camera target (where it's looking)
    void getTarget(float *x, float *y, float *z);

    // Set camera mode
    void setMode(CameraMode mode);
    CameraMode getMode();

    // Process trackpad input (call every frame)
    void processTrackpad(touchPosition *pos);

    // Process C-Stick input (New 3DS)
    void processCStick(s16 cx, s16 cy);

    // Zoom in/out
    void zoom(float amount);
    void updateZoomSmoothing(float dt);
    float getDistance();
    void setDistance(float distance);
    float getTargetDistance();
    float getMinDistance();
    float getMaxDistance();

    // Update camera (track target position)
    void update(float dt, float targetX, float targetY, float targetZ);

    // Get view matrix data for render pipeline
    void applyCameraView(float ex, float ey, float ez, float cx, float cy, float cz);

    // Get camera FOV
    float getFOV();
    void setFOV(float fov);

    // Set render distance (far clip)
    void setFarClip(float far);

    // === Water ===
    // Add a water surface
    void addWaterSurface(float height);

    // Update water animation
    void updateWater(float dt);

    // Get water height at position (with waves)
    float getWaterHeight(float x, float z, float time);

    // Get water surface reference
    const WaterSurface* getWaterSurface(int index);
    int getWaterSurfaceCount();

    // === Terrain ===
    // Initialize terrain voxel space
    void initTerrain(int size);

    // Set voxel at position
    void setVoxel(int x, int y, int z, int material, float hardness);

    // Get voxel at position
    float getVoxel(int x, int y, int z);

    // Get material at position
    int getVoxelMaterial(int x, int y, int z);

    // Shutdown
    void shutdown();
}