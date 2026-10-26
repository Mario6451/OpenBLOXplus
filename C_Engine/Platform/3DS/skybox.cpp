#include "skybox.h"
#include "texture_loader.h"
#include <string>
#include <vector>
#include <stdio.h>

static TextureData skyTex[6];
static bool skyboxLoaded = false;
static const char* skyboxFaceNames[] = {
    "null_plainsky512_ft.jpg",  // Front
    "null_plainsky512_bk.jpg",  // Back
    "null_plainsky512_lf.jpg",  // Left
    "null_plainsky512_rt.jpg",  // Right
    "null_plainsky512_up.jpg",  // Up
    "null_plainsky512_dn.jpg"   // Down
};

bool Skybox::loadSkyboxFromFolder(const std::string &folderPath) {
    skyboxLoaded = false;
    
    // Try to load all 6 cube faces
    int loadedCount = 0;
    for (int i = 0; i < 6; i++) {
        std::string path = folderPath + "/" + skyboxFaceNames[i];
        skyTex[i] = TextureLoader::loadTextureFromFile(path);
        if (skyTex[i].pixels != nullptr) {
            loadedCount++;
        }
    }
    
    // Skybox is valid if at least the front face loaded
    skyboxLoaded = (loadedCount >= 1);
    return skyboxLoaded;
}

void Skybox::bindSkybox() {
    if (!skyboxLoaded) return;
    // Skybox textures are already loaded; renderer will bind them
    // during skybox rendering phase using skyTex[0..5]
}
