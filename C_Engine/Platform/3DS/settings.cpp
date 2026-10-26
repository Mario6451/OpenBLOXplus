#include "settings.h"
#include "asset_loader.h"
#include "ui_asset_loader.h"
#include "CoreEngine/debug_logger.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace Settings {

    static AllSettings gSettings;

    void init() {
        gSettings = AllSettings();

        // Try loading settings - prefer JSON, fall back to INI for backward compat
        if (!load("sdmc:/OpenBLOX/settings.json")) {
            if (!load("sdmc:/OpenBLOX/settings.sav")) {
                // No settings file found, save defaults as JSON
                printf("No settings file found. Creating default settings.json\n");
                save("sdmc:/OpenBLOX/settings.json");
                return;
            }
        }
    }

    AllSettings& get() {
        return gSettings;
    }

    void applyPreset(PerformancePreset preset) {
        switch (preset) {
            case PerformancePreset::Performance:
                gSettings.graphics.era = GraphicsEra::Classic2007;
                gSettings.graphics.bloom = false;
                gSettings.graphics.fog = false;
                gSettings.graphics.shadows = false;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 0;
                gSettings.graphics.textureAutoScale = true;
                gSettings.graphics.resolutionScale = 50;
                gSettings.graphics.antiAlias = false;
                gSettings.graphics.renderDistance = 50;
                gSettings.graphics.shadowQuality = ShadowQuality::Off;
                gSettings.graphics.bloomIntensity = 0;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::None;
                gSettings.graphics.terrainMeshDensity = TerrainMeshDensity::Low;
                gSettings.sound.spatialAudio = false;
                gSettings.sound.collisionSounds = false;
                break;
            case PerformancePreset::Balanced:
                gSettings.graphics.era = GraphicsEra::Year2011;
                gSettings.graphics.bloom = false;
                gSettings.graphics.fog = true;
                gSettings.graphics.shadows = false;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 2;
                gSettings.graphics.textureAutoScale = true;
                gSettings.graphics.resolutionScale = 75;
                gSettings.graphics.antiAlias = false;
                gSettings.graphics.renderDistance = 100;
                gSettings.graphics.shadowQuality = ShadowQuality::Normal;
                gSettings.graphics.bloomIntensity = 0;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::None;
                gSettings.graphics.terrainMeshDensity = TerrainMeshDensity::Medium;
                gSettings.sound.spatialAudio = true;
                gSettings.sound.collisionSounds = true;
                break;
            case PerformancePreset::Quality:
                gSettings.graphics.era = GraphicsEra::Modern;
                gSettings.graphics.bloom = true;
                gSettings.graphics.fog = true;
                gSettings.graphics.shadows = true;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 3;
                gSettings.graphics.textureAutoScale = false;
                gSettings.graphics.resolutionScale = 100;
                gSettings.graphics.antiAlias = true;
                gSettings.graphics.renderDistance = 200;
                gSettings.graphics.shadowQuality = ShadowQuality::Soft;
                gSettings.graphics.bloomIntensity = 50;
                gSettings.graphics.vignette = true;
                gSettings.graphics.filmGrain = true;
                gSettings.graphics.ambientOcclusion = true;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::Cool;
                gSettings.graphics.terrainMeshDensity = TerrainMeshDensity::High;
                gSettings.sound.spatialAudio = true;
                gSettings.sound.collisionSounds = true;
                break;
        }
        gSettings.preset = preset;
    }

    void applyYearPreset(ContentYear year) {
        switch (year) {
            case ContentYear::Year2007:
                gSettings.graphics.era = GraphicsEra::Classic2007;
                gSettings.graphics.partStyle = PartStyle::Beveled;
                gSettings.graphics.bloom = false;
                gSettings.graphics.fog = false;
                gSettings.graphics.shadows = false;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 0;
                gSettings.graphics.textureAutoScale = true;
                gSettings.graphics.resolutionScale = 50;
                gSettings.graphics.antiAlias = false;
                gSettings.graphics.renderDistance = 50;
                gSettings.graphics.shadowQuality = ShadowQuality::Off;
                gSettings.graphics.bloomIntensity = 0;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::None;
                break;
            case ContentYear::Year2011:
                gSettings.graphics.era = GraphicsEra::Year2011;
                gSettings.graphics.partStyle = PartStyle::Beveled;
                gSettings.graphics.bloom = false;
                gSettings.graphics.fog = false;
                gSettings.graphics.shadows = false;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 0;
                gSettings.graphics.textureAutoScale = true;
                gSettings.graphics.resolutionScale = 65;
                gSettings.graphics.antiAlias = false;
                gSettings.graphics.renderDistance = 65;
                gSettings.graphics.shadowQuality = ShadowQuality::Off;
                gSettings.graphics.bloomIntensity = 0;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::None;
                break;
            case ContentYear::Year2012:
                gSettings.graphics.era = GraphicsEra::Year2012;
                gSettings.graphics.partStyle = PartStyle::Beveled;
                gSettings.graphics.bloom = false;
                gSettings.graphics.fog = false;
                gSettings.graphics.shadows = false;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 0;
                gSettings.graphics.textureAutoScale = true;
                gSettings.graphics.resolutionScale = 75;
                gSettings.graphics.antiAlias = false;
                gSettings.graphics.renderDistance = 80;
                gSettings.graphics.shadowQuality = ShadowQuality::Crisp;
                gSettings.graphics.bloomIntensity = 0;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::None;
                break;
            case ContentYear::Year2015:
                gSettings.graphics.era = GraphicsEra::Year2015;
                gSettings.graphics.partStyle = PartStyle::Outlined;
                gSettings.graphics.bloom = false;
                gSettings.graphics.fog = true;
                gSettings.graphics.shadows = false;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 1;
                gSettings.graphics.textureAutoScale = true;
                gSettings.graphics.resolutionScale = 80;
                gSettings.graphics.antiAlias = false;
                gSettings.graphics.renderDistance = 100;
                gSettings.graphics.shadowQuality = ShadowQuality::Normal;
                gSettings.graphics.bloomIntensity = 0;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::None;
                break;
            case ContentYear::Year2016:
                gSettings.graphics.era = GraphicsEra::Year2012;
                gSettings.graphics.partStyle = PartStyle::Outlined;
                gSettings.graphics.bloom = false;
                gSettings.graphics.fog = true;
                gSettings.graphics.shadows = true;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 1;
                gSettings.graphics.textureAutoScale = true;
                gSettings.graphics.resolutionScale = 85;
                gSettings.graphics.antiAlias = false;
                gSettings.graphics.renderDistance = 120;
                gSettings.graphics.shadowQuality = ShadowQuality::Normal;
                gSettings.graphics.bloomIntensity = 0;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::None;
                break;
            case ContentYear::Year2019:
                gSettings.graphics.era = GraphicsEra::Modern;
                gSettings.graphics.partStyle = PartStyle::Smooth;
                gSettings.graphics.bloom = true;
                gSettings.graphics.fog = true;
                gSettings.graphics.shadows = true;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 2;
                gSettings.graphics.textureAutoScale = false;
                gSettings.graphics.resolutionScale = 90;
                gSettings.graphics.antiAlias = true;
                gSettings.graphics.renderDistance = 150;
                gSettings.graphics.shadowQuality = ShadowQuality::Normal;
                gSettings.graphics.bloomIntensity = 30;
                gSettings.graphics.vignette = false;
                gSettings.graphics.filmGrain = false;
                gSettings.graphics.ambientOcclusion = false;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::Warm;
                break;
            case ContentYear::Year2022:
                gSettings.graphics.era = GraphicsEra::Modern;
                gSettings.graphics.partStyle = PartStyle::Smooth;
                gSettings.graphics.bloom = true;
                gSettings.graphics.fog = true;
                gSettings.graphics.shadows = true;
                gSettings.graphics.skybox = true;
                gSettings.graphics.particleQuality = 3;
                gSettings.graphics.textureAutoScale = false;
                gSettings.graphics.resolutionScale = 100;
                gSettings.graphics.antiAlias = true;
                gSettings.graphics.renderDistance = 200;
                gSettings.graphics.shadowQuality = ShadowQuality::Soft;
                gSettings.graphics.bloomIntensity = 40;
                gSettings.graphics.vignette = true;
                gSettings.graphics.filmGrain = true;
                gSettings.graphics.ambientOcclusion = true;
                gSettings.graphics.colorCorrection = GraphicsColorCorrection::Cool;
                break;
        }
        gSettings.general.contentYear = year;
    }

    // Helper to create a toggle option
    static SettingsOption toggleOpt(const char* label, bool val) {
        SettingsOption opt;
        opt.label = label;
        opt.value = val ? 1 : 0;
        opt.minValue = 0;
        opt.maxValue = 1;
        opt.unit = "";
        opt.isToggle = true;
        opt.isDropdown = false;
        opt.dropdownLabels = {"Off", "On"};
        return opt;
    }

    // Helper to create a slider option
    static SettingsOption sliderOpt(const char* label, int val, int min, int max, const char* unit = "") {
        SettingsOption opt;
        opt.label = label;
        opt.value = val;
        opt.minValue = min;
        opt.maxValue = max;
        opt.unit = unit;
        opt.isToggle = false;
        opt.isDropdown = false;
        return opt;
    }

    // Helper to create a dropdown option
    static SettingsOption dropdownOpt(const char* label, int val, const char** labels, int count) {
        SettingsOption opt;
        opt.label = label;
        opt.value = val;
        opt.minValue = 0;
        opt.maxValue = count - 1;
        opt.unit = "";
        opt.isToggle = false;
        opt.isDropdown = true;
        for (int i = 0; i < count; i++) opt.dropdownLabels.push_back(labels[i]);
        return opt;
    }

    static const char* eraLabels[] = {
        "2007 Classic", "2011", "2012", "2015", "Modern"
    };
    static const char* yearLabels[] = {
        "2007", "2011", "2012", "2015", "2016", "2019", "2022"
    };
    static const char* partStyleLabels[] = {
        "Beveled (2008-2013)", "Outlined (2013-2019)", "Smooth (2019-present)"
    };
    static const char* lodModeLabels[] = {
        "Distance-based", "Fixed Level"
    };
    static const char* fixedLODLabels[] = {
        "Level 1", "Level 2", "Level 3", "Level 4"
    };
    static const char* particleLabels[] = { "Off", "Low", "Medium", "High" };
    static const char* moveLabels[] = { "Joystick", "D-Pad", "Circlepad" };
    static const char* presetLabels[] = { "Performance", "Balanced", "Quality" };
    static const char* yearPresetLabels[] = { "2012", "2015", "2016", "2019", "2022" };
    static const char* fluidSynthInterpLabels[] = {
        "None (Raw)", "Nearest", "Linear", "4th Order"
    };
    static const char* terrainMeshLabels[] = {"Low", "Medium", "High"};
    static const char* shadowQualityLabels[] = {"Off", "Crisp", "Normal", "Soft", "High Detail"};
    static const char* colorCorrectionLabels[] = {"None", "Warm", "Cool", "Vibrant", "Desaturated"};
    static const char* physicsLabels[] = {
        "TPE", "PGS Solver", "Spring Physics"
    };

    // Helper to create a text input option
    static SettingsOption textInputOpt(const char* label, const std::string& val) {
        SettingsOption opt;
        opt.label = label;
        opt.value = 0;
        opt.minValue = 0;
        opt.maxValue = 0;
        opt.unit = "";
        opt.isToggle = false;
        opt.isDropdown = false;
        opt.isTextInput = true;
        opt.textValue = val;
        return opt;
    }

    std::vector<SettingsOption> getCategoryOptions(SettingsCategory cat) {
        std::vector<SettingsOption> opts;
        switch (cat) {
            case SettingsCategory::General:
                opts.push_back(toggleOpt("Safechat", gSettings.general.safechat));
                opts.push_back(toggleOpt("Offline Mode", gSettings.general.offlineMode));
                opts.push_back(textInputOpt("Asset Delivery Proxy", gSettings.general.assetdeliveryUrl));
                opts.push_back(textInputOpt("Toolbox Proxy", gSettings.general.toolboxProxy));
                opts.push_back(dropdownOpt("Physics Engine", (int)gSettings.general.physicsEngine, physicsLabels, 3));
                break;
            case SettingsCategory::Graphics:
                // Year preset buttons (2007, 2011, 2012, 2015, 2016, 2019, 2020, 2022)
                {
                    static const char* yearBtnLabels[] = { "2007", "2011", "2012", "2015", "2016", "2019", "2022" };
                    int currentYear = (int)gSettings.general.contentYear;
                    for (int i = 0; i < 7; i++) {
                        SettingsOption yearBtn;
                        yearBtn.label = yearBtnLabels[i];
                        yearBtn.value = (i == currentYear) ? 1 : 0;
                        yearBtn.minValue = 0;
                        yearBtn.maxValue = 1;
                        yearBtn.unit = "";
                        yearBtn.isToggle = true;
                        yearBtn.isDropdown = false;
                        yearBtn.dropdownLabels = {"Off", "On"};
                        opts.push_back(yearBtn);
                    }
                }
                opts.push_back(dropdownOpt("Part Style", (int)gSettings.graphics.partStyle, partStyleLabels, 3));
                opts.push_back(dropdownOpt("Level of Detail", (int)gSettings.graphics.lodMode, lodModeLabels, 2));
                opts.push_back(dropdownOpt("Fixed LOD Level", gSettings.graphics.fixedLODLevel - 1, fixedLODLabels, 4));
                opts.push_back(sliderOpt("Render Distance", gSettings.graphics.renderDistance, 50, 200, " studs"));
                opts.push_back(toggleOpt("Surface Detail", gSettings.graphics.surfaceDetail));
                opts.push_back(toggleOpt("HQ Assets", gSettings.graphics.hqAssets));
                opts.push_back(toggleOpt("Bloom", gSettings.graphics.bloom));
                opts.push_back(toggleOpt("Fog", gSettings.graphics.fog));
                opts.push_back(toggleOpt("Shadows", gSettings.graphics.shadows));
                opts.push_back(toggleOpt("Skybox", gSettings.graphics.skybox));
                opts.push_back(dropdownOpt("Particle Quality", gSettings.graphics.particleQuality, particleLabels, 4));
                opts.push_back(toggleOpt("Texture Auto-Scale", gSettings.graphics.textureAutoScale));
                opts.push_back(sliderOpt("Resolution Scale", gSettings.graphics.resolutionScale, 50, 100, "%"));
                opts.push_back(toggleOpt("Anti-Aliasing", gSettings.graphics.antiAlias));
                opts.push_back(sliderOpt("Brightness", gSettings.graphics.brightness, 0, 100));
                opts.push_back(sliderOpt("Gamma", gSettings.graphics.gamma, 0, 100));
                opts.push_back(toggleOpt("FPS Counter", gSettings.graphics.fpsCounter));
                // --- New graphics settings ---
                opts.push_back(sliderOpt("Exposure", gSettings.graphics.exposure, 300, 800));
                opts.push_back(sliderOpt("Color Saturation", gSettings.graphics.colorSaturation, 0, 200));
                opts.push_back(sliderOpt("Temperature", gSettings.graphics.temperature, -50, 50));
                opts.push_back(sliderOpt("Fog Distance", gSettings.graphics.fogDistance, 20, 200, " studs"));
                opts.push_back(dropdownOpt("Terrain Mesh Density", (int)gSettings.graphics.terrainMeshDensity, terrainMeshLabels, 3));
                opts.push_back(dropdownOpt("Shadow Quality", (int)gSettings.graphics.shadowQuality, shadowQualityLabels, 5));
                opts.push_back(toggleOpt("Vignette", gSettings.graphics.vignette));
                opts.push_back(toggleOpt("Film Grain", gSettings.graphics.filmGrain));
                opts.push_back(toggleOpt("Ambient Occlusion", gSettings.graphics.ambientOcclusion));
                opts.push_back(sliderOpt("Bloom Intensity", gSettings.graphics.bloomIntensity, 0, 100));
                opts.push_back(dropdownOpt("Color Correction", (int)gSettings.graphics.colorCorrection, colorCorrectionLabels, 5));
                break;
            case SettingsCategory::Sound:
                opts.push_back(sliderOpt("Master Volume", gSettings.sound.masterVolume, 0, 100, "%"));
                opts.push_back(sliderOpt("Music Volume", gSettings.sound.musicVolume, 0, 100, "%"));
                opts.push_back(sliderOpt("SFX Volume", gSettings.sound.sfxVolume, 0, 100, "%"));
                opts.push_back(toggleOpt("Collision Sounds", gSettings.sound.collisionSounds));
                opts.push_back(toggleOpt("Spatial Audio", gSettings.sound.spatialAudio));
                opts.push_back(dropdownOpt("Soundfont Interpolation", (int)gSettings.sound.fluidSynthInterp, fluidSynthInterpLabels, 4));
                opts.push_back(textInputOpt("Custom SoundFont", gSettings.sound.customSoundFont));
                break;
            case SettingsCategory::Controls:
                opts.push_back(dropdownOpt("Move Mode", (int)gSettings.controls.moveMode, moveLabels, 3));
                opts.push_back(toggleOpt("C-Stick Look", gSettings.controls.cStickLook));
                opts.push_back(toggleOpt("ZL/ZR Zoom", gSettings.controls.zlZoom));
                opts.push_back(sliderOpt("Move Speed", (int)(gSettings.controls.moveSpeed * 100), 10, 200, "%"));
                opts.push_back(sliderOpt("Look Sensitivity", (int)(gSettings.controls.lookSensitivity * 100), 10, 200, "%"));
                break;
            case SettingsCategory::Scripts:
                opts.push_back(toggleOpt("Enable Scripts", gSettings.scripts.enableScripts));
                opts.push_back(toggleOpt("Error Logging", gSettings.scripts.scriptErrorLogging));
                opts.push_back(sliderOpt("Script Timeout", gSettings.scripts.scriptTimeout, 5, 120, "s"));
                break;
            case SettingsCategory::Assets:
                opts.push_back(toggleOpt("Platform Content 2012", true));
                opts.push_back(toggleOpt("Platform Content 2015", true));
                opts.push_back(toggleOpt("Platform Content 2016", true));
                {
                    // "Clear Asset Cache" button (uses a slider 0->1 as a one-shot trigger)
                    SettingsOption clearOpt;
                    clearOpt.label = "Clear Asset Cache";
                    clearOpt.value = 0;
                    clearOpt.minValue = 0;
                    clearOpt.maxValue = 1;
                    clearOpt.unit = "";
                    clearOpt.isToggle = true;
                    clearOpt.isDropdown = false;
                    clearOpt.dropdownLabels = {"Cancel", "Clear"};
                    opts.push_back(clearOpt);
                }
                break;
            case SettingsCategory::Logs:
                opts.push_back(toggleOpt("Enable Logging", Settings::get().debug.enableFileLogging));
                {
                    SettingsOption pathOpt;
                    pathOpt.label = "Log File";
                    pathOpt.value = 0;
                    pathOpt.minValue = 0;
                    pathOpt.maxValue = 0;
                    pathOpt.unit = "";
                    pathOpt.isToggle = false;
                    pathOpt.isDropdown = false;
                    pathOpt.isTextInput = true;
                    const char* logPath = DebugLogger::getLogFilePath();
                    pathOpt.textValue = logPath && logPath[0] ? logPath : "(No active log file)";
                    opts.push_back(pathOpt);
                }
                break;
            case SettingsCategory::Debug:
                opts.push_back(toggleOpt("Debug Mode", gSettings.debug.debugMode));
                opts.push_back(toggleOpt("Enable File Logging", gSettings.debug.enableFileLogging));
                opts.push_back(toggleOpt("No-Clip", gSettings.debug.noclip));
                opts.push_back(toggleOpt("God Mode", gSettings.debug.godMode));
                opts.push_back(toggleOpt("Fly Mode", gSettings.debug.flyMode));
                opts.push_back(toggleOpt("Wireframe", gSettings.debug.showWireframe));
                opts.push_back(toggleOpt("VRAM Budget", gSettings.debug.vramBudget));
                break;
            case SettingsCategory::Presets:
                // Year preset buttons (2012, 2015, 2016, 2019, 2020, 2022)
                {
                    static const char* yearBtnLabels[] = { "2012", "2015", "2016", "2019", "2022" };
                    int currentYear = (int)gSettings.general.contentYear;
                    for (int i = 0; i < 5; i++) {
                        SettingsOption yearBtn;
                        yearBtn.label = yearBtnLabels[i];
                        yearBtn.value = (i == currentYear) ? 1 : 0;
                        yearBtn.minValue = 0;
                        yearBtn.maxValue = 1;
                        yearBtn.unit = "";
                        yearBtn.isToggle = true;
                        yearBtn.isDropdown = false;
                        yearBtn.dropdownLabels = {"Off", "On"};
                        opts.push_back(yearBtn);
                    }
                }
                // Performance presets
                opts.push_back(dropdownOpt("Performance Preset", (int)gSettings.preset, presetLabels, 3));
                break;
            case SettingsCategory::Multiplayer:
                opts.push_back(toggleOpt("Enable Multiplayer", true));
                break;
            case SettingsCategory::About:
                opts.push_back(sliderOpt("Version 0.1.0", 0, 0, 0, ""));
                break;
            default:
                break;
        }
        return opts;
    }

    void setOptionTextValue(SettingsCategory cat, int optionIndex, const std::string& value) {
        if (optionIndex < 0) return;
        switch (cat) {
            case SettingsCategory::General:
                if (optionIndex == 2) {  // Asset Delivery URL
                    gSettings.general.assetdeliveryUrl = value;
                    // Update asset loader base URL
                    AssetLoader::setAssetBaseUrl(value);
                }
                if (optionIndex == 3) {  // Toolbox Proxy
                    gSettings.general.toolboxProxy = value;
                }
                break;
            case SettingsCategory::Sound:
                if (optionIndex == 6) {  // Custom SoundFont path
                    gSettings.sound.customSoundFont = value;
                }
                break;
            default:
                break;
        }
    }

    void setOptionValue(SettingsCategory cat, int optionIndex, int value) {
        // Helper lambda to apply bool
        auto setBool = [&](bool& ref) { ref = (value != 0); };
        auto setInt = [&](int& ref, int min, int max) { ref = std::clamp(value, min, max); };

        switch (cat) {
            case SettingsCategory::General:
                if (optionIndex == 0) setBool(gSettings.general.safechat);
                if (optionIndex == 1) setBool(gSettings.general.offlineMode);
                if (optionIndex == 4) { gSettings.general.physicsEngine = (PhysicsEngine)value; }
                break;
            case SettingsCategory::Graphics:
                // Year preset buttons (indices 0-6: 2007, 2011, 2012, 2015, 2016, 2019, 2022)
                if (optionIndex >= 0 && optionIndex <= 6 && value == 1) {
                    ContentYear year = (ContentYear)optionIndex;
                    applyYearPreset(year);
                    UIAssetLoader::setContentYear(year);
                    UIAssetLoader::reloadAllAssets();
                    save("sdmc:/OpenBLOX/settings.json");
                    break;
                }
                if (optionIndex == 8) { gSettings.graphics.partStyle = (PartStyle)value; break; }
                if (optionIndex == 9) { gSettings.graphics.lodMode = (LODMode)value; break; }
                if (optionIndex == 10) { gSettings.graphics.fixedLODLevel = value + 1; break; }
                if (optionIndex == 11) setInt(gSettings.graphics.renderDistance, 50, 200);
                if (optionIndex == 12) setBool(gSettings.graphics.surfaceDetail);
                if (optionIndex == 13) setBool(gSettings.graphics.hqAssets);
                if (optionIndex == 14) setBool(gSettings.graphics.bloom);
                if (optionIndex == 15) setBool(gSettings.graphics.fog);
                if (optionIndex == 16) setBool(gSettings.graphics.shadows);
                if (optionIndex == 17) setBool(gSettings.graphics.skybox);
                if (optionIndex == 18) setInt(gSettings.graphics.particleQuality, 0, 3);
                if (optionIndex == 19) setBool(gSettings.graphics.textureAutoScale);
                if (optionIndex == 20) setInt(gSettings.graphics.resolutionScale, 50, 100);
                if (optionIndex == 21) setBool(gSettings.graphics.antiAlias);
                if (optionIndex == 22) setInt(gSettings.graphics.brightness, 0, 100);
                if (optionIndex == 23) setInt(gSettings.graphics.gamma, 0, 100);
                if (optionIndex == 24) setBool(gSettings.graphics.fpsCounter);
                // --- New graphics settings ---
                if (optionIndex == 25) setInt(gSettings.graphics.exposure, 300, 800);
                if (optionIndex == 26) setInt(gSettings.graphics.colorSaturation, 0, 200);
                if (optionIndex == 27) setInt(gSettings.graphics.temperature, -50, 50);
                if (optionIndex == 28) setInt(gSettings.graphics.fogDistance, 20, 200);
                if (optionIndex == 29) { gSettings.graphics.terrainMeshDensity = (TerrainMeshDensity)value; break; }
                if (optionIndex == 30) { gSettings.graphics.shadowQuality = (ShadowQuality)value; break; }
                if (optionIndex == 31) setBool(gSettings.graphics.vignette);
                if (optionIndex == 32) setBool(gSettings.graphics.filmGrain);
                if (optionIndex == 33) setBool(gSettings.graphics.ambientOcclusion);
                if (optionIndex == 34) setInt(gSettings.graphics.bloomIntensity, 0, 100);
                if (optionIndex == 35) { gSettings.graphics.colorCorrection = (GraphicsColorCorrection)value; break; }
                break;
            case SettingsCategory::Sound:
                if (optionIndex == 0) setInt(gSettings.sound.masterVolume, 0, 100);
                if (optionIndex == 1) setInt(gSettings.sound.musicVolume, 0, 100);
                if (optionIndex == 2) setInt(gSettings.sound.sfxVolume, 0, 100);
                if (optionIndex == 3) setBool(gSettings.sound.collisionSounds);
                if (optionIndex == 4) setBool(gSettings.sound.spatialAudio);
                if (optionIndex == 5) { gSettings.sound.fluidSynthInterp = (FluidSynthInterpMode)value; break; }
                break;
            case SettingsCategory::Controls:
                if (optionIndex == 0) { gSettings.controls.moveMode = (MoveControl)value; break; }
                if (optionIndex == 1) setBool(gSettings.controls.cStickLook);
                if (optionIndex == 2) setBool(gSettings.controls.zlZoom);
                if (optionIndex == 3) { gSettings.controls.moveSpeed = value / 100.0f; break; }
                if (optionIndex == 4) { gSettings.controls.lookSensitivity = value / 100.0f; break; }
                break;
            case SettingsCategory::Scripts:
                if (optionIndex == 0) setBool(gSettings.scripts.enableScripts);
                if (optionIndex == 1) setBool(gSettings.scripts.scriptErrorLogging);
                if (optionIndex == 2) setInt(gSettings.scripts.scriptTimeout, 5, 120);
                break;
            case SettingsCategory::Assets:
                if (optionIndex == 3 && value == 1) {
                    // "Clear Asset Cache" button pressed
                    AssetLoader::clearAllCache();
                    printf("[Settings] Asset cache cleared\n");
                }
                break;
            case SettingsCategory::Debug:
                if (optionIndex == 0) setBool(gSettings.debug.debugMode);
                if (optionIndex == 1) setBool(gSettings.debug.enableFileLogging);
                if (optionIndex == 2) setBool(gSettings.debug.noclip);
                if (optionIndex == 3) setBool(gSettings.debug.godMode);
                if (optionIndex == 4) setBool(gSettings.debug.flyMode);
                if (optionIndex == 5) setBool(gSettings.debug.showWireframe);
                if (optionIndex == 6) setBool(gSettings.debug.vramBudget);
                break;
            case SettingsCategory::Presets:
                // Year preset buttons (indices 0-6: 2007, 2011, 2012, 2015, 2016, 2019, 2022)
                if (optionIndex >= 0 && optionIndex <= 6 && value == 1) {
                    ContentYear year = (ContentYear)optionIndex;
                    applyYearPreset(year);
                    UIAssetLoader::setContentYear(year);
                    UIAssetLoader::reloadAllAssets();
                    break;
                }
                // Performance preset (index 8)
                if (optionIndex == 8) applyPreset((PerformancePreset)value);
                break;
            default:
                break;
        }
    }

    const char* getCategoryName(SettingsCategory cat) {
        switch (cat) {
            case SettingsCategory::General:     return "General";
            case SettingsCategory::Graphics:    return "Graphics";
            case SettingsCategory::Sound:       return "Sound";
            case SettingsCategory::Controls:    return "Controls";
            case SettingsCategory::Scripts:     return "Scripts";
            case SettingsCategory::Assets:      return "Assets";
            case SettingsCategory::Logs:        return "Logs";
            case SettingsCategory::Debug:       return "Debug";
            case SettingsCategory::Presets:     return "Presets";
            case SettingsCategory::Multiplayer: return "Multiplayer";
            case SettingsCategory::About:       return "About";
            default: return "Unknown";
        }
    }

    bool load(const std::string& path) {
        FILE* f = fopen(path.c_str(), "r");
        if (!f) return false;

        // Read entire file into string
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buf = new char[len + 1];
        fread(buf, 1, len, f);
        buf[len] = 0;
        fclose(f);

        // Detect if JSON or INI format
        bool isJson = (strchr(buf, '{') != nullptr);
        char* p = buf;

        if (isJson) {
            // Simple JSON parser - extract key-value pairs
            // Track current section (general, graphics, sound, etc.)
            std::string section = "";

            while (*p) {
                // Skip whitespace
                while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
                if (!*p) break;

                // Parse section names (top-level keys like "general", "graphics")
                if (*p == '"') {
                    char key[128] = {0};
                    int ki = 0;
                    p++; // skip opening quote
                    while (*p && *p != '"' && ki < 127) {
                        key[ki++] = *p++;
                    }
                    key[ki] = 0;
                    if (*p == '"') p++;

                    // Skip colon
                    while (*p && *p != ':') p++;
                    if (*p == ':') p++;
                    while (*p && (*p == ' ' || *p == '\t')) p++;

                    if (*p == '"') {
                        // String value - check if it's a section name
                        p--; // go back to re-parse as section
                        // Check if next char after string is '{'
                        char* quoteStart = p;
                        p++;
                        while (*p && *p != '"') p++;
                        if (*p == '"') {
                            p++;
                            while (*p && (*p == ' ' || *p == '\t')) p++;
                            if (*p == '{') {
                                // This is a section name
                                section = key;
                                p++; // skip {
                                continue;
                            }
                        }
                        // It's a string value
                        char val[512] = {0};
                        int vi = 0;
                        p = quoteStart + 1; // go back to start of string
                        p++; // skip opening quote
                        while (*p && *p != '"' && vi < 511) {
                            if (*p == '\\' && *(p + 1)) {
                                p++;
                                if (*p == 'n') val[vi++] = '\n';
                                else if (*p == 't') val[vi++] = '\t';
                                else if (*p == '\\') val[vi++] = '\\';
                                else val[vi++] = *p;
                            } else {
                                val[vi++] = *p;
                            }
                            p++;
                        }
                        val[vi] = 0;
                        if (*p == '"') p++;

                        // Apply string value
                        std::string fullKey = section + "." + key;
                        if (fullKey == "sound.customSoundFont") gSettings.sound.customSoundFont = val;
                        else if (fullKey == "general.assetdeliveryUrl") gSettings.general.assetdeliveryUrl = val;
                        else if (fullKey == "general.toolboxProxy") gSettings.general.toolboxProxy = val;
                        continue;
                    } else if (*p == '{') {
                        // Section name - this should have been caught above
                    } else {
                        // Number or bool
                        char val[64] = {0};
                        int vi = 0;
                        while (*p && (*p == '-' || (*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E' || *p == '+')) {
                            val[vi++] = *p++;
                        }
                        val[vi] = 0;

                        // Check if it's a bool
                        bool isBool = (strcmp(key, "safechat") == 0 ||
                                      strcmp(key, "offlineMode") == 0 ||
                                      strcmp(key, "surfaceDetail") == 0 ||
                                      strcmp(key, "hqAssets") == 0 ||
                                      strcmp(key, "bloom") == 0 ||
                                      strcmp(key, "fog") == 0 ||
                                      strcmp(key, "shadows") == 0 ||
                                      strcmp(key, "skybox") == 0 ||
                                      strcmp(key, "textureAutoScale") == 0 ||
                                      strcmp(key, "antiAlias") == 0 ||
                                      strcmp(key, "fpsCounter") == 0 ||
                                      strcmp(key, "vignette") == 0 ||
                                      strcmp(key, "filmGrain") == 0 ||
                                      strcmp(key, "ambientOcclusion") == 0 ||
                                      strcmp(key, "collisionSounds") == 0 ||
                                      strcmp(key, "spatialAudio") == 0 ||
                                      strcmp(key, "cStickLook") == 0 ||
                                      strcmp(key, "zlZoom") == 0 ||
                                      strcmp(key, "enableScripts") == 0 ||
                                      strcmp(key, "scriptErrorLogging") == 0 ||
                                      strcmp(key, "debugMode") == 0 ||
                                      strcmp(key, "enableFileLogging") == 0 ||
                                      strcmp(key, "noclip") == 0 ||
                                      strcmp(key, "godMode") == 0 ||
                                      strcmp(key, "flyMode") == 0 ||
                                      strcmp(key, "showWireframe") == 0 ||
                                      strcmp(key, "vramBudget") == 0 ||
                                      strcmp(key, "surfaceDetail") == 0);

                        if (isBool) {
                            bool bv = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
                            std::string fullKey = section + "." + key;
                            if (fullKey == "general.safechat") gSettings.general.safechat = bv;
                            else if (fullKey == "general.offlineMode") gSettings.general.offlineMode = bv;
                            else if (fullKey == "graphics.surfaceDetail") gSettings.graphics.surfaceDetail = bv;
                            else if (fullKey == "graphics.hqAssets") gSettings.graphics.hqAssets = bv;
                            else if (fullKey == "graphics.bloom") gSettings.graphics.bloom = bv;
                            else if (fullKey == "graphics.fog") gSettings.graphics.fog = bv;
                            else if (fullKey == "graphics.shadows") gSettings.graphics.shadows = bv;
                            else if (fullKey == "graphics.skybox") gSettings.graphics.skybox = bv;
                            else if (fullKey == "graphics.textureAutoScale") gSettings.graphics.textureAutoScale = bv;
                            else if (fullKey == "graphics.antiAlias") gSettings.graphics.antiAlias = bv;
                            else if (fullKey == "graphics.fpsCounter") gSettings.graphics.fpsCounter = bv;
                            else if (fullKey == "graphics.vignette") gSettings.graphics.vignette = bv;
                            else if (fullKey == "graphics.filmGrain") gSettings.graphics.filmGrain = bv;
                            else if (fullKey == "graphics.ambientOcclusion") gSettings.graphics.ambientOcclusion = bv;
                            else if (fullKey == "sound.collisionSounds") gSettings.sound.collisionSounds = bv;
                            else if (fullKey == "sound.spatialAudio") gSettings.sound.spatialAudio = bv;
                            else if (fullKey == "controls.cStickLook") gSettings.controls.cStickLook = bv;
                            else if (fullKey == "controls.zlZoom") gSettings.controls.zlZoom = bv;
                            else if (fullKey == "scripts.enableScripts") gSettings.scripts.enableScripts = bv;
                            else if (fullKey == "scripts.scriptErrorLogging") gSettings.scripts.scriptErrorLogging = bv;
                            else if (fullKey == "debug.debugMode") gSettings.debug.debugMode = bv;
                            else if (fullKey == "debug.enableFileLogging") gSettings.debug.enableFileLogging = bv;
                            else if (fullKey == "debug.noclip") gSettings.debug.noclip = bv;
                            else if (fullKey == "debug.godMode") gSettings.debug.godMode = bv;
                            else if (fullKey == "debug.flyMode") gSettings.debug.flyMode = bv;
                            else if (fullKey == "debug.showWireframe") gSettings.debug.showWireframe = bv;
                            else if (fullKey == "debug.vramBudget") gSettings.debug.vramBudget = bv;
                        } else {
                            // Integer/float
                            int ival = atoi(val);
                            float fval = atof(val);
                            std::string fullKey = section + "." + key;
                            if (fullKey == "graphics.era") gSettings.graphics.era = (GraphicsEra)ival;
                            else if (fullKey == "graphics.partStyle") gSettings.graphics.partStyle = (PartStyle)ival;
                            else if (fullKey == "graphics.lodMode") gSettings.graphics.lodMode = (LODMode)ival;
                            else if (fullKey == "graphics.fixedLODLevel") gSettings.graphics.fixedLODLevel = std::clamp(ival, 1, 4);
                            else if (fullKey == "graphics.renderDistance") gSettings.graphics.renderDistance = ival;
                            else if (fullKey == "graphics.particleQuality") gSettings.graphics.particleQuality = ival;
                            else if (fullKey == "graphics.resolutionScale") gSettings.graphics.resolutionScale = ival;
                            else if (fullKey == "graphics.brightness") gSettings.graphics.brightness = ival;
                            else if (fullKey == "graphics.gamma") gSettings.graphics.gamma = ival;
                            else if (fullKey == "graphics.exposure") gSettings.graphics.exposure = ival;
                            else if (fullKey == "graphics.colorSaturation") gSettings.graphics.colorSaturation = ival;
                            else if (fullKey == "graphics.temperature") gSettings.graphics.temperature = ival;
                            else if (fullKey == "graphics.fogDistance") gSettings.graphics.fogDistance = ival;
                            else if (fullKey == "graphics.terrainMeshDensity") gSettings.graphics.terrainMeshDensity = (TerrainMeshDensity)ival;
                            else if (fullKey == "graphics.shadowQuality") gSettings.graphics.shadowQuality = (ShadowQuality)ival;
                            else if (fullKey == "graphics.bloomIntensity") gSettings.graphics.bloomIntensity = ival;
                            else if (fullKey == "graphics.colorCorrection") gSettings.graphics.colorCorrection = (GraphicsColorCorrection)ival;
                            else if (fullKey == "sound.masterVolume") gSettings.sound.masterVolume = ival;
                            else if (fullKey == "sound.musicVolume") gSettings.sound.musicVolume = ival;
                            else if (fullKey == "sound.sfxVolume") gSettings.sound.sfxVolume = ival;
                            else if (fullKey == "sound.fluidSynthInterp") gSettings.sound.fluidSynthInterp = (FluidSynthInterpMode)ival;
                            else if (fullKey == "controls.moveMode") gSettings.controls.moveMode = (MoveControl)ival;
                            else if (fullKey == "controls.moveSpeed") gSettings.controls.moveSpeed = fval;
                            else if (fullKey == "controls.lookSensitivity") gSettings.controls.lookSensitivity = fval;
                            else if (fullKey == "general.contentYear") { int yr=ival; if(yr<0||yr>6) yr=1; gSettings.general.contentYear=(ContentYear)yr; }
                            else if (fullKey == "general.physicsEngine") gSettings.general.physicsEngine = (PhysicsEngine)ival;
                            else if (fullKey == "scripts.scriptTimeout") gSettings.scripts.scriptTimeout = ival;
                            else if (!section.compare("preset")) gSettings.preset = (PerformancePreset)ival;
                        }
                    }
                    continue;
                } else if (*p == '}') {
                    // End of section
                    section = "";
                    p++;
                    continue;
                } else if (*p == ',') {
                    // Skip comma
                    p++;
                    continue;
                } else {
                    p++;
                    continue;
                }
            }
        } else {
            // INI-style parser (backward compatibility)
            // Reset buffer pointer
            p = buf;
            char line[256];
            int li = 0;

            while (*p) {
                memset(line, 0, sizeof(line));
                li = 0;
                // Read until newline
                while (*p && *p != '\n' && *p != '\r' && li < 255) {
                    line[li++] = *p++;
                }
                line[li] = 0;
                while (*p && (*p == '\n' || *p == '\r')) p++;

                // Skip comments
                if (line[0] == '#' || line[0] == ';' || line[0] == '\0') continue;

                // Parse key=value
                char* eq = strchr(line, '=');
                if (!eq) continue;
                *eq = 0;
                char* key = line;
                char* val = eq + 1;
                while (*key == ' ') key++;
                while (*val == ' ') val++;

                // Apply (same as before)
                if (strcmp(key, "graphics.era") == 0) gSettings.graphics.era = (GraphicsEra)atoi(val);
                else if (strcmp(key, "graphics.partStyle") == 0) gSettings.graphics.partStyle = (PartStyle)atoi(val);
                else if (strcmp(key, "graphics.lodMode") == 0) gSettings.graphics.lodMode = (LODMode)atoi(val);
                else if (strcmp(key, "graphics.fixedLODLevel") == 0) gSettings.graphics.fixedLODLevel = std::clamp(atoi(val), 1, 4);
                else if (strcmp(key, "graphics.renderDistance") == 0) gSettings.graphics.renderDistance = atoi(val);
                else if (strcmp(key, "graphics.surfaceDetail") == 0) gSettings.graphics.surfaceDetail = (atoi(val) != 0);
                else if (strcmp(key, "graphics.hqAssets") == 0) gSettings.graphics.hqAssets = (atoi(val) != 0);
                else if (strcmp(key, "graphics.bloom") == 0) gSettings.graphics.bloom = (atoi(val) != 0);
                else if (strcmp(key, "graphics.fog") == 0) gSettings.graphics.fog = (atoi(val) != 0);
                else if (strcmp(key, "graphics.shadows") == 0) gSettings.graphics.shadows = (atoi(val) != 0);
                else if (strcmp(key, "graphics.skybox") == 0) gSettings.graphics.skybox = (atoi(val) != 0);
                else if (strcmp(key, "graphics.particleQuality") == 0) gSettings.graphics.particleQuality = atoi(val);
                else if (strcmp(key, "graphics.textureAutoScale") == 0) gSettings.graphics.textureAutoScale = (atoi(val) != 0);
                else if (strcmp(key, "graphics.resolutionScale") == 0) gSettings.graphics.resolutionScale = atoi(val);
                else if (strcmp(key, "graphics.antiAlias") == 0) gSettings.graphics.antiAlias = (atoi(val) != 0);
                else if (strcmp(key, "graphics.brightness") == 0) gSettings.graphics.brightness = atoi(val);
                else if (strcmp(key, "graphics.gamma") == 0) gSettings.graphics.gamma = atoi(val);
                else if (strcmp(key, "graphics.fpsCounter") == 0) gSettings.graphics.fpsCounter = (atoi(val) != 0);
                else if (strcmp(key, "graphics.exposure") == 0) gSettings.graphics.exposure = atoi(val);
                else if (strcmp(key, "graphics.colorSaturation") == 0) gSettings.graphics.colorSaturation = atoi(val);
                else if (strcmp(key, "graphics.temperature") == 0) gSettings.graphics.temperature = atoi(val);
                else if (strcmp(key, "graphics.fogDistance") == 0) gSettings.graphics.fogDistance = atoi(val);
                else if (strcmp(key, "graphics.terrainMeshDensity") == 0) gSettings.graphics.terrainMeshDensity = (TerrainMeshDensity)atoi(val);
                else if (strcmp(key, "graphics.shadowQuality") == 0) gSettings.graphics.shadowQuality = (ShadowQuality)atoi(val);
                else if (strcmp(key, "graphics.vignette") == 0) gSettings.graphics.vignette = (atoi(val) != 0);
                else if (strcmp(key, "graphics.filmGrain") == 0) gSettings.graphics.filmGrain = (atoi(val) != 0);
                else if (strcmp(key, "graphics.ambientOcclusion") == 0) gSettings.graphics.ambientOcclusion = (atoi(val) != 0);
                else if (strcmp(key, "graphics.bloomIntensity") == 0) gSettings.graphics.bloomIntensity = atoi(val);
                else if (strcmp(key, "graphics.colorCorrection") == 0) gSettings.graphics.colorCorrection = (GraphicsColorCorrection)atoi(val);
                else if (strcmp(key, "sound.masterVolume") == 0) gSettings.sound.masterVolume = atoi(val);
                else if (strcmp(key, "sound.musicVolume") == 0) gSettings.sound.musicVolume = atoi(val);
                else if (strcmp(key, "sound.sfxVolume") == 0) gSettings.sound.sfxVolume = atoi(val);
                else if (strcmp(key, "sound.collisionSounds") == 0) gSettings.sound.collisionSounds = (atoi(val) != 0);
                else if (strcmp(key, "sound.spatialAudio") == 0) gSettings.sound.spatialAudio = (atoi(val) != 0);
                else if (strcmp(key, "sound.fluidSynthInterp") == 0) gSettings.sound.fluidSynthInterp = (FluidSynthInterpMode)atoi(val);
                else if (strcmp(key, "sound.customSoundFont") == 0) gSettings.sound.customSoundFont = val;
                else if (strcmp(key, "controls.moveMode") == 0) gSettings.controls.moveMode = (MoveControl)atoi(val);
                else if (strcmp(key, "controls.cStickLook") == 0) gSettings.controls.cStickLook = (atoi(val) != 0);
                else if (strcmp(key, "controls.zlZoom") == 0) gSettings.controls.zlZoom = (atoi(val) != 0);
                else if (strcmp(key, "controls.moveSpeed") == 0) gSettings.controls.moveSpeed = atof(val);
                else if (strcmp(key, "controls.lookSensitivity") == 0) gSettings.controls.lookSensitivity = atof(val);
                else if (strcmp(key, "general.safechat") == 0) gSettings.general.safechat = (atoi(val) != 0);
                else if (strcmp(key, "general.offlineMode") == 0) gSettings.general.offlineMode = (atoi(val) != 0);
                else if (strcmp(key, "general.contentYear") == 0) { int yr=atoi(val); if(yr<0||yr>6) yr=1; gSettings.general.contentYear=(ContentYear)yr; }
                else if (strcmp(key, "general.physicsEngine") == 0) gSettings.general.physicsEngine = (PhysicsEngine)atoi(val);
                else if (strcmp(key, "general.assetdeliveryUrl") == 0) gSettings.general.assetdeliveryUrl = val;
                else if (strcmp(key, "general.toolboxProxy") == 0) gSettings.general.toolboxProxy = val;
                else if (strcmp(key, "scripts.enableScripts") == 0) gSettings.scripts.enableScripts = (atoi(val) != 0);
                else if (strcmp(key, "scripts.scriptErrorLogging") == 0) gSettings.scripts.scriptErrorLogging = (atoi(val) != 0);
                else if (strcmp(key, "scripts.scriptTimeout") == 0) gSettings.scripts.scriptTimeout = atoi(val);
                else if (strcmp(key, "debug.debugMode") == 0) gSettings.debug.debugMode = (atoi(val) != 0);
                else if (strcmp(key, "debug.enableFileLogging") == 0) gSettings.debug.enableFileLogging = (atoi(val) != 0);
                else if (strcmp(key, "debug.noclip") == 0) gSettings.debug.noclip = (atoi(val) != 0);
                else if (strcmp(key, "debug.godMode") == 0) gSettings.debug.godMode = (atoi(val) != 0);
                else if (strcmp(key, "debug.flyMode") == 0) gSettings.debug.flyMode = (atoi(val) != 0);
                else if (strcmp(key, "preset") == 0) gSettings.preset = (PerformancePreset)atoi(val);
            }
        }

        delete[] buf;
        printf("Settings loaded from %s\n", path.c_str());

        // Apply loaded asset delivery URL to asset loader
        AssetLoader::setAssetBaseUrl(gSettings.general.assetdeliveryUrl);
        return true;
    }

    bool save(const std::string& path) {
        FILE* f = fopen(path.c_str(), "w");
        if (!f) return false;

        // JSON output
        fprintf(f, "{\n");

        // General section
        fprintf(f, "  \"general\": {\n");
        fprintf(f, "    \"safechat\": %s,\n", gSettings.general.safechat ? "true" : "false");
        fprintf(f, "    \"offlineMode\": %s,\n", gSettings.general.offlineMode ? "true" : "false");
        fprintf(f, "    \"contentYear\": %d,\n", (int)gSettings.general.contentYear);
        fprintf(f, "    \"physicsEngine\": %d,\n", (int)gSettings.general.physicsEngine);
        fprintf(f, "    \"assetdeliveryUrl\": \"%s\",\n", gSettings.general.assetdeliveryUrl.c_str());
        fprintf(f, "    \"toolboxProxy\": \"%s\"\n", gSettings.general.toolboxProxy.c_str());
        fprintf(f, "  },\n");

        // Graphics section
        fprintf(f, "  \"graphics\": {\n");
        fprintf(f, "    \"era\": %d,\n", (int)gSettings.graphics.era);
        fprintf(f, "    \"partStyle\": %d,\n", (int)gSettings.graphics.partStyle);
        fprintf(f, "    \"lodMode\": %d,\n", (int)gSettings.graphics.lodMode);
        fprintf(f, "    \"fixedLODLevel\": %d,\n", gSettings.graphics.fixedLODLevel);
        fprintf(f, "    \"renderDistance\": %d,\n", gSettings.graphics.renderDistance);
        fprintf(f, "    \"surfaceDetail\": %s,\n", gSettings.graphics.surfaceDetail ? "true" : "false");
        fprintf(f, "    \"hqAssets\": %s,\n", gSettings.graphics.hqAssets ? "true" : "false");
        fprintf(f, "    \"bloom\": %s,\n", gSettings.graphics.bloom ? "true" : "false");
        fprintf(f, "    \"fog\": %s,\n", gSettings.graphics.fog ? "true" : "false");
        fprintf(f, "    \"shadows\": %s,\n", gSettings.graphics.shadows ? "true" : "false");
        fprintf(f, "    \"skybox\": %s,\n", gSettings.graphics.skybox ? "true" : "false");
        fprintf(f, "    \"particleQuality\": %d,\n", gSettings.graphics.particleQuality);
        fprintf(f, "    \"textureAutoScale\": %s,\n", gSettings.graphics.textureAutoScale ? "true" : "false");
        fprintf(f, "    \"resolutionScale\": %d,\n", gSettings.graphics.resolutionScale);
        fprintf(f, "    \"antiAlias\": %s,\n", gSettings.graphics.antiAlias ? "true" : "false");
        fprintf(f, "    \"brightness\": %d,\n", gSettings.graphics.brightness);
        fprintf(f, "    \"gamma\": %d,\n", gSettings.graphics.gamma);
        fprintf(f, "    \"fpsCounter\": %s,\n", gSettings.graphics.fpsCounter ? "true" : "false");
        fprintf(f, "    \"exposure\": %d,\n", gSettings.graphics.exposure);
        fprintf(f, "    \"colorSaturation\": %d,\n", gSettings.graphics.colorSaturation);
        fprintf(f, "    \"temperature\": %d,\n", gSettings.graphics.temperature);
        fprintf(f, "    \"fogDistance\": %d,\n", gSettings.graphics.fogDistance);
        fprintf(f, "    \"terrainMeshDensity\": %d,\n", (int)gSettings.graphics.terrainMeshDensity);
        fprintf(f, "    \"shadowQuality\": %d,\n", (int)gSettings.graphics.shadowQuality);
        fprintf(f, "    \"vignette\": %s,\n", gSettings.graphics.vignette ? "true" : "false");
        fprintf(f, "    \"filmGrain\": %s,\n", gSettings.graphics.filmGrain ? "true" : "false");
        fprintf(f, "    \"ambientOcclusion\": %s,\n", gSettings.graphics.ambientOcclusion ? "true" : "false");
        fprintf(f, "    \"bloomIntensity\": %d,\n", gSettings.graphics.bloomIntensity);
        fprintf(f, "    \"colorCorrection\": %d\n", (int)gSettings.graphics.colorCorrection);
        fprintf(f, "  },\n");

        // Sound section
        fprintf(f, "  \"sound\": {\n");
        fprintf(f, "    \"masterVolume\": %d,\n", gSettings.sound.masterVolume);
        fprintf(f, "    \"musicVolume\": %d,\n", gSettings.sound.musicVolume);
        fprintf(f, "    \"sfxVolume\": %d,\n", gSettings.sound.sfxVolume);
        fprintf(f, "    \"collisionSounds\": %s,\n", gSettings.sound.collisionSounds ? "true" : "false");
        fprintf(f, "    \"spatialAudio\": %s,\n", gSettings.sound.spatialAudio ? "true" : "false");
        fprintf(f, "    \"fluidSynthInterp\": %d,\n", (int)gSettings.sound.fluidSynthInterp);
        fprintf(f, "    \"customSoundFont\": \"%s\"\n", gSettings.sound.customSoundFont.c_str());
        fprintf(f, "  },\n");

        // Controls section
        fprintf(f, "  \"controls\": {\n");
        fprintf(f, "    \"moveMode\": %d,\n", (int)gSettings.controls.moveMode);
        fprintf(f, "    \"cStickLook\": %s,\n", gSettings.controls.cStickLook ? "true" : "false");
        fprintf(f, "    \"zlZoom\": %s,\n", gSettings.controls.zlZoom ? "true" : "false");
        fprintf(f, "    \"moveSpeed\": %.2f,\n", gSettings.controls.moveSpeed);
        fprintf(f, "    \"lookSensitivity\": %.2f\n", gSettings.controls.lookSensitivity);
        fprintf(f, "  },\n");

        // Scripts section
        fprintf(f, "  \"scripts\": {\n");
        fprintf(f, "    \"enableScripts\": %s,\n", gSettings.scripts.enableScripts ? "true" : "false");
        fprintf(f, "    \"scriptErrorLogging\": %s,\n", gSettings.scripts.scriptErrorLogging ? "true" : "false");
        fprintf(f, "    \"scriptTimeout\": %d\n", gSettings.scripts.scriptTimeout);
        fprintf(f, "  },\n");

        // Debug section
        fprintf(f, "  \"debug\": {\n");
        fprintf(f, "    \"debugMode\": %s,\n", gSettings.debug.debugMode ? "true" : "false");
        fprintf(f, "    \"enableFileLogging\": %s,\n", gSettings.debug.enableFileLogging ? "true" : "false");
        fprintf(f, "    \"noclip\": %s,\n", gSettings.debug.noclip ? "true" : "false");
        fprintf(f, "    \"godMode\": %s,\n", gSettings.debug.godMode ? "true" : "false");
        fprintf(f, "    \"flyMode\": %s,\n", gSettings.debug.flyMode ? "true" : "false");
        fprintf(f, "    \"showWireframe\": %s,\n", gSettings.debug.showWireframe ? "true" : "false");
        fprintf(f, "    \"vramBudget\": %s\n", gSettings.debug.vramBudget ? "true" : "false");
        fprintf(f, "  },\n");

        // Preset (top-level)
        fprintf(f, "  \"preset\": %d\n", (int)gSettings.preset);
        fprintf(f, "}\n");

        fclose(f);
        printf("Settings saved to %s\n", path.c_str());
        return true;
    }

    void shutdown() {
    }

}