#pragma once

#include <vector>
#include <string>

namespace LightingSystem {

    enum class LightType {
        Point,
        Spot,
        Directional
    };

    struct Light {
        LightType type;
        float x, y, z;              // position or direction
        float tx, ty, tz;           // target direction (spot)
        float r, g, b;              // color
        float intensity;             // brightness multiplier
        float range;                 // max distance
        float spotAngle;             // spot cone angle (degrees)
        bool enabled;
    };

    struct Decal {
        float x, y, z;              // position
        float rx, ry, rz;           // rotation (degrees)
        float width, height;        // size
        float u0, v0, u1, v1;       // UV coordinates
        void *texture;              // texture pointer
        bool isText;                // text decal
        std::string text;           // text content
        float textScale;
        float tr, tg, tb, ta;       // text color
    };

    struct Billboard {
        float x, y, z;              // world position
        float width, height;        // size
        float screenX, screenY;     // screen-space position (if 2D)
        float screenW, screenH;     // screen-space size
        void *texture;
        float r, g, b, a;           // tint
        bool alwaysFaceCamera;
        bool isScreenSpace;         // rendered in screen space
    };

    struct ViewportFrame {
        float screenX, screenY;     // top-left position
        float width, height;        // dimensions
        bool enabled;
    };

    // Initialize
    void init();

    // Shutdown
    void shutdown();

    // Add a light
    Light* addLight(LightType type, float x, float y, float z);

    // Remove a light
    void removeLight(Light *light);

    // Get lights
    const std::vector<Light*>& getLights();

    // Update lighting (apply era settings)
    void updateLighting(float dt);

    // Add a decal
    Decal* addDecal(float x, float y, float z, float w, float h, void *texture);

    // Add a text decal
    Decal* addTextDecal(float x, float y, float z, const std::string &text, float scale);

    // Remove decal
    void removeDecal(Decal *decal);

    // Get decals
    std::vector<Decal>& getDecals();

    // Add a billboard
    Billboard* addBillboard(float x, float y, float z, float w, float h, void *texture);

    // Remove billboard
    void removeBillboard(Billboard *billboard);

    // Get billboards
    std::vector<Billboard>& getBillboards();

    // Render all decals
    void renderDecals();

    // Render all billboards (face camera)
    void renderBillboards();

    // Render ViewportFrame overlay
    void renderViewportFrame(ViewportFrame *frame);

    // Set max lights based on graphics quality
    void setMaxLights(int max);

    // ClockTime (0-24 hours)
    inline float clockTime = 14.0f;
    inline float sunAzimuth = 45.0f, sunElevation = 60.0f;
    inline float moonAzimuth = 225.0f, moonElevation = 30.0f;

    // Fog settings
    inline bool fogEnabled = true;
    inline float fogR = 0.7f, fogG = 0.7f, fogB = 0.7f;
    inline float fogNear = 50.0f, fogFar = 200.0f;

    // Sky colors
    inline float skyTopR = 0.3f, skyTopG = 0.6f, skyTopB = 1.0f;
    inline float skyHorizonR = 0.8f, skyHorizonG = 0.85f, skyHorizonB = 1.0f;
    inline float skyBottomR = 1.0f, skyBottomG = 0.9f, skyBottomB = 0.7f;

    // Celestial bodies
    inline bool sunDiscVisible = true, moonDiscVisible = false, starsVisible = false;
    inline float sunColorR = 1.0f, sunColorG = 0.95f, sunColorB = 0.8f;
    inline float moonColorR = 0.9f, moonColorG = 0.9f, moonColorB = 0.95f;

    // Set ClockTime and update all derived state
    void setClockTime(float hours);

    // Update fog/sky/sun/moon based on ClockTime
    void updateTimeOfDay();

    // Render sky gradient (background quad)
    void renderSkyGradient();

    // Render sun disc
    void renderSunDisc(float camX, float camY, float camZ);

    // Render moon disc
    void renderMoonDisc(float camX, float camY, float camZ);

    // Render stars (simple dot pattern)
    void renderStars(float camX, float camY, float camZ);

    // Enable/disable fog
    void setFog(bool enabled, float nearDist, float farDist);

    // Get fog parameters for shader
    void getFogParams(float &r, float &g, float &b, float &near, float &far);

    // Get sky colors
    void getSkyColors(float &topR, float &topG, float &topB,
                      float &horR, float &horG, float &horB,
                      float &botR, float &botG, float &botB);

    // Get current ClockTime
    __attribute__((weak)) inline float getClockTime() { return clockTime; }

    // Load and apply era sky preset
    void loadSkyPreset(const std::string &name);

    // Load era lighting (changes ambient, fog, light count)
    void applyEraLighting(int era);

    // Apply lighting based on content year (0=2012, 1=2015, 2=2016, 3=2019, 4=2020, 5=2022)
    void applyContentYearLighting(int contentYearIndex);

    // Load WorldModel (.mesh file) and render
    void *loadWorldModel(const std::string &path);
    void renderWorldModel(void *model, float x, float y, float z, float rx, float ry, float rz);
}