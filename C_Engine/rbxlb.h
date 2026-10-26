#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "CoreEngine/Node/nodes.h"

// Forward declaration for logging function used by RBXL binary parser
void print_message(const char *fmt, ...);

// Binary RBXL parser stub - basic header detection
inline Node* parseRBXMb(FILE* f, bool place) {
    fseek(f, 0, SEEK_SET);
    char header[8];
    if (fread(header, 1, 8, f) != 8) return NULL;
    
    // Check for binary RBXL magic: "<roblox!" signature
    if (strncmp(header, "<roblox!", 8) == 0) {
        print_message("Binary RBXL format detected (not supported yet)\n");
    } else {
        print_message("Unknown binary format\n");
    }
    return NULL;
}
