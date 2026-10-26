#pragma once

#include <string>
#include <vector>
#include <map>
#include <3ds.h>

// OpenBLOX Settings Framework
// Categories: General, Graphics, Sound, Scripts, Assets, Logs, Debug, Presets, Multiplayer, About

namespace Settings {

    // === Graphics Settings ===
    enum class GraphicsEra {
        Classic2007,
        Year2011,
        Year2012,
        Year2015,
        Modern
    };

    enum class PartStyle {
        Beveled,       // 2008-2013
        Outlined,      // 2013-2019
        Smooth         // 2019-present (Neither)
    };

    enum class LODMode {
        DistanceBased,
        FixedLevel     // Level 1-4
    };

    enum class TerrainMeshDensity {
        Low,
        Medium,
        High
    };

    enum class ShadowQuality {
        Off,
        Crisp,       // Hard edges (2007-2012)
        Normal,      // Slight blur (2015+)
        Soft,        // Heavy blur
        HighDetail   // Max quality
    };

    enum class GraphicsColorCorrection {
        None,
        Warm,
        Cool,
        Vibrant,
        Desaturated
    };

    struct GraphicsSettings {
        GraphicsEra era = GraphicsEra::Classic2007;
        PartStyle partStyle = PartStyle::Beveled;
        LODMode lodMode = LODMode::DistanceBased;
        int fixedLODLevel = 2;       // 1-4 (only used if lodMode == FixedLevel)
        int renderDistance = 100;    // studs (50-200)
        bool surfaceDetail = true;
        bool hqAssets = false;
        bool bloom = false;
        bool fog = true;
        bool shadows = false;
        bool skybox = true;
        int particleQuality = 2;   // 0=off, 1=low, 2=medium, 3=high
        bool textureAutoScale = true;
        int resolutionScale = 100; // Percentage (50-100)
        bool antiAlias = false;
        int brightness = 50;       // 0-100
        int gamma = 50;            // 0-100
        bool fpsCounter = false;
        int exposure = 500;             // 300-800, brightness multiplier
        int colorSaturation = 100;      // 0-200, desaturated to oversaturated
        int temperature = 0;           // -50 to 50, cool (blue) to warm (amber)
        int fogDistance = 100;          // 20-200 studs, where fog starts
        TerrainMeshDensity terrainMeshDensity = TerrainMeshDensity::Medium;
        ShadowQuality shadowQuality = ShadowQuality::Normal;
        bool vignette = false;          // Darkens screen corners
        bool filmGrain = false;         // Subtle noise for film look
        bool ambientOcclusion = false;  // Simulates contact shadows (SSAO-like)
        int bloomIntensity = 50;        // 0-100, bloom strength
        GraphicsColorCorrection colorCorrection = GraphicsColorCorrection::None;
    };

    // === Sound Settings ===
    enum class FluidSynthInterpMode {
        None = 0,       // No interpolation - raw samples
        Nearest,        // Nearest neighbor
        Linear,         // Linear interpolation
        FourthOrder     // 4th order - high quality
    };

    struct SoundSettings {
        int masterVolume = 80;     // 0-100
        int musicVolume = 80;      // 0-100
        int sfxVolume = 80;        // 0-100
        bool collisionSounds = true;
        bool spatialAudio = true;
        FluidSynthInterpMode fluidSynthInterp = FluidSynthInterpMode::Linear;
        std::string customSoundFont = "";
    };

    // === Control Settings ===
    enum class MoveControl {
        Joystick,
        DPad,
        Circlepad
    };

    struct ControlSettings {
        MoveControl moveMode = MoveControl::Joystick;
        bool cStickLook = true;       // New 3DS
        bool zlZoom = true;           // New 3DS
        float moveSpeed = 1.0f;
        float lookSensitivity = 1.0f;
    };

    // === General Settings ===
    enum class PhysicsEngine {
        TPE,      // TinyPhysicsEngine (default)
        PGS,      // Projected Gauss-Seidel solver
        Spring    // Spring-based physics
    };

    enum class ContentYear {
        Year2007,
        Year2011,
        Year2012,
        Year2015,
        Year2016,
        Year2019,
        Year2022
    };

    struct GeneralSettings {
        bool safechat = true;
        std::string playerName = "Player";
        int playerId = 0;
        bool offlineMode = false;
        std::string assetdeliveryUrl = "https://assetdelivery.roblox.com";
        std::string toolboxProxy = "";
        ContentYear contentYear = ContentYear::Year2012;
        PhysicsEngine physicsEngine = PhysicsEngine::TPE;
    };

    // === Script Settings ===
    struct ScriptSettings {
        bool enableScripts = true;
        bool scriptErrorLogging = true;
        int scriptTimeout = 30;    // seconds
    };

    // === Debug Settings ===
    struct DebugSettings {
        bool debugMode = false;
        bool enableFileLogging = false;  // Enable debug file logging
        bool noclip = false;
        bool godMode = false;
        bool flyMode = false;
        bool showWireframe = false;
        bool vramBudget = false;
    };

    // === Preset ===
    enum class PerformancePreset {
        Performance,
        Balanced,
        Quality
    };

    // === All Settings ===
    struct AllSettings {
        GeneralSettings general;
        GraphicsSettings graphics;
        SoundSettings sound;
        ControlSettings controls;
        ScriptSettings scripts;
        DebugSettings debug;
        PerformancePreset preset = PerformancePreset::Balanced;
    };

    // Settings categories for menu
    enum class SettingsCategory {
        General,
        Graphics,
        Sound,
        Controls,
        Scripts,
        Assets,
        Logs,
        Debug,
        Presets,
        Multiplayer,
        About,
        Count
    };

    // A single settings option
    struct SettingsOption {
        std::string label;
        int value;           // Current integer value
        int minValue;        // Min value for slider
        int maxValue;        // Max value for slider
        std::string unit;    // Display unit (e.g., "studs", "%")
        bool isToggle;       // If true, value is 0/1 (off/on)
        bool isDropdown;     // If true, value is index into labels
        bool isTextInput;    // If true, value is a string (use textValue)
        std::string textValue;  // Text value for input fields
        std::vector<std::string> dropdownLabels;
    };

    // === Core API ===
    void init();

    // Get singleton settings reference
    AllSettings& get();

    // Load settings from file
    bool load(const std::string& path);

    // Save settings to file
    bool save(const std::string& path = "sdmc:/OpenBLOX/settings.json");

    // Apply preset
    void applyPreset(PerformancePreset preset);

    // Apply year preset (2012-2022)
    void applyYearPreset(ContentYear year);

    // Get options for a category (returns array of settings options)
    std::vector<SettingsOption> getCategoryOptions(SettingsCategory cat);

    // Apply a changed option
    void setOptionValue(SettingsCategory cat, int optionIndex, int value);

    // Apply a changed text option
    void setOptionTextValue(SettingsCategory cat, int optionIndex, const std::string& value);

    // Get category name for display
    const char* getCategoryName(SettingsCategory cat);

    // Shutdown
    void shutdown();
}