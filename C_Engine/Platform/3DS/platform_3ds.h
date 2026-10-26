#pragma once

#include "3dsINC.h"

// Minimal platform hooks for 3DS
bool InitPlatform3DS();
void PollInput3DS();
void SwapBuffers3DS();
void ShutdownPlatform3DS();

// Map 3DS keys to engine input bits; engine should use these or map accordingly
#define KEY_UP_3DS    KEY_DUP
#define KEY_DOWN_3DS  KEY_DDOWN
#define KEY_LEFT_3DS  KEY_DLEFT
#define KEY_RIGHT_3DS KEY_DRIGHT
#define KEY_A_3DS     KEY_A
#define KEY_B_3DS     KEY_B

// Hotbar state (updated by processInput(), read by UI/gear code)
extern int selectedHotbarSlot;

