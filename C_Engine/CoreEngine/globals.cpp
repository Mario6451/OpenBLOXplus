#include "globals.h"

int platSingleApp = 0;
int startEngine = 0;

float camera_x = 0.0f;
float camera_y = 0.0f;
float camera_z = 0.0f;
float camera_rx = 0.0f;
float camera_ry = 0.0f;
float camera_rz = 0.0f;

float vertical_axis = 0.0f;
float horizontal_axis = 0.0f;
float fly_vertical_axis = 0.0f;

float look_horizontal_axis = 0.0f;
float look_vertical_axis = 0.0f;

unsigned char jump = 0;
unsigned char backpackItem = 0;

TPE_World world;

// Extern variable definitions (declared in headers)

// timing.h - timer globals
double previousTime = 0;
float deltaTime;

// gameGlobalVariables.h - game state globals
unsigned int currentLanguageCode = 0;
unsigned char isGameRunning = 1;
unsigned char isDebgRunning = 0;
unsigned char runtimeMode = 0;

// 3dsINC.h - platform globals
const char* platformName = "Nintendo 3DS";
float maxPartDist = 256;

// locale.h - localization globals
const char *locale_codes[] = {"en", "es", "tok", 0};
const char *locale_names[] = {"English", "Español", "toki pona", 0};
const char *strings_debug[][3] = {{"Current Language", "Idioma Actual", "toki li lon"}, {"Current Platform", "Plataforma Actual", "ilo musi li lon"}, {0}};
const char *strings_lingo[][3] = {{"English", "Spanish", "Toki Pona"}, {"Inglés", "Español", "Toki Pona"}, {"toki Inli", "toki Epanja", "toki pona"}, {0}};
const char *strings_platform[][3] = {{"Nintendo DS", "Nintendo DS", "ilo Si Esu"}, {"Nintendo DSi", "Nintendo DSi", "ilo Si Esu Ije"}, {"Nintendo Wii", "Nintendo Wii", "ilo Wi"}, {"PC", "Computadora", "ilo lipu"}, {"SEGA Dreamcast", "SEGA Dreamcast", "ilo Seka Sikaso"}, {0}};
