#pragma once

#include <string>
#include <vector>
#include <map>

// Studio Objects - Roblox Instance Types and Defaults for OpenBLOX Studio
// Manages all basic and advanced Roblox objects for the Studio client

namespace StudioObjects {

    // Categories for the "Add" menu
    enum class ObjectCategory {
        Model,       // BasePart, MeshPart, Model, Folder, Attachment
        UI,          // ScreenGui, SurfaceGui, Frame, TextLabel, etc.
        Values,      // StringValue, IntValue, BoolValue, etc.
        Constraints  // Weld, Motor6D, HingeConstraint, etc.
    };

    struct ObjectTemplate {
        std::string className;
        std::string displayName;
        char icon; // Explorer icon character
        ObjectCategory category;

        // Default properties (class name, position, size, color)
        float defaultPosition[3];
        float defaultSize[3];
        float defaultColor[3];

        ObjectTemplate() :
            icon('?'), category(ObjectCategory::Model),
            defaultPosition{0, 0, 0},
            defaultSize{4, 4, 4},
            defaultColor{1.0f, 0.5f, 0.5f} {}
    };

    // Get the template for a given class name
    const ObjectTemplate* getTemplate(const std::string& className);

    // Get all available object templates
    const std::vector<ObjectTemplate>& getAllTemplates();

    // Get templates for a specific category
    std::vector<const ObjectTemplate*> getTemplatesByCategory(ObjectCategory category);

    // Initialize the object registry
    void init();

    // Shutdown
    void shutdown();
}