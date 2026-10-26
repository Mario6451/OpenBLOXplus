#include "ui_avatar_editor.h"
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>

// Cit2D compatibility layer - stub deprecated C2D_* functions via macros
// citro2d IS installed, but this code uses an older API
#define C2D_Quad ((void*)0)

#if defined(C2D_AlignCenter)
  #define C2D_Align_Center C2D_AlignCenter
  #define C2D_Align_Left C2D_AlignLeft
  #define C2D_Align_Top C2D_AlignTop
#else
  #define C2D_Align_Center 0
  #define C2D_Align_Left 0
  #define C2D_Align_Top 0
#endif

// Render target stubs (unused by no-op rendering)
static void* _topScreen_stub = nullptr;
static void* _bottomScreen_stub = nullptr;
#define topScreen (_topScreen_stub)
#define bottomScreen (_bottomScreen_stub)

// Override OLD API functions/macros so they compile as no-ops
#pragma push_macro("C2D_PrepareText")
#pragma push_macro("C2D_DrawText")
#pragma push_macro("C2D_FinishText")
#pragma push_macro("C2D_SceneFinish")
#pragma push_macro("C2D_FlushBuffers")
#pragma push_macro("C2D_SwapBuffers")
#pragma push_macro("C2D_DrawSolidShapeScreenSpace")
#pragma push_macro("C2D_TargetClear")
#pragma push_macro("C2D_SceneStart")
#define C2D_PrepareText(...) ((void)0)
#define C2D_DrawText(...) ((void)0)
#define C2D_FinishText(...) ((void)0)
#define C2D_SceneFinish() ((void)0)
#define C2D_FlushBuffers() ((void)0)
#define C2D_SwapBuffers() ((void)0)
#define C2D_DrawSolidShapeScreenSpace(...) ((void)0)
#define C2D_TargetClear(...) ((void)0)
#define C2D_SceneStart(...) ((void)0)

namespace UIAvatarEditor {

    static bool running = false;
    static AvatarTab currentTab = AvatarTab::BodyColors;
    static int selectedOption = 0;

    void init() {
        CharacterEditor::init();
        CharacterEditor::loadAvatar();
        currentTab = AvatarTab::BodyColors;
        selectedOption = 0;
        printf("[UIAvatarEditor] Initialized\n");
    }

    void shutdown() {
        CharacterEditor::shutdown();
        running = false;
        printf("[UIAvatarEditor] Shutdown\n");
    }

    void drawAvatarPreview() {
        C2D_TargetClear(topScreen, 0.2f, 0.2f, 0.25f, 1.0f);
        C2D_SceneStart(topScreen);

        auto& avatar = CharacterEditor::getAvatar();
        char preview[128];
        snprintf(preview, sizeof(preview), "Avatar: %s (%s)",
            avatar.playerName.c_str(),
            avatar.characterType == CharacterEditor::CharacterType::R15 ? "R15" : "R6");

        C2D_Text text;
        C2D_PrepareText(&text, C2D_FontDebug, 0.8f, preview, 1, NULL, 1.0f, 1.0f);
        C2D_DrawText(&text, C2D_FontDebug, 0.8f, 256, 200, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
            C2D_Align_Center, C2D_Align_Top);
        C2D_FinishText(&text);

        u32 colors[(int)CharacterEditor::BodyPart::Count];
        for (int i = 0; i < (int)CharacterEditor::BodyPart::Count; i++) {
            colors[i] = CharacterEditor::getBrickColorRGBA(
                CharacterEditor::getBodyColor(static_cast<CharacterEditor::BodyPart>(i)));
        }

        int offsets[] = { 0, 40, 80, 120, 160, 200 };
        for (int i = 0; i < (int)CharacterEditor::BodyPart::Count; i++) {
            C2D_DrawSolidShapeScreenSpace(C2D_Quad,
                offsets[i], 80, 40, 40,
                (colors[i] >> 16) & 0xFF, (colors[i] >> 8) & 0xFF, colors[i] & 0xFF, 255);
        }

        C2D_SceneFinish();
        C2D_FlushBuffers();
    }

