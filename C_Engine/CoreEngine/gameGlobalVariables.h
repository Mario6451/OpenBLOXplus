/*
    OpenBLOX global variables
*/

#include "./Node/nodes.h"

#include "./Localization/locale.h"
extern unsigned int currentLanguageCode;

extern unsigned char isGameRunning;
extern unsigned char isDebgRunning;

#define MINIGAME_MODE 0
#define PLAYSOLO_MODE 1
#define ONLINEPL_MODE 2
#define __STUDIO_MODE 3

extern unsigned char runtimeMode;

void gameRenderLoop();