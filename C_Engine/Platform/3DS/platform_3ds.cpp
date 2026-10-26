#include "platform_3ds.h"
#include "player_controller.h"
#include "CoreEngine/globals.h"          // defines global variables and lerp()
#include "Platform/3DS/3ds_function_time.h"  // for deltaTimeCalc()
#include "Platform/3DS/voice_chat.h"
#include "Platform/3DS/input_types.h"

// Hotbar state (accessed by UI/gear code via platform_3ds.h)
int selectedHotbarSlot = 0;

// Minimal 3DS platform implementation using libctru.
// Provides console output and basic input polling.
// Graphics rendering integration (Citro3D) and audio need to be added.

bool InitPlatform3DS() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    InputSystem::init();
    // Optionally initialize other subsystems here (apt, srv, hid already used)
    return true;
}

void PollInput3DS() {
    InputSystem::poll();
}

void SwapBuffers3DS() {
    // If using console only, flush buffers; for GPU rendering integrate Citro3D
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void ShutdownPlatform3DS() {
    gfxExit();
}

// Bridge functions expected by the engine
void systemStart() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    consoleInit(GFX_BOTTOM, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    aptInit();
    fsInit();
    archiveMountSdmc();

    platSingleApp = 1;
    startEngine = 1;

    // initialize Citro3D renderer (software fallback currently)
    CitroRenderer::init(400, 240);

    // initialize player controller at origin
    PlayerController::initPlayer(0.0f, 2.0f, 0.0f);
}

void processInput() {
    InputSystem::poll();

    // Reset axes
    vertical_axis   = 0;
    horizontal_axis = 0;
    fly_vertical_axis = 0;

    // Circle Pad - Movement in all directions (analog)
    float cpX = InputSystem::getCirclePadX();
    float cpY = InputSystem::getCirclePadY();

    // Circle pad: Y up = forward, Y down = backward
    // Circle pad: X left = strafe left, X right = strafe right
    vertical_axis   = cpY;
    horizontal_axis = cpX;

    // D-Pad - Additional strafe / movement control
    if (InputSystem::isHeld(ButtonType::DPadLeft))  horizontal_axis -= 0.5f;
    if (InputSystem::isHeld(ButtonType::DPadRight)) horizontal_axis += 0.5f;
    if (InputSystem::isHeld(ButtonType::DPadUp))    vertical_axis += 0.5f;
    if (InputSystem::isHeld(ButtonType::DPadDown))  vertical_axis -= 0.5f;

    // Clamp axes to [-1, 1]
    if (vertical_axis   >  1.0f) vertical_axis   =  1.0f;
    if (vertical_axis   < -1.0f) vertical_axis   = -1.0f;
    if (horizontal_axis >  1.0f) horizontal_axis =  1.0f;
    if (horizontal_axis < -1.0f) horizontal_axis = -1.0f;

    // A - Jump
    if (InputSystem::isPressed(ButtonType::A)) {
        // jump_requested = true;
    }

    // B - Interact / Cancel
    if (InputSystem::isPressed(ButtonType::B)) {
        // interact_requested = true;
    }

    // X - Open Backpack
    if (InputSystem::isPressed(ButtonType::X)) {
        // backpack_open = !backpack_open;
    }

    // Y - Push to talk (voice chat)
    VoiceChat::setTalking(InputSystem::isHeld(ButtonType::Y));

    // L/R - Hotbar slot navigation
    if (InputSystem::isPressed(ButtonType::L)) {
        selectedHotbarSlot--;
        if (selectedHotbarSlot < 0) selectedHotbarSlot = 9;
    }
    if (InputSystem::isPressed(ButtonType::R)) {
        selectedHotbarSlot++;
        if (selectedHotbarSlot > 9) selectedHotbarSlot = 0;
    }

    double dt = deltaTimeCalc();

    // Camera movement (TODO: move to separate camera controller)
    camera_x += (horizontal_axis * 64) * dt;
    camera_y += (fly_vertical_axis * 64) * dt;
    camera_z += (vertical_axis * 64) * dt;

    // Touch input - Camera / Look (swipe to rotate)
    TouchEvent touch = InputSystem::getTouch();
    if (touch.down) {
        look_horizontal_axis = lerp(look_horizontal_axis, touch.dx * 60, dt * 2);
        look_vertical_axis   = lerp(look_vertical_axis, touch.dy * 60, dt * 2);
    } else {
        look_horizontal_axis = lerp(look_horizontal_axis, 0, dt * 2);
        look_vertical_axis   = lerp(look_vertical_axis, 0, dt * 2);
    }
}

// OpenGL initialization handled inline in 3ds_function_opnGL.h
// void initOpenGL() {}
// void perspectiveModeGL() {}
