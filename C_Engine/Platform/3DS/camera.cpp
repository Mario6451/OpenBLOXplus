#include "camera.h"
#include "render_pipeline.h"
#include "settings.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

namespace CameraSystem {

    static Camera gCamera;
    static TrackpadState gTrackpad;
    static std::vector<WaterSurface> gWaterSurfaces;
    static float gWaterTime = 0;
    static float* gTerrainData;         // Dynamically allocated when terrain is used
    static int* gTerrainMaterials;      // Dynamically allocated when terrain is used
    static int gTerrainSize = 0;

    void init() {
        gCamera.x = 0; gCamera.y = 10; gCamera.z = -15;
        gCamera.pitch = -20;
        gCamera.yaw = 0;
        gCamera.distance = 25;
        gCamera.targetDistance = 25;
        gCamera.zoomSpeed = 30;
        gCamera.zoomLerpSpeed = 8.0f;  // Smooth interpolation speed
        gCamera.rotationSpeed = 2;
        gCamera.mode = CameraMode::ThirdPerson;
        gCamera.fov = 60;
        gCamera.nearClip = 0.1f;
        gCamera.farClip = Settings::get().graphics.renderDistance;
        gCamera.minDistance = 3;
        gCamera.maxDistance = 100;
        gCamera.firstPersonThreshold = 3.5f;
        gCamera.headOffsetY = 1.6f;
        gCamera.autoFirstPerson = true;

        gTrackpad.active = false;
        gTrackpad.startX = 0; gTrackpad.startY = 0;
        gTrackpad.lastX = 0; gTrackpad.lastY = 0;
        gTrackpad.deltaX = 0; gTrackpad.deltaY = 0;

        // Add default water surface
        WaterSurface ws;
        ws.y = 0; ws.waveHeight = 0.3f; ws.waveSpeed = 1.0f;
        ws.r = 0.1f; ws.g = 0.4f; ws.b = 0.8f; ws.a = 0.6f;
        ws.reflectivity = 0.4f;
        gWaterSurfaces.push_back(ws);
    }

    void setPosition(float x, float y, float z) {
        gCamera.x = x; gCamera.y = y; gCamera.z = z;
    }

    void getPosition(float *x, float *y, float *z) {
        *x = gCamera.x; *y = gCamera.y; *z = gCamera.z;
    }

    void getTarget(float *x, float *y, float *z) {
        float radYaw = gCamera.yaw * M_PI / 180.0f;
        *x = gCamera.x - sinf(radYaw) * gCamera.distance;
        *y = gCamera.y + sinf(gCamera.pitch * M_PI / 180.0f) * gCamera.distance;
        *z = gCamera.z - cosf(radYaw) * gCamera.distance;
    }

    void setMode(CameraMode mode) { gCamera.mode = mode; }
    CameraMode getMode() { return gCamera.mode; }

    void processTrackpad(touchPosition *pos) {
        if (!pos) return;

        // Check if touch is within the trackpad area on bottom screen
        // Trackpad area: center-left portion of bottom screen
        const float TRACKPAD_X = 40;
        const float TRACKPAD_Y = 60;
        const float TRACKPAD_W = 120;
        const float TRACKPAD_H = 120;

        if (pos->px >= TRACKPAD_X && pos->px <= TRACKPAD_X + TRACKPAD_W &&
            pos->py >= TRACKPAD_Y && pos->py <= TRACKPAD_Y + TRACKPAD_H) {
            if (!gTrackpad.active) {
                gTrackpad.active = true;
                gTrackpad.startX = pos->px;
                gTrackpad.startY = pos->py;
                gTrackpad.lastX = pos->px;
                gTrackpad.lastY = pos->py;
            } else {
                gTrackpad.deltaX = (pos->px - gTrackpad.lastX) * gCamera.rotationSpeed * 0.5f;
                gTrackpad.deltaY = (pos->py - gTrackpad.lastY) * gCamera.rotationSpeed * 0.5f;
                gCamera.yaw += gTrackpad.deltaX;
                gCamera.pitch += gTrackpad.deltaY;
                // Clamp pitch
                if (gCamera.pitch > 80) gCamera.pitch = 80;
                if (gCamera.pitch < -80) gCamera.pitch = -80;
                gTrackpad.lastX = pos->px;
                gTrackpad.lastY = pos->py;
            }
        } else {
            gTrackpad.active = false;
            gTrackpad.deltaX = 0;
            gTrackpad.deltaY = 0;
        }
    }

    void processCStick(s16 cx, s16 cy) {
        if (abs(cx) > 16 || abs(cy) > 16) {
            gCamera.yaw += cx * 0.15f * gCamera.rotationSpeed;
            gCamera.pitch += cy * 0.15f * gCamera.rotationSpeed;
            if (gCamera.pitch > 80) gCamera.pitch = 80;
            if (gCamera.pitch < -80) gCamera.pitch = -80;
        }
    }

    void zoom(float amount) {
        gCamera.targetDistance += amount * gCamera.zoomSpeed;
        if (gCamera.targetDistance < gCamera.minDistance) gCamera.targetDistance = gCamera.minDistance;
        if (gCamera.targetDistance > gCamera.maxDistance) gCamera.targetDistance = gCamera.maxDistance;
    }

