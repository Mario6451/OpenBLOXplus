#pragma once

#include <stdint.h>
#include "ThirdParty/TinyPhysicsEngine/tinyphysicsengine.h"

// Global engine state variables
extern int platSingleApp;
extern int startEngine;

// Camera and input axes
extern float camera_x, camera_y, camera_z;
extern float camera_rx, camera_ry, camera_rz;
extern float vertical_axis, horizontal_axis, fly_vertical_axis;
extern float look_horizontal_axis, look_vertical_axis;

// Player state
extern unsigned char jump;
extern unsigned char backpackItem;

// Physics world
extern TPE_World world;

// Utility function
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
