/*
    OpenBLOX Math Helper
*/

#ifndef _MATHHELP
#define _MATHHELP

#include <cmath>

__attribute__((weak)) inline float lengthdir_x(float length, float dir) {
    return length * sin(-dir*0.0174533);
}

__attribute__((weak)) inline float lengthdir_y(float length, float dir) {
    return length * cos(-dir*0.0174533);
}

__attribute__((weak)) inline float lengthdir_z(float length, float dir) {
    return length * sin(-dir*0.0174533);
}

__attribute__((weak)) inline float clamp(float value, float min, float max) {
    if (value > max) {
        return max;
    } else if (value < min) {
        return min;
    } else {
        return value;
    }
}

__attribute__((weak)) inline float lerp(float start, float end, float t) {
  return start + t * (end - start);
}

#endif