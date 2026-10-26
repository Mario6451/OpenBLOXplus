#include "ui_file_selector.h"
#include "citro_renderer.h"
#include <3ds.h>
#include <vector>
#include <string>
#include <stdio.h>

int UIFileSelector::selectFromList(const std::vector<std::string> &names) {
	int mapLength = (int)names.size();
	if (mapLength==0) return -1;
	int fSelectNumb = 0;
	int topIndex = 0;
	bool selected = false;
	const int showCount = 10;
	while (!selected) {
		consoleClear();
		iprintf("-- OpenBLOX Map Select --\n");
		for (int i=0;i<showCount;i++){
			int idx = topIndex + i;
			if (idx>=mapLength) break;
			if (idx==fSelectNumb) iprintf("-> %s\n", names[idx].c_str()); else iprintf("   %s\n", names[idx].c_str());
		}
		// draw preview placeholder on top screen
		CitroRenderer::startFrame();
		// draw a simple colored quad as placeholder
		CitroRenderer::beginTriangles();
		CitroRenderer::setColor(0.2f,0.6f,0.9f);
		CitroRenderer::vertex3f(-0.5f, -0.5f, 0.0f);
		CitroRenderer::vertex3f( 0.5f, -0.5f, 0.0f);
		CitroRenderer::vertex3f( 0.5f,  0.5f, 0.0f);
		CitroRenderer::vertex3f(-0.5f, -0.5f, 0.0f);
		CitroRenderer::vertex3f( 0.5f,  0.5f, 0.0f);
		CitroRenderer::vertex3f(-0.5f,  0.5f, 0.0f);
		CitroRenderer::endPrimitives();
		CitroRenderer::endFrame();

		// input
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_DOWN) { fSelectNumb++; if (fSelectNumb>=mapLength) fSelectNumb=0; if (fSelectNumb>=topIndex+showCount) topIndex = fSelectNumb - showCount + 1; }
		if (kDown & KEY_UP) { fSelectNumb--; if (fSelectNumb<0) fSelectNumb=mapLength-1; if (fSelectNumb<topIndex) topIndex = fSelectNumb; }
		if (kDown & KEY_A) { selected = true; break; }
		touchPosition touch;
		if (hidKeysHeld() & KEY_TOUCH) {
			touchRead(&touch);
			int line = touch.py / 16;
			int idx = topIndex + line - 1;
			if (idx>=0 && idx<mapLength) { fSelectNumb = idx; selected = true; break; }
		}
		svcSleepThread(10000000ULL);
	}
	return fSelectNumb;
}
