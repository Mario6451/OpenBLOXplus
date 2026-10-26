// Platform-agnostic input abstraction
#ifndef _INPUT_TYPES_H_
#define _INPUT_TYPES_H_

#include <cstdint>

using s16 = int16_t;
using u16 = uint16_t;
using u8 = uint8_t;

enum class ButtonType {

    A, B, X, Y,
    L, R,
    Start, Select,
    DPadUp, DPadDown, DPadLeft, DPadRight,
    Touch, Stylus
};

struct TouchEvent {
    s16 x, y;
    bool down;
    s16 dx, dy;   // delta since last frame
};

struct InputEvent {
    // Edge-detected (pressed once per frame)
    bool buttons_down[32]  = {};
    bool buttons_held[32]  = {};
    // Circle pad analog (-1..1)
    float circlePadX = 0.0f;
    float circlePadY = 0.0f;
    // Touch
    TouchEvent touch;
};

class InputSystem {
public:
    static void init();
    static void poll();   // call once per frame
    static const InputEvent& get();

    // Convenience queries on latest frame
    static bool isPressed(ButtonType);   // edge (down this frame)
    static bool isHeld(ButtonType);      // held state
    static float getCirclePadX();
    static float getCirclePadY();
    static TouchEvent getTouch();

private:
    static InputEvent currentInput;
    static s16 lastTouchX, lastTouchY;
};

#endif // _INPUT_TYPES_H_