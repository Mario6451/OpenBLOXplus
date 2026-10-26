#pragma once

#include <time.h>

static clock_t lastClkTime = 0;

__attribute__((weak)) static inline clock_t get_clock_time() {
    return clock();
}
    
__attribute__((weak)) static inline double deltaTimeCalc() {
    double currentTime = get_clock_time();
    if (lastClkTime == 0) lastClkTime = currentTime;
    double dt = (currentTime - lastClkTime)/(CLOCKS_PER_SEC*2);
    lastClkTime = currentTime;
    return dt;
}
