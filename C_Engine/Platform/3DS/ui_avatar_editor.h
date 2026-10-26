#ifndef UI_AVATAR_EDITOR_H
#define UI_AVATAR_EDITOR_H

#include "character_editor.h"

// Avatar Editor UI - provides visual interface for character customization
namespace UIAvatarEditor {

    enum class AvatarTab {
        BodyColors,
        Clothing,
        Face,
        Hat,
        CharacterType
    };

    // Initialize the avatar editor UI
    void init();

    // Run the avatar editor UI loop (blocks until user exits)
    void run();

    // Shutdown the avatar editor UI
    void shutdown();

    // Draw the avatar preview on top screen
    void drawAvatarPreview();

    // Draw the avatar editor UI on bottom screen
    void drawAvatarUI();

    // Process avatar editor input
    void processAvatarInput();

    // Save and return to main menu
    void saveAndExit();
}

#endif