#pragma once

#include <string>
#include <vector>
#include <map>
#include <3ds.h>

struct TextureData {
	int width;
	int height;
	int channels;
	unsigned char *pixels; // row-major RGBA or RGB
	bool fromStb;         // true if pixels were allocated by stb_image
};

namespace TextureLoader {
	// Load texture from SD card path (relative to sdmc:/OpenBLOX or absolute)
	TextureData loadTextureFromFile(const std::string &path);
	// Load DDS texture from file
	TextureData loadDDSFromFile(const std::string &path);
	// Load texture from memory buffer (PNG/JPEG bytes)
	TextureData loadTextureFromMemory(const unsigned char *data, int size);
	// Free texture pixels
	void freeTexture(TextureData &t);

	// Create and upload a Citro3D texture from TextureData. Returns nullptr on failure.
	void* createC3DTexture(const TextureData &t);

	// Create C3D texture with specific format and filtering
	void* createC3DTextureEx(const TextureData &t, bool linear_filter, bool mipmaps);

	// Texture cache - automatically caches loaded textures
	void* getCachedTexture(const std::string &path);

	// Clear the texture cache
	void clearCache();

	// Get total cached texture count
	int getCacheSize();

	// Get approximate VRAM used by cached textures
	uint32_t getCacheVRAMSize();

	// Delete a specific cached texture
	void evictCachedTexture(const std::string &path);

	// Shutdown and free all cached textures
	void shutdown();
}
