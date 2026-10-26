#ifndef _IMPORTANT_INCLUDES
#define _IMPORTANT_INCLUDES

// Engine core that should be defined before the core platform includes
#include "CoreEngine/gameGlobalVariables.h"
#include "CoreEngine/mathHelper.h"
#include "CoreEngine/timing.h"
#include "ThirdParty/xml/include/xml.h"

// Physics
#include "ThirdParty/TinyPhysicsEngine/tinyphysicsengine.h"
extern TPE_World world;    // The physical world.

float distanceBetweenPoints(float x1, float y1, float z1, float x2, float y2, float z2);
float averageSize(float x, float y, float z);
void print_message(const char * text, ...);
extern int startEngine;
extern int platSingleApp;

extern float camera_x;
extern float camera_y;
extern float camera_z;
extern float camera_rx;
extern float camera_ry;
extern float camera_rz;

extern float vertical_axis;
extern float horizontal_axis;
extern float look_vertical_axis;
extern float look_horizontal_axis;
extern float fly_vertical_axis;

extern unsigned char jump;
extern unsigned char backpackItem;

#ifdef NDS_BUILD
    #define platformNameLoc STRING_PLATFORM_DS______
    #include "Platform/DS/dsINC.h"
#elif PC_BUILD
    #define platformNameLoc STRING_PLATFORM_PC______
    #include "Platform/PC/pcINC.h"
#elif WII_BUILD
    #define platformNameLoc STRING_PLATFORM_PC______
    #include "Platform/Wii/wii_specific.h"
    #include "Platform/PC/pcINC.h"
#elif defined(BUILD_3DS)
    #define platformNameLoc STRING_PLATFORM_DS______
    #include "Platform/3DS/3dsINC.h"
#else
    #error "No platform build macro defined. Define PC_BUILD, NDS_BUILD, WII_BUILD, or BUILD_3DS."
#endif

// Post platform includes
#include "CoreEngine/color.h"
#include "CoreEngine/GL/gameGL_globals.h"

// Level/Model stuff
#include "CoreEngine/Node/nodes.h"
#include "./rbxl.h"

#ifdef NDS_BUILD
    #include "Platform/DS/ds_function_loading.h"
#elif PC_BUILD
    #include "Platform/PC/pc_function_loading.h"
#elif WII_BUILD
    #include "Platform/PC/pc_function_loading.h"
#endif

#endif
