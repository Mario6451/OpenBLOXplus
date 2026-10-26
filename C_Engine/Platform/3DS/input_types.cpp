#include "input_types.h"
#include <3ds.h>
#include <cmath>
#include <algorithm>

InputEvent InputSystem::currentInput;
s16 InputSystem::lastTouchX = 0;
s16 InputSystem::lastTouchY = 0;

static int buttonToIndex(ButtonType b) {
    switch (b) {
        case ButtonType::A:       return 0;
        case ButtonType::B:       return 1;
        case ButtonType::X:       return 2;
        case ButtonType::Y:       return 3;
        case ButtonType::L:       return 4;
        case ButtonType::R:       return 5;
        case ButtonType::Start:   return 6;
        case ButtonType::Select:  return 7;
        case ButtonType::DPadUp:  return 8;
        case ButtonType::DPadDown: return 9;
        case ButtonType::DPadLeft: return 10;
        case ButtonType::DPadRight: return 11;
        case ButtonType::Touch:   return 12;
        case ButtonType::Stylus:  return 13;
        default: return -1;
    }
}

void InputSystem::init() {
    // Nothing extra needed; ctrulib already initializes HID
}

void InputSystem::poll() {
    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    // Reset state
    InputEvent ev;

    // Buttons
    ev.buttons_down[buttonToIndex(ButtonType::A)]       = !!(kDown & KEY_A);
    ev.buttons_down[buttonToIndex(ButtonType::B)]       = !!(kDown & KEY_B);
    ev.buttons_down[buttonToIndex(ButtonType::X)]       = !!(kDown & KEY_X);
    ev.buttons_down[buttonToIndex(ButtonType::Y)]       = !!(kDown & KEY_Y);
    ev.buttons_down[buttonToIndex(ButtonType::L)]       = !!(kDown & KEY_L);
    ev.buttons_down[buttonToIndex(ButtonType::R)]       = !!(kDown & KEY_R);
    ev.buttons_down[buttonToIndex(ButtonType::Start)]   = !!(kDown & KEY_START);
    ev.buttons_down[buttonToIndex(ButtonType::Select)]  = !!(kDown & KEY_SELECT);
    ev.buttons_down[buttonToIndex(ButtonType::DPadUp)]  = !!(kDown & KEY_DUP);
    ev.buttons_down[buttonToIndex(ButtonType::DPadDown)]= !!(kDown & KEY_DDOWN);
    ev.buttons_down[buttonToIndex(ButtonType::DPadLeft)]= !!(kDown & KEY_DLEFT);
    ev.buttons_down[buttonToIndex(ButtonType::DPadRight)]= !!(kDown & KEY_DRIGHT);

    // Held state
    ev.buttons_held[buttonToIndex(ButtonType::A)]       = !!(kHeld & KEY_A);
    ev.buttons_held[buttonToIndex(ButtonType::B)]       = !!(kHeld & KEY_B);
    ev.buttons_held[buttonToIndex(ButtonType::X)]       = !!(kHeld & KEY_X);
    ev.buttons_held[buttonToIndex(ButtonType::Y)]       = !!(kHeld & KEY_Y);
    ev.buttons_held[buttonToIndex(ButtonType::L)]       = !!(kHeld & KEY_L);
    ev.buttons_held[buttonToIndex(ButtonType::R)]       = !!(kHeld & KEY_R);
    ev.buttons_held[buttonToIndex(ButtonType::Start)]   = !!(kHeld & KEY_START);
    ev.buttons_held[buttonToIndex(ButtonType::Select)]  = !!(kHeld & KEY_SELECT);
    ev.buttons_held[buttonToIndex(ButtonType::DPadUp)]  = !!(kHeld & KEY_DUP);
    ev.buttons_held[buttonToIndex(ButtonType::DPadDown)]= !!(kHeld & KEY_DDOWN);
    ev.buttons_held[buttonToIndex(ButtonType::DPadLeft)]= !!(kHeld & KEY_DLEFT);
    ev.buttons_held[buttonToIndex(ButtonType::DPadRight)]= !!(kHeld & KEY_DRIGHT);

    // Touch
    ev.buttons_down[buttonToIndex(ButtonType::Touch)]   = !!(kDown & KEY_TOUCH);
    ev.buttons_held[buttonToIndex(ButtonType::Touch)]   = !!(kHeld & KEY_TOUCH);

    // Circle pad
    circlePosition cp;
    hidCircleRead(&cp);
    float cx = (float)cp.dx / 1024.0f;
    float cy = -(float)cp.dy / 1024.0f;
    ev.circlePadX = std::clamp(cx, -1.0f, 1.0f);
    ev.circlePadY = std::clamp(cy, -1.0f, 1.0f);


    // Touch position and delta
    touchPosition tp;
    touchRead(&tp);
    if (kHeld & KEY_TOUCH) {
        ev.touch.x  = tp.px;
        ev.touch.y  = tp.py;
        ev.touch.down = true;
        ev.touch.dx = tp.px - lastTouchX;
        ev.touch.dy = tp.py - lastTouchY;
        lastTouchX = tp.px;
        lastTouchY = tp.py;
    } else {
        ev.touch.down = false;
        ev.touch.dx = 0;
        ev.touch.dy = 0;
    }

    currentInput = ev;
}

const InputEvent& InputSystem::get() { return currentInput; }

bool InputSystem::isPressed(ButtonType b) {
    int idx = buttonToIndex(b);
    return idx >= 0 && currentInput.buttons_down[idx];
}

bool InputSystem::isHeld(ButtonType b) {
    int idx = buttonToIndex(b);
    return idx >= 0 && currentInput.buttons_held[idx];
}

float InputSystem::getCirclePadX() { return currentInput.circlePadX; }
float InputSystem::getCirclePadY() { return currentInput.circlePadY; }
TouchEvent InputSystem::getTouch() { return currentInput.touch; }