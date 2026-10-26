/*

    OpenBLOX Engine: Locale file
    @17:43 on 2024-02-26

*/

#ifndef _LOCALES_H
#define _LOCALES_H

extern "C" {

extern const char *locale_codes[];
extern const char *locale_names[];

// Hardcoded strings
#define STRING_CURRENT_LANGUAGE 0
#define STRING_CURRENT_PLATFORM 1
extern const char *strings_debug[][3];
extern const char *strings_lingo[][3];
extern const char *strings_platform[][3];

#define STRING_PLATFORM_DS______ 0
#define STRING_PLATFORM_DSI_____ 1
#define STRING_PLATFORM_PC______ 2

}

#endif