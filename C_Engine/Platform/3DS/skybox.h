#pragma once

#include <string>

namespace Skybox {
	// Load skybox from OpenBLOX content Sky folder; returns true on success
	bool loadSkyboxFromFolder(const std::string &folderPath);
	void bindSkybox();
}
