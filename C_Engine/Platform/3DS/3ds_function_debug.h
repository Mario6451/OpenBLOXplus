#pragma once

#include <3ds.h>
#include <cstdio>   // provides stdout, _IONBF, setvbuf
#include <stdarg.h> // for va_list

__attribute__((weak)) static inline void enableDebugConsole(unsigned char enable) {
    if (enable) {
        gfxInitDefault();
        consoleInit(GFX_TOP, NULL);
        setvbuf(stdout, NULL, _IONBF, 0);
    }
}

__attribute__((weak)) inline void vprint_message(const char* text, va_list args) {
    vprintf(text, args);
}

__attribute__((weak)) inline void print_message(const char* text, ...) {
    va_list args;
    va_start(args, text);
    vprintf(text, args);
    va_end(args);
}
