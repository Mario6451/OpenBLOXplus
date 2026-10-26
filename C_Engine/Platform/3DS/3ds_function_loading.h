#pragma once

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <3ds.h>
// file.h may not be available in all devkitPro setups; use archiveMountSdmc instead of sdmcInit
#include <3ds/archive.h>
#include <string>
#include <vector>
#include "ui_file_selector.h"

#include "../../rbxl.h"

// Adapted file loader for 3DS using sdmc (SD card) filesystem

__attribute__((weak)) inline void levelLoaderMenu(Node **dataModel, Node **character) {
	fsInit();
	archiveMountSdmc();

	DIR *pdir;
	struct dirent *pent;

	consoleInit(GFX_BOTTOM, NULL);
	pdir = opendir("sdmc:/OpenBLOX");

	if (pdir) {
		const char *mapFile[1024];
		int mapLength = 0;

		while ((pent = readdir(pdir)) != NULL) {
			if (!(strcmp(".", pent->d_name) == 0 || strcmp("..", pent->d_name) == 0)) {
				if (pent->d_type == DT_DIR) {
					printf("Dir: [%s]\n", pent->d_name);
				} else {
					printf("%s\n", pent->d_name);
					mapFile[mapLength] = strdup(pent->d_name);
					mapLength++;
					if (mapLength >= 1024) break;
				}
			}
		}

		if (mapLength == 0) {
			iprintf("No maps found in sdmc:/OpenBLOX\n");
			closedir(pdir);
			return;
		}

		int fSelectNumb = 0; // default to first
		bool selected = false;
		int topIndex = 0;

		// Build vector of filenames and call graphical selector
		std::vector<std::string> names;
		for (int i = 0; i < mapLength; ++i) names.push_back(std::string(mapFile[i]));
		int sel = -1;
		if (names.size() > 0) {
			sel = UIFileSelector::selectFromList(names);
		}

		if (sel < 0 || sel >= mapLength) {
			iprintf("No selection made or invalid index\n");
			closedir(pdir);
			return;
		}

		fSelectNumb = sel;

		// load selected map
		char filePath[512];
		snprintf(filePath, sizeof(filePath), "sdmc:/OpenBLOX/%s", mapFile[fSelectNumb]);
		FILE *fp = fopen(filePath, "r");
		if (fp) {
			*dataModel = parseRBXL(fp);
			fclose(fp);
		} else {
			iprintf("Failed to open map: %s\n", filePath);
		}

		// Load character model
		char filePathChar[512];
		snprintf(filePathChar, sizeof(filePathChar), "sdmc:/OpenBLOX/content/fonts/character.rbxm");
		FILE *fpchar = fopen(filePathChar, "r");
		if (fpchar) {
			*character = parseRBXM(fpchar);
			fclose(fpchar);
		} else {
			printf("Character model not found: %s\n", filePathChar);
		}

		// free allocated names
		for (int i = 0; i < mapLength; ++i) free((void*)mapFile[i]);
		closedir(pdir);
	} else {
		printf("opendir() failure for sdmc:/OpenBLOX; ensure SD card is mounted and folder exists\n");
	}
}