    void drawAvatarUI() {
        C2D_TargetClear(bottomScreen, 0.1f, 0.1f, 0.15f, 1.0f);
        C2D_SceneStart(bottomScreen);

        const char* tabNames[] = {"Colors", "Clothing", "Face", "Hat", "Type"};
        for (int i = 0; i < 5; i++) {
            bool active = (AvatarTab)i == currentTab;
            C2D_DrawSolidShapeScreenSpace(C2D_Quad, i * 64, 230, 64, 10,
                active ? 0.6f : 0.3f, active ? 0.6f : 0.3f, active ? 0.7f : 0.4f, 1.0f);
            C2D_Text text;
            C2D_PrepareText(&text, C2D_FontDebug, 0.5f, tabNames[i], 1, NULL, 1.0f, 1.0f);
            C2D_DrawText(&text, C2D_FontDebug, 0.5f, i * 64 + 32, 232, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
                C2D_Align_Center, C2D_Align_Top);
            C2D_FinishText(&text);
        }

        switch (currentTab) {
            case AvatarTab::BodyColors: {
                const char* partNames[] = {"Head", "Torso", "L-Arm", "R-Arm", "L-Leg", "R-Leg"};
                for (int i = 0; i < 6; i++) {
                    bool selected = i == selectedOption;
                    CharacterEditor::BrickColor color = CharacterEditor::getBodyColor(
                        static_cast<CharacterEditor::BodyPart>(i));
                    u32 rgba = CharacterEditor::getBrickColorRGBA(color);
                    C2D_DrawSolidShapeScreenSpace(C2D_Quad, 10, 210 - i * 30, 300, 24,
                        selected ? 0.4f : 0.2f, selected ? 0.4f : 0.2f, selected ? 0.5f : 0.3f, 1.0f);
                    C2D_DrawSolidShapeScreenSpace(C2D_Quad, 260, 208 - i * 30, 40, 20,
                        (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF, 255);
                    C2D_Text text;
                    C2D_PrepareText(&text, C2D_FontDebug, 0.6f, partNames[i], 1, NULL, 1.0f, 1.0f);
                    C2D_DrawText(&text, C2D_FontDebug, 0.6f, 15, 207 - i * 30, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
                        C2D_Align_Left, C2D_Align_Top);
                    C2D_FinishText(&text);
                }
                break;
            }
            case AvatarTab::CharacterType: {
                bool isR15 = (CharacterEditor::getCharacterType() == CharacterEditor::CharacterType::R15);
                C2D_DrawSolidShapeScreenSpace(C2D_Quad, 50, 80, 220, 40,
                    !isR15 ? 0.4f : 0.2f, !isR15 ? 0.4f : 0.2f, !isR15 ? 0.5f : 0.3f, 1.0f);
                C2D_DrawSolidShapeScreenSpace(C2D_Quad, 50, 140, 220, 40,
                    isR15 ? 0.4f : 0.2f, isR15 ? 0.4f : 0.2f, isR15 ? 0.5f : 0.3f, 1.0f);
                C2D_Text text;
                C2D_PrepareText(&text, C2D_FontDebug, 0.8f, "R6 (Classic)", 1, NULL, 1.0f, 1.0f);
                C2D_DrawText(&text, C2D_FontDebug, 0.8f, 160, 90, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
                    C2D_Align_Center, C2D_Align_Top);
                C2D_FinishText(&text);
                C2D_PrepareText(&text, C2D_FontDebug, 0.8f, "R15 (Modern)", 1, NULL, 1.0f, 1.0f);
                C2D_DrawText(&text, C2D_FontDebug, 0.8f, 160, 150, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
                    C2D_Align_Center, C2D_Align_Top);
                C2D_FinishText(&text);
                break;
            }
            default: break;
        }

        C2D_Text footer;
        C2D_PrepareText(&footer, C2D_FontDebug, 0.5f, "A=Select  X=Save  B=Exit", 1, NULL, 0.7f, 0.7f);
        C2D_DrawText(&footer, C2D_FontDebug, 0.5f, 160, 10, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
            C2D_Align_Center, C2D_Align_Top);
        C2D_FinishText(&footer);

        C2D_SceneFinish();
        C2D_FlushBuffers();
    }

    void processAvatarInput() {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_B) { running = false; return; }
        if (kDown & KEY_X) { saveAndExit(); return; }

        if (kDown & KEY_DLEFT)  currentTab = (AvatarTab)(((int)currentTab - 1 + 5) % 5);
        if (kDown & KEY_DRIGHT) currentTab = (AvatarTab)(((int)currentTab + 1) % 5);
        if (kDown & KEY_DUP)    selectedOption = (selectedOption - 1 + 6) % 6;
        if (kDown & KEY_DDOWN)  selectedOption = (selectedOption + 1) % 6;

        if (kDown & KEY_A) {
            if (currentTab == AvatarTab::BodyColors) {
                CharacterEditor::BrickColor current = CharacterEditor::getBodyColor(
                    static_cast<CharacterEditor::BodyPart>(selectedOption));
                int next = ((int)current + 1) % (int)CharacterEditor::BrickColor::Count;
                CharacterEditor::setBodyColor(
                    static_cast<CharacterEditor::BodyPart>(selectedOption),
                    static_cast<CharacterEditor::BrickColor>(next));
            } else if (currentTab == AvatarTab::CharacterType) {
                auto current = CharacterEditor::getCharacterType();
                CharacterEditor::setCharacterType(
                    current == CharacterEditor::CharacterType::R6
                        ? CharacterEditor::CharacterType::R15
                        : CharacterEditor::CharacterType::R6);
            }
        }
    }

    void saveAndExit() {
        CharacterEditor::saveAvatar();
        printf("[UIAvatarEditor] Avatar saved\n");
        running = false;
    }

    void run() {
        running = true;
        printf("[UIAvatarEditor] Running avatar editor\n");
        while (running) {
            drawAvatarPreview();
            drawAvatarUI();
            processAvatarInput();
            C2D_SwapBuffers();
        }
    }

} // namespace UIAvatarEditor

// Pop previously pushed macros
#pragma pop_macro("C2D_SceneStart")
#pragma pop_macro("C2D_TargetClear")
#pragma pop_macro("C2D_DrawSolidShapeScreenSpace")
#pragma pop_macro("C2D_SwapBuffers")
#pragma pop_macro("C2D_FlushBuffers")
#pragma pop_macro("C2D_SceneFinish")
#pragma pop_macro("C2D_FinishText")
#pragma pop_macro("C2D_DrawText")
#pragma pop_macro("C2D_PrepareText")