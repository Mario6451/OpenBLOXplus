#pragma once

#include <3ds.h>

// Include platform-specific function headers used by the engine
#include "3ds_function_debug.h"
#include "3ds_function_opnGL.h"
#include "3ds_function_time.h"
#include "3ds_function_loading.h"

// Declarations for platform lifecycle used by main.cpp
void systemStart();
void processInput();
void initOpenGL();
void perspectiveModeGL();

// Platform name
extern const char* platformName;
// Maximum part draw distance (match DS default)
extern float maxPartDist;
