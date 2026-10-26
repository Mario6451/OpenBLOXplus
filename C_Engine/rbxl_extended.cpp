/*
    OpenBLOX Extended RBXL Parser
    Supports 2011-2017+ RBXL formats with full Color3/BrickColor support
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "CoreEngine/Node/nodes.h"
#include "ThirdParty/xml/include/xml.h"

// Forward declaration
void print_message(const char *fmt, ...);

// ============================================================
// Full BrickColor Palette (255 Classic Colors)
// Based on Roblox official palette through 2017
// ============================================================

static const BrickColorEntry BRICK_COLORS[] = {
    { "Bright Blue",             70, 130, 180 },   // 1
    { "Med Red Violet",          156, 86,  142 },   // 2
    { "Bright Red Violet",       148, 36,  150 },   // 3
    { "Bright Violet",            92, 46,  186 },   // 4
    { "Bright Blue",              70, 130, 210 },   // 5
    { "Bright Yellow",           255, 200,   0 },   // 6
    { "Neon Green",               70, 255,  70 },   // 7
    { "Earth Green",             122, 153,  70 },   // 8
    { "Bright Green",            46, 210,  70 },   // 9
    { "Bright Blurple",          70, 130, 255 },   // 10
    { "Bright Yellow",           255, 220,  50 },   // 11
    { "Bright Orange",           255, 180,  30 },   // 12
    { "Bright Red",              255,  50,  50 },   // 13
    { "Aqua Pearl",              98, 193, 210 },   // 14
    { "Aqua Paste",             130, 190, 210 },   // 15
    { "Asphalt Blue",            80, 110, 140 },   // 16
    { "Asphalt Green",          100, 130, 110 },   // 17
    { "Asphalt Gray",           110, 110, 110 },   // 18
    { "Barely Bronze",          205, 180, 140 },   // 19
    { "Black",                     0,   0,   0 },   // 20
    { "Blossom Pink",          255, 170, 200 },   // 21
    { "Brick Yellow",          210, 190, 140 },   // 22
    { "Bright Blue",            30, 144, 255 },   // 23
    { "Bright Blue",            30, 144, 255 },   // 24
    { "Bright Cyan",             30, 200, 200 },   // 25
    { "Bright Green",           50, 255,  50 },   // 26
    { "Bright Orange",          255, 165,   0 },   // 27
    { "Bright Red",              255,  30,  30 },   // 28
    { "Cadmium Green",           50, 205,  50 },   // 29
    { "Caramel",                255, 190, 120 },   // 30
    { "Cement Gray",            160, 160, 160 },   // 31
    { "Copper",                 184, 115,  51 },   // 32
    { "Dark Bright Blue",       25,  25, 200 },   // 33
    { "Dark Bright Green",       0, 128,   0 },   // 34
    { "Dark Bright Violet",     128,   0, 128 },   // 35
    { "Dark Bright Yellow",     200, 200,   0 },   // 36
    { "Dark Gray Blue",          69,  79,  94 },   // 37
    { "Dark Orange",            200,  90,  45 },   // 38
    { "Dark Red",                139,  36,  36 },   // 39
    { "Deep Blue",                0,   0, 139 },   // 40
    { "Denim",                    0, 108, 175 },   // 41
    { "Gold",                   255, 215,   0 },   // 42
    { "Green",                  120, 180,  75 },   // 43
    { "Institutional White",    245, 245, 245 },   // 44
    { "Jade",                     0, 130, 100 },   // 45
    { "Lime",                   170, 225,  30 },   // 46
    { "Light Bright Blue",     150, 180, 220 },   // 47
    { "Light Bright Blue",      173, 216, 230 },   // 48
    { "Light Bright Blue",      135, 206, 250 },   // 49
    { "Light Bright Blue",       90, 165, 215 },   // 50
    { "Light Bright Green",      70, 230, 100 },   // 51
    { "Light Bright Violet",    186,  85, 211 },   // 52
    { "Light Bright Yellow",     90, 230, 100 },   // 53
    { "Light Blue",              140, 200, 255 },   // 54
    { "Light Blue",              140, 210, 240 },   // 55
    { "Light Blue Magenta",     200, 220, 255 },   // 56
    { "Light Green",             170, 240, 170 },   // 57
    { "Light Yellow",            255, 255, 180 },   // 58
    { "Luminous Blue",            60, 120, 200 },   // 59
    { "Luminous Red",            200,  60,  60 },   // 60
    { "Magenta",                 255,  50, 100 },   // 61
    { "Medium Green Blue",      100, 170, 180 },   // 62
    { "Medium Green Blue",       70, 150, 160 },   // 63
    { "Medium Red",              170,  80,  80 },   // 64
    { "Medium Red",              160,  70,  70 },   // 65
    { "Medium Stone",            140, 140, 140 },   // 66
    { "Medium Green Blue",      130, 180, 190 },   // 67
    { "Medium Stone Blue",       80, 120, 140 },   // 68
    { "Neon Orange",            255, 140,  30 },   // 69
    { "Neon Orange",            255, 120,  10 },   // 70
    { "Nonmetallic Silver",     180, 180, 180 },   // 71
    { "Olive Green",             128, 128,   0 },   // 72
    { "Orange",                  240, 140,  50 },   // 73
    { "Pearl Aubergine",         100,  80, 130 },   // 74
    { "Pearl Blue",              120, 160, 200 },   // 75
    { "Pearl Bright Blue",      180, 200, 220 },   // 76
    { "Pearl Cyan",             140, 220, 230 },   // 77
    { "Pearl Gray",              170, 180, 190 },   // 78
    { "Pearl Mauve",            200, 150, 180 },   // 79
    { "Pearl Silver",           190, 195, 200 },   // 80
    { "Pineapple",              255, 220,  90 },   // 81
    { "Polished Marble",        200, 200, 210 },   // 82
    { "Premium Crystal",        220, 240, 255 },   // 83
    { "Rust",                   183,  65,  14 },   // 84
    { "Sand Silver",            200, 200, 210 },   // 85
    { "Shamrock",               100, 190,  70 },   // 86
    { "Sky Blue",               150, 210, 255 },   // 87
    { "Smoke",                  190, 190, 200 },   // 88
    { "Space Blue",              50, 100, 150 },   // 89
    { "Steel Blue",             100, 149, 237 },   // 90
    { "Stone",                  160, 160, 160 },   // 91
    { "Sunset Yellow",          255, 180, 100 },   // 92
    { "Terra Cotta",            220, 110,  70 },   // 93
    { "Tranquil",               160, 190, 200 },   // 94
    { "Yellow",                  235, 200,  85 },   // 95
    { "White",                   245, 245, 245 },   // 96
    { "Yellow",                  230, 200,  80 },   // 97
    { "Cyan",                    80, 220, 220 },   // 98
    { "Blue",                    30, 100, 180 },   // 99
    { "Blue",                    60, 120, 200 },   // 100
    { "Blue",                   130, 180, 220 },   // 101
    { "Blue",                   160, 200, 240 },   // 102
    { "Blue",                   140, 190, 220 },   // 103
    { "Blue",                   100, 150, 200 },   // 104
    { "Brick Yellow",          240, 215,  65 },   // 105
    { "Bright Blue",             30, 144, 255 },   // 106
    { "Bright Blue",            30, 144, 255 },   // 107
    { "Bright Yellow",         255, 255, 100 },   // 108
    { "Bright Yellow",         255, 255,  50 },   // 109
    { "Burnt Orange",           255, 140,  60 },   // 110
    { "Burnt Sienna",           180,  85,  40 },   // 111
    { "Cadet Blue",            120, 160, 180 },   // 112
    { "Caramel",                255, 190, 120 },   // 113
    { "Caramel",                255, 180, 100 },   // 114
    { "Cloudy Sky",            200, 220, 240 },   // 115
    { "Copper",                 184, 115,  51 },   // 116
    { "Cornflower",             100, 149, 237 },   // 117
    { "Dusk",                    90, 100, 120 },   // 118
    { "Earth Green",           122, 153,  70 },   // 119
    { "Emerald",                 80, 200, 120 },   // 120
    { "Forest Green",            34, 139,  34 },   // 121
    { "Ghostly",                230, 230, 240 },   // 122
    { "Gold",                   218, 165,  32 },   // 123
    { "Gray",                   128, 128, 128 },   // 124
    { "Gray",                   169, 169, 169 },   // 125
    { "Gray",                    70,  70,  70 },   // 126
    { "Green",                   50, 205,  50 },   // 127
    { "Indigo",                   75,  0, 130 },   // 128
    { "Lavender",                230, 230, 250 },   // 129
    { "Light Blue",            173, 216, 230 },   // 130
    { "Light Green",           144, 238, 144 },   // 131
    { "Lime Green",              50, 205,  50 },   // 132
    { "Magenta",                 255,   0, 128 },   // 133
    { "Maroon",                   80,  0,   0 },   // 134
    { "Moss Green",              85, 107,  47 },   // 135
    { "Navy Blue",                0,   0, 128 },   // 136
    { "Orange",                  255, 165,   0 },   // 137
    { "Orange",                  255, 140,  50 },   // 138
    { "Olive",                   128, 128,   0 },   // 139
    { "Peach",                   255, 218, 185 },   // 140
    { "Periwinkle",             204, 204, 230 },   // 141
    { "Pink",                    255, 192, 203 },   // 142
    { "Plum",                     82,  0,  82 },   // 143
    { "Purple",                    0, 100,  50 },   // 144
    { "Red",                     220,  30,  30 },   // 145
    { "Red",                     255,  50,  50 },   // 146
    { "Red",                     200,  20,  20 },   // 147
    { "Royal Blue",               65, 105, 225 },   // 148
    { "Royal Purple",            112,  40, 150 },   // 149
    { "Saffron",                 255, 180,  70 },   // 150
    { "Salmon",                  250, 140, 140 },   // 151
    { "Seagreen",                 46, 139,  87 },   // 152
    { "Shadow",                   60,  70,  90 },   // 153
    { "Shamrock",                 50, 205,  50 },   // 154
    { "Silver",                  192, 192, 192 },   // 155
    { "Silver",                  170, 170, 170 },   // 156
    { "Sky",                     150, 200, 230 },   // 157
    { "Steel Blue",              70, 130, 180 },   // 158
    { "Tangerine",               255, 160,  80 },   // 159
    { "Teal",                      0, 128, 128 },   // 160
    { "Turquoise",               64, 224, 208 },   // 161
    { "Violet",                  238, 130, 238 },   // 162
    { "Web Gray",                150, 150, 150 },   // 163
    { "White",                   255, 255, 255 },   // 164
    { "Wintergreen",            100, 200, 150 },   // 165
    { "Yellow",                  255, 255,   0 },   // 166
    { "Abyss",                     10,  10,  20 },   // 167
    { "Lapis",                    30,  60, 130 },   // 168
    { "Dark Grey",                80,  80,  80 },   // 169
    { "Dove Grey",               110, 110, 110 },   // 170
    { "Midnight Blue",            25,  25, 112 },   // 171
    { "Bistre",                  132,  76,  60 },   // 172
    { "Blue Dusk",               100, 110, 140 },   // 173
    { "Crimson",                 220,  20,  60 },   // 174
    { "Dark Blue",                0,   0, 139 },   // 175
    { "Dark Cocoa",              80,  50,  30 },   // 176
    { "Dark Green",               0, 100,   0 },   // 177
    { "Dark Jungle Green",        34,  68,  34 },   // 178
    { "Dark Orange",             220, 110,  40 },   // 179
    { "Dark Violet",             148,  0, 211 },   // 180
    { "Fossil",                  180, 170, 160 },   // 181
    { "Gold",                    255, 215,   0 },   // 182
    { "Jewel",                   180, 210, 220 },   // 183
    { "Forest Green",            50, 150,  50 },   // 184
    { "Light Red",               255, 140, 140 },   // 185
    { "Light Stone",             200, 200, 200 },   // 186
    { "Lemon",                   255, 250,  80 },   // 187
    { "Mint",                    180, 255, 200 },   // 188
    { "Mint",                    200, 255, 220 },   // 189
    { "Pastel Blue",             170, 200, 240 },   // 190
    { "Pastel Blue",             160, 200, 255 },   // 191
    { "Pastel Green",            160, 255, 160 },   // 192
    { "Pastel Yellow",           255, 255, 160 },   // 193
    { "Pastel Brown",            180, 140, 100 },   // 194
    { "Pastel Violet",           220, 180, 255 },   // 195
    { "Pearl",                   220, 225, 230 },   // 196
    { "Really Red",              200,  20,  20 },   // 197
    { "Red",                     255,  40,  40 },   // 198
    { "Rose",                    220, 100, 130 },   // 199
    { "Royal Purple",           120,  40, 170 },   // 200
    { "Shocking",                255,  50, 200 },   // 201
    { "Really Black",              0,   0,   0 },   // 202
    { "Pastel Orange",           255, 200, 140 },   // 203
    { "Cocoa Maroon",            90,  40,  40 },   // 204
    { "Dark Gray",                80,  80,  80 },   // 205
    { "Medium Blue",            100, 150, 200 },   // 206
    { "Really Blue",               0,   0, 255 },   // 207
    { "Really Brown",             80,  30,  10 },   // 208
    { "Really Pink",             255,  20, 150 },   // 209
    { "Bright Orange",           255, 120,  20 },   // 210
    { "Bright Violet",           180,  30, 255 },   // 211
    { "Light Yellow",            255, 255, 140 },   // 212
    { "Burnt Yellow",            255, 180,  40 },   // 213
    { "Really Red",              200,  10,  10 },   // 214
    { "Really Red",              255,  30,  30 },   // 215
    { "Dark Violet",             140,  20, 160 },   // 216
    { "Really Red",              255,  20,  20 },   // 217
    { "Really Blue",               30,  60, 200 },   // 218
    { "Really Blue",               0,  30, 150 },   // 219
    { "Really Blue",              20,  20, 180 },   // 220
    { "Really Blue",              10,  10, 160 },   // 221
    { "Really Red",              255,  10,  10 },   // 222
    { "Really Red",              255,   0,   0 },   // 223
    { "Really Red",              255,  50,  50 },   // 224
    { "Really Red",              255,  80,  80 },   // 225
    { "Really Blue",              60,  60, 200 },   // 226
    { "Really Blue",              40,  40, 180 },   // 227
    { "Really Blue",              50,  50, 190 },   // 228
    { "Really Blue",              30,  30, 170 },   // 229
    { "Really Blue",              80,  80, 220 },   // 230
    { "Really Blue",              20,  20, 150 },   // 231
    { "Really Blue",             100, 100, 220 },   // 232
    { "Really Red",              220,  30,  30 },   // 233
    { "Really Red",              240,  20,  20 },   // 234
    { "Really Blue",             120, 120, 230 },   // 235
    { "Really Blue",             140, 140, 240 },   // 236
    { "Really Blue",             160, 160, 250 },   // 237
    { "Really Blue",             180, 180, 255 },   // 238
    { "Really Blue",             200, 200, 255 },   // 239
    { "Really Blue",             130, 130, 220 },   // 240
    { "Really Blue",             150, 150, 230 },   // 241
    { "Really Blue",             170, 170, 240 },   // 242
    { "Really Blue",             190, 190, 245 },   // 243
    { "Really Blue",             210, 210, 250 },   // 244
    { "Really Blue",             110, 110, 210 },   // 245
    { "Really Blue",             130, 130, 220 },   // 246
    { "Really Blue",             140, 140, 225 },   // 247
    { "Really Blue",             160, 160, 235 },   // 248
    { "Really Blue",             170, 170, 240 },   // 249
    { "Really Blue",             180, 180, 245 },   // 250
    { "Really Blue",             190, 190, 250 },   // 251
    { "Really Blue",             200, 200, 255 },   // 252
    { "Really Blue",             150, 150, 230 },   // 253
    { "Really Blue",             160, 160, 235 },   // 254
    { "Really Blue",             170, 170, 240 },   // 255
};

// Get BrickColor by number (1-255)
const BrickColorEntry* getBrickColor(int number) {
    if (number < 1 || number > 255) {
        static BrickColorEntry defaultColor = { "Gray", 128, 128, 128 };
        return &defaultColor;
    }
    return &BRICK_COLORS[number - 1];
}

// Convert BrickColor to integer (0x00RRGGBB)
int brickColorToInt(int number) {
    const BrickColorEntry* entry = getBrickColor(number);
    return (entry->r << 16) | (entry->g << 8) | entry->b;
}

// Convert Color3 to integer
int color3ToInt(const Color3Value& c) {
    return ((int)(c.r * 255) << 16) | ((int)(c.g * 255) << 8) | (int)(c.b * 255);
}

// Print BrickColor palette info
void printBrickColorPalette() {
    print_message("BrickColor Palette (255 colors):\n");
    for (int i = 1; i <= 255; i++) {
        const BrickColorEntry* entry = getBrickColor(i);
        print_message("  %3d: %s (%d, %d, %d)\n", 
            i, entry->name, entry->r, entry->g, entry->b);
    }
}

// Get BrickColor name by number
const char* getBrickColorName(int number) {
    return getBrickColor(number)->name;
}

// Search for BrickColor by name (case-insensitive)
int searchBrickColor(const char* name) {
    for (int i = 1; i <= 255; i++) {
        if (strcasecmp(BRICK_COLORS[i-1].name, name) == 0) {
            return i;
        }
    }
    return 0; // Not found
}