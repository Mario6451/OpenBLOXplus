#include "lighting.h"
#include "render_pipeline.h"
#include "settings.h"
#include "font_manager.h"
#include "mesh_loader.h"
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace LightingSystem {

    static std::vector<Light*> gLights;
    static std::vector<Decal> gDecals;
    static std::vector<Billboard> gBillboards;
    static std::vector<void*> gWorldModels;
    static int gMaxLights = 8;
    static float gAmbientR = 0.3f, gAmbientG = 0.3f, gAmbientB = 0.3f;

    // ClockTime state
    static float gClockTime = 12.0f;
    static float gSunAzimuth = 0.0f, gSunElevation = M_PI / 2.0f;
    static float gMoonAzimuth = 0.0f, gMoonElevation = -M_PI / 2.0f;
    static bool gFogEnabled = false;
    static float gFogR = 0.5f, gFogG = 0.5f, gFogB = 0.5f;
    static float gFogNear = 50.0f, gFogFar = 200.0f;
    static float gSkyTopR = 0.3f, gSkyTopG = 0.5f, gSkyTopB = 0.9f;
    static float gSkyHorizonR = 0.6f, gSkyHorizonG = 0.7f, gSkyHorizonB = 0.9f;
    static float gSkyBottomR = 0.8f, gSkyBottomG = 0.85f, gSkyBottomB = 0.9f;
    static bool gSunDiscVisible = true, gMoonDiscVisible = false, gStarsVisible = false;
    static float gSunColorR = 1.0f, gSunColorG = 0.95f, gSunColorB = 0.8f;
    static float gMoonColorR = 0.9f, gMoonColorG = 0.9f, gMoonColorB = 0.85f;

    // Year-based lighting style settings
    static bool gBloomEnabled = false;
    static float gBloomIntensity = 0;
    static float gShadowSoftness = 0;
    static int gSkyGradientLayers = 1;
    static float gSunGlowSize = 0;
    static bool gCloudEnabled = false;
    static int gFogBlendMode = 0; // 0=linear, 1=distance lerp, 2=volumetric approx
    static int gTonemapMode = 0; // 0=none, 1=simple, 2=ACES-like

    // Simple pseudo-random for star positions
    static float gStars[128][3]; // x, y, z positions on sphere
    static int gStarCount = 0;

    static void initStars() {
        if (gStarCount > 0) return;
        srand(42);
        for (int i = 0; i < 128; i++) {
            // Random points on upper hemisphere
            float theta = (float)(rand() % 1000) / 1000.0f * M_PI;   // 0 to PI
            float phi = (float)(rand() % 3600) / 3600.0f * 2.0f * M_PI; // 0 to 2PI
            float r = 100.0f; // Radius of star sphere

            // Only place stars in upper hemisphere (theta < PI/2)
            if (theta < M_PI * 0.45f) {
                gStars[gStarCount][0] = r * sin(theta) * cos(phi);
                gStars[gStarCount][1] = r * cos(theta);
                gStars[gStarCount][2] = r * sin(theta) * sin(phi);
                gStarCount++;
            }
        }
    }

    void init() {
        gLights.clear();
        gDecals.clear();
        gBillboards.clear();
        initStars();
        // Initialize default sky (noon)
        setClockTime(12.0f);
        setFog(false, 50.0f, 200.0f);
        // Add default directional light
        Light *dir = new Light();
        dir->type = LightType::Directional;
        dir->x = 0; dir->y = -1; dir->z = 0;
        dir->r = 1; dir->g = 1; dir->b = 1;
        dir->intensity = 0.8f;
        dir->range = 200;
        dir->enabled = true;
        gLights.push_back(dir);
        applyEraLighting(0);
    }

    void shutdown() {
        for (auto l : gLights) delete l;
        gLights.clear();
        gDecals.clear();
        gBillboards.clear();
        gWorldModels.clear();
    }

    Light* addLight(LightType type, float x, float y, float z) {
        if ((int)gLights.size() >= gMaxLights) return nullptr;
        Light *l = new Light();
        l->type = type;
        l->x = x; l->y = y; l->z = z;
        l->tx = 0; l->ty = 0; l->tz = -1;
        l->r = 1; l->g = 1; l->b = 1;
        l->intensity = 1.0f;
        l->range = 30;
        l->spotAngle = 45;
        l->enabled = true;
        gLights.push_back(l);
        return l;
    }

    void removeLight(Light *light) {
        auto it = std::find(gLights.begin(), gLights.end(), light);
        if (it != gLights.end()) { gLights.erase(it); delete light; }
    }

    const std::vector<Light*>& getLights() { return gLights; }

    void updateLighting(float dt) { (void)dt; /* dynamic light updates */ }

    Decal* addDecal(float x, float y, float z, float w, float h, void *texture) {
        Decal *d = new Decal();
        d->x = x; d->y = y; d->z = z;
        d->rx = 0; d->ry = 0; d->rz = 0;
        d->width = w; d->height = h;
        d->u0 = 0; d->v0 = 0; d->u1 = 1; d->v1 = 1;
        d->texture = texture;
        d->isText = false;
        d->tr = 1; d->tg = 1; d->tb = 1; d->ta = 1;
        d->textScale = 1;
        gDecals.push_back(*d);
        return &gDecals.back();
    }

    Decal* addTextDecal(float x, float y, float z, const std::string &text, float scale) {
        Decal *d = new Decal();
        d->x = x; d->y = y; d->z = z;
        d->rx = 0; d->ry = 0; d->rz = 0;
        d->width = text.length() * 8 * scale;
        d->height = 12 * scale;
        d->texture = nullptr;
        d->isText = true;
        d->text = text;
        d->textScale = scale;
        d->tr = 0; d->tg = 0; d->tb = 0; d->ta = 1;
        gDecals.push_back(*d);
        return &gDecals.back();
    }

    void removeDecal(Decal *decal) {
        auto it = std::find_if(gDecals.begin(), gDecals.end(),
            [decal](const Decal &d) { return &d == (const Decal*)decal; });
        if (it != gDecals.end()) gDecals.erase(it);
    }

    std::vector<Decal>& getDecals() { return gDecals; }

    Billboard* addBillboard(float x, float y, float z, float w, float h, void *texture) {
        Billboard *b = new Billboard();
        b->x = x; b->y = y; b->z = z;
        b->width = w; b->height = h;
        b->texture = texture;
        b->r = 1; b->g = 1; b->b = 1; b->a = 1;
        b->alwaysFaceCamera = true;
        b->isScreenSpace = false;
        gBillboards.push_back(*b);
        return &gBillboards.back();
    }

    void removeBillboard(Billboard *billboard) {
        auto it = std::find_if(gBillboards.begin(), gBillboards.end(),
            [billboard](const Billboard &b) { return &b == (const Billboard*)billboard; });
        if (it != gBillboards.end()) gBillboards.erase(it);
    }

    std::vector<Billboard>& getBillboards() { return gBillboards; }

    void renderDecals() {
        RenderPipeline::setBlend(true);
        for (const Decal &d : gDecals) {
            if (d.isText) {
                // Render text using font manager (3D text not yet supported, use 2D fallback)
                /* FontManager::drawText3D(tx, ty, tz, d.text.c_str(), d.textScale, d.tr, d.tg, d.tb, d.ta); */
            } else if (d.texture) {
                RenderPipeline::pushMatrix();
                RenderPipeline::translate(d.x, d.y, d.z);
                RenderPipeline::rotate(d.rx, 1, 0, 0);
                RenderPipeline::rotate(d.ry, 0, 1, 0);
                RenderPipeline::rotate(d.rz, 0, 0, 1);
                RenderPipeline::drawTexturedRect(-d.width/2, -d.height/2, d.width, d.height, d.texture, d.tr, d.tg, d.tb, d.ta);
                RenderPipeline::popMatrix();
            }
        }
        RenderPipeline::setBlend(false);
    }

    void renderBillboards() {
        RenderPipeline::setBlend(true);
        for (Billboard &b : gBillboards) {
            if (b.isScreenSpace) {
                RenderPipeline::drawRect(b.screenX, b.screenY, b.screenW, b.screenH, b.r, b.g, b.b, b.a);
            } else if (b.texture) {
                RenderPipeline::pushMatrix();
                RenderPipeline::translate(b.x, b.y, b.z);
                if (b.alwaysFaceCamera) {
                    // Billboard faces camera (simplified)
                    RenderPipeline::rotate(0, 0, 1, 0);
                }
                RenderPipeline::drawTexturedRect(-b.width/2, -b.height/2, b.width, b.height, b.texture, b.r, b.g, b.b, b.a);
                RenderPipeline::popMatrix();
            }
        }
        RenderPipeline::setBlend(false);
    }

    void renderViewportFrame(ViewportFrame *frame) {
        if (!frame || !frame->enabled) return;
        RenderPipeline::setBlend(true);
        RenderPipeline::drawRect(frame->screenX, frame->screenY, frame->width, frame->height, 0, 0, 0, 0.5f);
        RenderPipeline::setBlend(false);
    }

    void setMaxLights(int max) { gMaxLights = max; }

    // Helper: lerp between two floats
    static float lerp(float a, float b, float t) { return a + (b - a) * t; }

    // Helper: lerp between two colors (stored as r,g,b references)
    static void lerpColor(float &r, float &g, float &b,
                          float r1, float g1, float b1,
                          float r2, float g2, float b2, float t) {
        r = lerp(r1, r2, t);
        g = lerp(g1, g2, t);
        b = lerp(b1, b2, t);
    }

    void setClockTime(float hours) {
        gClockTime = fmodf(hours, 24.0f);
        if (gClockTime < 0.0f) gClockTime += 24.0f;
        updateTimeOfDay();
    }

    void updateTimeOfDay() {
        float t = gClockTime;

        // Sun: rises at 6, sets at 18, peaks at 12
        // Moon: rises at 18, sets at 6, peaks at 0/24
        float sunCycle = fmodf(t - 6.0f, 24.0f) / 24.0f; // 0=sunrise, 0.5=noon, 1.0=sunset
        float moonCycle = fmodf(t - 18.0f, 24.0f) / 24.0f;

        // Sun elevation (positive = above horizon)
        gSunElevation = sinf(sunCycle * M_PI * 2.0f) * M_PI * 0.5f;
        gSunAzimuth = sunCycle * M_PI * 2.0f;
        gSunDiscVisible = (gSunElevation > -0.1f);

        // Moon elevation
        gMoonElevation = sinf(moonCycle * M_PI * 2.0f) * M_PI * 0.5f;
        gMoonAzimuth = moonCycle * M_PI * 2.0f;
        gMoonDiscVisible = (gMoonElevation > -0.1f);

        // Stars visible during night
        gStarsVisible = (t > 19.0f || t < 5.0f);

        // Sky color transitions
        // Noon (12): bright blue top, light blue horizon
        // Sunset (18): orange/red
        // Midnight (0): dark blue/black
        // Dawn (6): pink/orange

        float dayFactor = 0.0f; // 0=night, 1=day
        if (t >= 6.0f && t < 18.0f) {
            dayFactor = 1.0f;
            // Reduce near dawn/dusk
            if (t < 8.0f) dayFactor = (t - 6.0f) / 2.0f;
            else if (t > 16.0f) dayFactor = (18.0f - t) / 2.0f;
        } else if (t >= 18.0f && t < 20.0f) {
            dayFactor = (20.0f - t) / 2.0f;
        } else if (t >= 4.0f && t < 6.0f) {
            dayFactor = (t - 4.0f) / 2.0f;
        }

        // Sky top colors
        if (t >= 18.0f && t < 20.0f) {
            // Sunset transition
            float sunsetT = (t - 18.0f) / 2.0f;
            lerpColor(gSkyTopR, gSkyTopG, gSkyTopB,
                      0.3f, 0.5f, 0.9f,   // day
                      0.1f, 0.05f, 0.15f,  // night
                      sunsetT);
            lerpColor(gSkyHorizonR, gSkyHorizonG, gSkyHorizonB,
                      0.6f, 0.7f, 0.9f,
                      0.6f, 0.2f, 0.15f,   // sunset orange
                      sunsetT * 0.5f);
            lerpColor(gSunColorR, gSunColorG, gSunColorB,
                      1.0f, 0.95f, 0.8f,
                      1.0f, 0.4f, 0.2f,    // red sunset sun
                      sunsetT);
        } else if (t >= 4.0f && t < 6.0f) {
            // Dawn transition
            float dawnT = (t - 4.0f) / 2.0f;
            lerpColor(gSkyTopR, gSkyTopG, gSkyTopB,
                      0.1f, 0.05f, 0.15f,
                      0.3f, 0.5f, 0.9f,
                      dawnT);
            lerpColor(gSkyHorizonR, gSkyHorizonG, gSkyHorizonB,
                      0.6f, 0.2f, 0.15f,
                      0.6f, 0.7f, 0.9f,
                      dawnT);
            lerpColor(gSunColorR, gSunColorG, gSunColorB,
                      1.0f, 0.4f, 0.2f,
                      1.0f, 0.95f, 0.8f,
                      dawnT);
        } else if (dayFactor >= 0.9f) {
            // Full day
            gSkyTopR = 0.3f; gSkyTopG = 0.5f; gSkyTopB = 0.9f;
            gSkyHorizonR = 0.6f; gSkyHorizonG = 0.7f; gSkyHorizonB = 0.9f;
            gSkyBottomR = 0.8f; gSkyBottomG = 0.85f; gSkyBottomB = 0.9f;
            gSunColorR = 1.0f; gSunColorG = 0.95f; gSunColorB = 0.8f;
        } else {
            // Night
            gSkyTopR = 0.05f; gSkyTopG = 0.05f; gSkyTopB = 0.15f;
            gSkyHorizonR = 0.08f; gSkyHorizonG = 0.08f; gSkyHorizonB = 0.12f;
            gSkyBottomR = 0.1f; gSkyBottomG = 0.1f; gSkyBottomB = 0.15f;
            gSunColorR = 1.0f; gSunColorG = 0.95f; gSunColorB = 0.8f;
        }

        gSkyBottomR = gSkyHorizonR * 0.9f;
        gSkyBottomG = gSkyHorizonG * 0.9f;
        gSkyBottomB = gSkyHorizonB * 0.9f;

        // Moon is always pale white
        gMoonColorR = 0.9f; gMoonColorG = 0.9f; gMoonColorB = 0.85f;

        // Update fog color to match horizon
        if (gFogEnabled) {
            gFogR = gSkyHorizonR;
            gFogG = gSkyHorizonG;
            gFogB = gSkyHorizonB;
        }

        // Update directional light based on sun position
        if (!gLights.empty()) {
            Light* sun = gLights[0];
            sun->r = gSunColorR * dayFactor;
            sun->g = gSunColorG * dayFactor;
            sun->b = gSunColorB * dayFactor;
            sun->intensity = 0.3f + 0.5f * dayFactor;
        }
    }

    void renderSkyGradient() {
        // Render a large quad as sky background
        RenderPipeline::pushMatrix();
        float size = 500.0f;

        // Top face (looking up)
        RenderPipeline::drawQuad(
            -size, size, -size,
             size, size, -size,
             size, size,  size,
            -size, size,  size,
            gSkyTopR, gSkyTopG, gSkyTopB, 1.0f
        );

        // Horizon faces
        float horR = gSkyHorizonR, horG = gSkyHorizonG, horB = gSkyHorizonB;

        // Back
        RenderPipeline::drawQuad(
            -size, -size, -size,
             size, -size, -size,
             size,  size, -size,
            -size,  size, -size,
            horR, horG, horB, 1.0f
        );

        // Front
        RenderPipeline::drawQuad(
            -size, -size,  size,
             size, -size,  size,
             size,  size,  size,
            -size,  size,  size,
            horR, horG, horB, 1.0f
        );

        // Right
        RenderPipeline::drawQuad(
             size, -size, -size,
             size, -size,  size,
             size,  size,  size,
             size,  size, -size,
            horR, horG, horB, 1.0f
        );

        // Left
        RenderPipeline::drawQuad(
            -size, -size,  size,
            -size, -size, -size,
            -size,  size, -size,
            -size,  size,  size,
            horR, horG, horB, 1.0f
        );

        // Bottom
        float botR = gSkyBottomR, botG = gSkyBottomG, botB = gSkyBottomB;
        RenderPipeline::drawQuad(
            -size, -size, -size,
             size, -size, -size,
             size, -size,  size,
            -size, -size,  size,
            botR, botG, botB, 1.0f
        );

        RenderPipeline::popMatrix();
    }

    void renderSunDisc(float camX, float camY, float camZ) {
        if (!gSunDiscVisible) return;
        RenderPipeline::pushMatrix();
        RenderPipeline::translate(camX, camY, camZ);

        // Convert spherical to Cartesian for sun direction
        float sunDirX = cosf(gSunElevation) * cosf(gSunAzimuth);
        float sunDirY = sinf(gSunElevation);
        float sunDirZ = cosf(gSunElevation) * sinf(gSunAzimuth);

        float dist = 150.0f;
        float sx = camX + sunDirX * dist;
        float sy = camY + sunDirY * dist;
        float sz = camZ + sunDirZ * dist;

        RenderPipeline::translate(sx - camX, sy - camY, sz - camZ);

        // Sun disc as a bright circle (render as textured quad)
        RenderPipeline::drawCircle(0, 0, 5.0f, gSunColorR, gSunColorG, gSunColorB, 1.0f);

        RenderPipeline::popMatrix();
    }

    void renderMoonDisc(float camX, float camY, float camZ) {
        if (!gMoonDiscVisible) return;
        RenderPipeline::pushMatrix();
        RenderPipeline::translate(camX, camY, camZ);

        float moonDirX = cosf(gMoonElevation) * cosf(gMoonAzimuth);
        float moonDirY = sinf(gMoonElevation);
        float moonDirZ = cosf(gMoonElevation) * sinf(gMoonAzimuth);

        float dist = 150.0f;
        float mx = camX + moonDirX * dist;
        float my = camY + moonDirY * dist;
        float mz = camZ + moonDirZ * dist;

        RenderPipeline::translate(mx - camX, my - camY, mz - camZ);
        RenderPipeline::drawCircle(0, 0, 4.0f, gMoonColorR, gMoonColorG, gMoonColorB, 1.0f);

        RenderPipeline::popMatrix();
    }

    void renderStars(float camX, float camY, float camZ) {
        if (!gStarsVisible || gStarCount == 0) return;
        RenderPipeline::pushMatrix();
        RenderPipeline::translate(camX, camY, camZ);

        RenderPipeline::setBlend(true);
        for (int i = 0; i < gStarCount; i++) {
            float sx = gStars[i][0] + camX;
            float sy = gStars[i][1] + camY;
            float sz = gStars[i][2] + camZ;

            RenderPipeline::drawPixel(sx - camX, sy - camY, sz - camZ,
                                      1.0f, 1.0f, 1.0f, 0.8f);
        }
        RenderPipeline::setBlend(false);

        RenderPipeline::popMatrix();
    }

    void setFog(bool enabled, float nearDist, float farDist) {
        gFogEnabled = enabled;
        gFogNear = nearDist;
        gFogFar = farDist;
        if (enabled) {
            gFogR = gSkyHorizonR;
            gFogG = gSkyHorizonG;
            gFogB = gSkyHorizonB;
        }
    }

    void getFogParams(float &r, float &g, float &b, float &near, float &far) {
        r = gFogR; g = gFogG; b = gFogB;
        near = gFogNear; far = gFogFar;
    }

    void getSkyColors(float &topR, float &topG, float &topB,
                      float &horR, float &horG, float &horB,
                      float &botR, float &botG, float &botB) {
        topR = gSkyTopR; topG = gSkyTopG; topB = gSkyTopB;
        horR = gSkyHorizonR; horG = gSkyHorizonG; horB = gSkyHorizonB;
        botR = gSkyBottomR; botG = gSkyBottomG; botB = gSkyBottomB;
    }

    void loadSkyPreset(const std::string &name) {
        if (name == "default") {
            setClockTime(12.0f);
        } else if (name == "sunset") {
            setClockTime(18.3f);
        } else if (name == "midnight") {
            setClockTime(0.0f);
        } else if (name == "dawn") {
            setClockTime(5.5f);
        } else if (name == "dusk") {
            setClockTime(18.8f);
        } else if (name == "halloween") {
            setClockTime(22.0f);
            gSkyTopR = 0.05f; gSkyTopG = 0.02f; gSkyTopB = 0.05f;
            gSkyHorizonR = 0.15f; gSkyHorizonG = 0.05f; gSkyHorizonB = 0.05f;
            gSkyBottomR = 0.1f; gSkyBottomG = 0.03f; gSkyBottomB = 0.03f;
            gFogR = 0.1f; gFogG = 0.05f; gFogB = 0.05f;
            gFogNear = 20.0f; gFogFar = 100.0f;
        } else if (name == "christmas") {
            setClockTime(2.0f);
            gSkyTopR = 0.1f; gSkyTopG = 0.1f; gSkyTopB = 0.2f;
            gSkyHorizonR = 0.15f; gSkyHorizonG = 0.15f; gSkyHorizonB = 0.25f;
            gFogNear = 30.0f; gFogFar = 150.0f;
        }
    }

    void applyEraLighting(int era) {
        // Era 0 = 2007 Classic, 1 = 2011, 2 = 2012, 3 = 2015, 4 = Modern
        switch (era) {
            case 0: // 2007 Classic
                gAmbientR = 0.3f; gAmbientG = 0.3f; gAmbientB = 0.3f;
                gMaxLights = 2;
                gBloomEnabled = false;
                gBloomIntensity = 0;
                gShadowSoftness = 0;
                gSkyGradientLayers = 1;
                gSunGlowSize = 0;
                gCloudEnabled = false;
                gFogBlendMode = 0; // Linear
                gTonemapMode = 0; // None
                break;
            case 1: // 2011
                gAmbientR = 0.4f; gAmbientG = 0.4f; gAmbientB = 0.4f;
                gMaxLights = 4;
                gBloomEnabled = false;
                gBloomIntensity = 0;
                gShadowSoftness = 0;
                gSkyGradientLayers = 1;
                gSunGlowSize = 0;
                gCloudEnabled = false;
                gFogBlendMode = 0;
                gTonemapMode = 0;
                break;
            case 2: // 2012 (Classic)
                gAmbientR = 0.35f; gAmbientG = 0.35f; gAmbientB = 0.4f;
                gMaxLights = 6;
                gBloomEnabled = false;
                gBloomIntensity = 0;
                gShadowSoftness = 0;
                gSkyGradientLayers = 1; // Single flat color
                gSunGlowSize = 0; // No glow
                gCloudEnabled = false;
                gFogBlendMode = 0;
                gTonemapMode = 0;
                break;
            case 3: // 2015
                gAmbientR = 0.3f; gAmbientG = 0.35f; gAmbientB = 0.4f;
                gMaxLights = 8;
                gBloomEnabled = false;
                gBloomIntensity = 0;
                gShadowSoftness = 0.2f; // Slightly softer
                gSkyGradientLayers = 2; // Top + horizon gradient
                gSunGlowSize = 0.3f; // Slight glow
                gCloudEnabled = false;
                gFogBlendMode = 1; // Distance-based lerp
                gTonemapMode = 0;
                break;
            case 4: // 2016 (Modern)
                gAmbientR = 0.25f; gAmbientG = 0.3f; gAmbientB = 0.38f;
                gMaxLights = 10;
                gBloomEnabled = true;
                gBloomIntensity = 0.3f;
                gShadowSoftness = 0.4f;
                gSkyGradientLayers = 3; // Top + horizon + bottom
                gSunGlowSize = 0.5f;
                gCloudEnabled = true; // Billboarded clouds
                gFogBlendMode = 1;
                gTonemapMode = 0;
                break;
            case 5: // 2019
                gAmbientR = 0.22f; gAmbientG = 0.28f; gAmbientB = 0.38f;
                gMaxLights = 12;
                gBloomEnabled = true;
                gBloomIntensity = 0.5f;
                gShadowSoftness = 0.6f;
                gSkyGradientLayers = 3;
                gSunGlowSize = 0.8f; // HDR-style glow
                gCloudEnabled = true;
                gFogBlendMode = 1;
                gTonemapMode = 1; // Simple tonemap
                break;
            case 6: // 2020
                gAmbientR = 0.2f; gAmbientG = 0.26f; gAmbientB = 0.36f;
                gMaxLights = 14;
                gBloomEnabled = true;
                gBloomIntensity = 0.7f; // Advanced bloom
                gShadowSoftness = 0.8f;
                gSkyGradientLayers = 4; // Ultra sky layers
                gSunGlowSize = 1.0f;
                gCloudEnabled = true; // Layered clouds
                gFogBlendMode = 2; // Volumetric approximation
                gTonemapMode = 2; // ACES-like
                break;
            case 7: // 2022
                gAmbientR = 0.18f; gAmbientG = 0.24f; gAmbientB = 0.34f;
                gMaxLights = 16;
                gBloomEnabled = true;
                gBloomIntensity = 0.9f;
                gShadowSoftness = 1.0f;
                gSkyGradientLayers = 5; // Multi-layer procedural
                gSunGlowSize = 1.2f;
                gCloudEnabled = true;
                gFogBlendMode = 2;
                gTonemapMode = 2;
                break;
            default: // Modern
                gAmbientR = 0.25f; gAmbientG = 0.3f; gAmbientB = 0.4f;
                gMaxLights = 16;
                gBloomEnabled = true;
                gBloomIntensity = 0.5f;
                gShadowSoftness = 0.5f;
                gSkyGradientLayers = 3;
                gSunGlowSize = 0.6f;
                gCloudEnabled = true;
                gFogBlendMode = 1;
                gTonemapMode = 1;
                break;
        }
        RenderPipeline::setAmbientLight(gAmbientR, gAmbientG, gAmbientB, 1.0f);
        /* RenderPipeline::setBloom(gBloomEnabled, gBloomIntensity); */
    }

    // Convert Settings::ContentYear to era index
    void applyContentYearLighting(int contentYearIndex) {
        // contentYearIndex: 0=2012, 1=2015, 2=2016, 3=2019, 4=2020, 5=2022
        applyEraLighting(contentYearIndex + 2); // Offset: 2012=era2, 2015=era3, etc.
    }

    void *loadWorldModel(const std::string &path) {
        MeshData data = MeshLoader::loadMeshFromFile(path);
        return MeshLoader::uploadMeshToGPU(data);
    }

    void renderWorldModel(void *model, float x, float y, float z, float rx, float ry, float rz) {
        if (!model) return;
        RenderPipeline::pushMatrix();
        RenderPipeline::translate(x, y, z);
        RenderPipeline::rotate(rx, 1, 0, 0);
        RenderPipeline::rotate(ry, 0, 1, 0);
        RenderPipeline::rotate(rz, 0, 0, 1);
        RenderPipeline::drawMesh(model, 1.0f, 1.0f, 1.0f, 1.0f);
        RenderPipeline::popMatrix();
    }
}