    float getDistance() { return gCamera.distance; }
    void setDistance(float d) {
        gCamera.targetDistance = d;
        if (gCamera.targetDistance < gCamera.minDistance) gCamera.targetDistance = gCamera.minDistance;
        if (gCamera.targetDistance > gCamera.maxDistance) gCamera.targetDistance = gCamera.maxDistance;
    }
    float getTargetDistance() { return gCamera.targetDistance; }
    float getMinDistance() { return gCamera.minDistance; }
    float getMaxDistance() { return gCamera.maxDistance; }
    void updateZoomSmoothing(float dt) {
        // Smooth lerp from current distance to target distance
        float lerpFactor = 1.0f - expf(-gCamera.zoomLerpSpeed * dt);
        gCamera.distance += (gCamera.targetDistance - gCamera.distance) * lerpFactor;
    }

    void update(float dt, float targetX, float targetY, float targetZ) {
        // Smooth zoom interpolation
        updateZoomSmoothing(dt);

        // Auto-switch to first person when zoomed very close to character
        if (gCamera.autoFirstPerson && gCamera.mode == CameraMode::ThirdPerson) {
            if (gCamera.distance <= gCamera.firstPersonThreshold) {
                gCamera.mode = CameraMode::FirstPerson;
            }
        }
        // Auto-switch back to third person when zoomed out
        if (gCamera.autoFirstPerson && gCamera.mode == CameraMode::FirstPerson) {
            if (gCamera.distance > gCamera.firstPersonThreshold) {
                gCamera.mode = CameraMode::ThirdPerson;
            }
        }

        if (gCamera.mode == CameraMode::FirstPerson) {
            // First-person: camera at character's head (eye level)
            gCamera.x = targetX;
            gCamera.y = targetY + gCamera.headOffsetY;
            gCamera.z = targetZ;
        } else {
            // Third-person orbit camera
            float radYaw = gCamera.yaw * M_PI / 180.0f;
            float radPitch = gCamera.pitch * M_PI / 180.0f;

            gCamera.x = targetX + sinf(radYaw) * gCamera.distance;
            gCamera.y = targetY + sinf(radPitch) * gCamera.distance * 0.5f + 2.0f;
            gCamera.z = targetZ + cosf(radYaw) * gCamera.distance;
        }
    }

    void applyCameraView(float ex, float ey, float ez, float cx, float cy, float cz) {
        RenderPipeline::lookAt(ex, ey, ez, cx, cy, cz, 0, 1, 0);
        RenderPipeline::perspective(gCamera.fov, 1280.0f / 720.0f, gCamera.nearClip, gCamera.farClip);
    }

    float getFOV() { return gCamera.fov; }
    void setFOV(float fov) { gCamera.fov = fov; }
    void setFarClip(float far) { gCamera.farClip = far; }

    // === Water ===
    void addWaterSurface(float height) {
        WaterSurface ws;
        ws.y = height; ws.waveHeight = 0.3f; ws.waveSpeed = 1.0f;
        ws.r = 0.1f; ws.g = 0.4f; ws.b = 0.8f; ws.a = 0.6f;
        ws.reflectivity = 0.4f;
        gWaterSurfaces.push_back(ws);
    }

    void updateWater(float dt) {
        gWaterTime += dt;
    }

    float getWaterHeight(float x, float z, float time) {
        if (gWaterSurfaces.empty()) return -1000;
        const WaterSurface &ws = gWaterSurfaces[0];
        float wave = sinf(x * 0.5f + time * ws.waveSpeed) * ws.waveHeight +
                     sinf(z * 0.3f + time * ws.waveSpeed * 0.7f) * ws.waveHeight * 0.5f;
        return ws.y + wave;
    }

    const WaterSurface* getWaterSurface(int index) {
        if (index >= 0 && index < (int)gWaterSurfaces.size())
            return &gWaterSurfaces[index];
        return nullptr;
    }

    int getWaterSurfaceCount() { return (int)gWaterSurfaces.size(); }

    // === Terrain ===
    void initTerrain(int size) {
        gTerrainSize = size;
        int totalVoxels = size * size * 8;
        gTerrainData = (float*)calloc(totalVoxels, sizeof(float));
        gTerrainMaterials = (int*)calloc(totalVoxels, sizeof(int));
    }

    void setVoxel(int x, int y, int z, int material, float hardness) {
        if (x < 0 || x >= gTerrainSize || y < 0 || y >= gTerrainSize || z < 0 || z >= 8) return;
        if (!gTerrainData || !gTerrainMaterials) return;
        int idx = x * gTerrainSize * 8 + y * 8 + z;
        gTerrainData[idx] = hardness;
        gTerrainMaterials[idx] = material;
    }

    float getVoxel(int x, int y, int z) {
        if (x < 0 || x >= gTerrainSize || y < 0 || y >= gTerrainSize || z < 0 || z >= 8) return 0;
        if (!gTerrainData) return 0;
        int idx = x * gTerrainSize * 8 + y * 8 + z;
        return gTerrainData[idx];
    }

    int getVoxelMaterial(int x, int y, int z) {
        if (x < 0 || x >= gTerrainSize || y < 0 || y >= gTerrainSize || z < 0 || z >= 8) return 0;
        if (!gTerrainMaterials) return 0;
        int idx = x * gTerrainSize * 8 + y * 8 + z;
        return gTerrainMaterials[idx];
    }

    void shutdown() {
        gWaterSurfaces.clear();
        if (gTerrainData) { free(gTerrainData); gTerrainData = nullptr; }
        if (gTerrainMaterials) { free(gTerrainMaterials); gTerrainMaterials = nullptr; }
        gTerrainSize = 0;
    }
}