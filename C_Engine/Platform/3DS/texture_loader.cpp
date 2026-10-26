#include "texture_loader.h"
#include "dds_loader.h"
#include "gif_decoder.h"
#include "stb_image.h"
#include "base64.h"
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Texture cache
static std::map<std::string, void*> s_texture_cache;
static std::map<std::string, GIFDecoder::AnimatedTexture*> s_animated_cache;
static uint32_t s_total_vram = 0;

TextureData TextureLoader::loadTextureFromFile(const std::string &path) {
	TextureData res = {0,0,0,nullptr};
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) return res;
	fseek(f,0,SEEK_END);
	long len = ftell(f);
	fseek(f,0,SEEK_SET);
	unsigned char *buf = (unsigned char*)malloc(len);
	if (!buf) { fclose(f); return res; }
	fread(buf,1,len,f);
	fclose(f);
	
	// Check for GIF magic
	if (len >= 6 && (memcmp(buf,"GIF87a",6)==0 || memcmp(buf,"GIF89a",6)==0)) {
		free(buf);
		// Return first frame as static texture for backwards compatibility
		GIFDecoder::AnimatedTexture gif;
		if (gif.loadFromFile(path)) {
			// Render first frame
			gif.setFrame(0);
			gif.renderFrameToTexture(0);
			void* tex = gif.getCurrentTexture();
			if (tex) {
				C3D_Tex* c3d = (C3D_Tex*)tex;
				res.width = gif.getWidth();
				res.height = gif.getHeight();
				res.channels = 4;
				res.pixels = (unsigned char*)malloc(res.width * res.height * 4);
				if (res.pixels) {
					unsigned char* src = (unsigned char*)c3d->data;
					int stride = c3d->width * (c3d->fmt == GPU_RGBA8 ? 4 : 3);
					for (int y = 0; y < res.height; y++) {
						if (c3d->fmt == GPU_RGBA8) {
							memcpy(res.pixels + y*res.width*4, src + y*stride, res.width * 4);
						} else {
							// RGB -> RGBA conversion
							for (int x = 0; x < res.width; x++) {
								memcpy(res.pixels + (y*res.width+x)*4, src + y*stride + x*3, 3);
								res.pixels[(y*res.width+x)*4+3] = 255;
							}
						}
					}
				}
			}
			return res;
		}
	}
	
	res = TextureLoader::loadTextureFromMemory(buf, (int)len);
	free(buf);
	return res;
}

// Load DDS file and decode to TextureData
TextureData TextureLoader::loadDDSFromFile(const std::string &path) {
	TextureData res = {0,0,0,nullptr};
	DDSLoader::DDSData dds = DDSLoader::loadDDSFromFile(path);
	if (!dds.pixels || dds.width == 0) {
		DDSLoader::freeDDS(dds);
		return res;
	}
	res.width = dds.width;
	res.height = dds.height;
	res.channels = dds.channels;
	// DDSLoader already decoded to RGBA; pixels are owned by DDSData
	// We need to copy since DDSLoader::freeDDS will free them
	res.pixels = (unsigned char*)malloc(res.width * res.height * 4);
	if (res.pixels) {
		memcpy(res.pixels, dds.pixels, res.width * res.height * 4);
	}
	DDSLoader::freeDDS(dds);
	return res;
}

TextureData TextureLoader::loadTextureFromMemory(const unsigned char *data, int size) {
	TextureData res = {0,0,0,nullptr};
	if (size < 6) return res;
	// Check for GIF magic
	if (memcmp(data,"GIF87a",6)==0 || memcmp(data,"GIF89a",6)==0) {
		GIFDecoder::AnimatedTexture gif;
		if (gif.loadFromMemory(data, size)) {
			gif.renderFrameToTexture(0);
			void* tex = gif.getCurrentTexture();
			if (tex) {
				C3D_Tex* c3d = (C3D_Tex*)tex;
				res.width = gif.getWidth();
				res.height = gif.getHeight();
				res.channels = 4;
				res.pixels = (unsigned char*)malloc(res.width * res.height * 4);
				if (res.pixels) {
					unsigned char* src = (unsigned char*)c3d->data;
					int stride = c3d->width * 4;
					for (int y = 0; y < res.height; y++)
						memcpy(res.pixels + y*res.width*4, src + y*stride, res.width * 4);
				}
			}
			return res;
		}
	}
	// Use stb_image if available
	int x=0,y=0,comp=0;
	unsigned char *img = stbi_load_from_memory((unsigned char*)data, size, &x, &y, &comp, 4);
	if (!img) return res;
	res.width = x; res.height = y; res.channels = 4; res.pixels = img;
	return res;
}

void TextureLoader::freeTexture(TextureData &t) {
	// All loading paths (stbi_load, malloc for GIF/DDS) use standard malloc/free.
	// stbi_image_free is just an alias for free on default config, but use
	// free() directly to avoid confusion when allocations come from malloc().
	if (t.pixels) free(t.pixels);
	t.pixels = nullptr;
}

// Create and upload a Citro3D texture from TextureData (RGBA32, linear filter)
void* TextureLoader::createC3DTexture(const TextureData &t) {
	return createC3DTextureEx(t, true, false);
}

// Create C3D texture with specific format and filtering
void* TextureLoader::createC3DTextureEx(const TextureData &t, bool linear_filter, bool mipmaps) {
	if (!t.pixels || t.width == 0 || t.height == 0) return nullptr;

	// Power-of-2 check (3DS GPU requires POT for most filtering)
	bool is_pow2 = true;
	int max_dim = 512; // 3DS max texture dimension
	if (t.width > max_dim || t.height > max_dim) {
		is_pow2 = false;
	}
	for (int d = t.width; d > 1; d >>= 1) if (d & 1) is_pow2 = false;
	for (int d = t.height; d > 1; d >>= 1) if (d & 1) is_pow2 = false;

	C3D_Tex *tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
	if (!tex) return nullptr;

	// Choose format: RGBA8 for full alpha, or RGB8 if all opaque
	bool has_alpha = false;
	for (int i = 3; i < t.width * t.height * 4; i += 4) {
		if (t.pixels[i] < 255) { has_alpha = true; break; }
	}

	C3D_TexInit(tex, t.width, t.height, has_alpha ? GPU_RGBA8 : GPU_RGB8);

	// Upload pixel data (3DS expects bottom-up)
	unsigned char *dst = (unsigned char*)tex->data;
	int dst_stride = t.width * (has_alpha ? 4 : 3);
	int src_stride = t.width * 4;

	for (int y = 0; y < t.height; y++) {
		const unsigned char *src_row = t.pixels + (t.height - 1 - y) * src_stride;
		unsigned char *dst_row = dst + y * dst_stride;
		if (has_alpha) {
			// RGBA -> RGBA (swap RB for BGRA)
			for (int x = 0; x < t.width; x++) {
				dst_row[x*4+0] = src_row[x*4+2]; // B = R
				dst_row[x*4+1] = src_row[x*4+1]; // G = G
				dst_row[x*4+2] = src_row[x*4+0]; // R = B
				dst_row[x*4+3] = src_row[x*4+3]; // A = A
			}
		} else {
			// RGBA -> RGB (swap RB for BGR)
			for (int x = 0; x < t.width; x++) {
				dst_row[x*3+0] = src_row[x*4+2]; // B = R
				dst_row[x*3+1] = src_row[x*4+1]; // G = G
				dst_row[x*3+2] = src_row[x*4+0]; // R = B
			}
		}
	}

	// Set filtering
	if (is_pow2) {
		C3D_TexSetFilter(tex, linear_filter ? GPU_LINEAR : GPU_NEAREST,
		                    linear_filter ? GPU_LINEAR : GPU_NEAREST);
	} else {
		// Non-POT can't use mipmaps or linear filtering
		C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);
	}

	// Set wrapping
	C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);

	// Generate mipmaps if requested
	if (mipmaps && is_pow2) {
		C3D_TexInitMipmap(tex, t.width, t.height, has_alpha ? GPU_RGBA8 : GPU_RGB8);
	}

	// Track VRAM usage
	s_total_vram += t.width * t.height * (has_alpha ? 4 : 3);

	return (void*)tex;
}

// Get cached texture (loads if not present)
void* TextureLoader::getCachedTexture(const std::string &path) {
	auto it = s_texture_cache.find(path);
	if (it != s_texture_cache.end()) {
		return it->second;
	}

	TextureData tex = {0, 0, 0, nullptr};
	std::string lower = path;
	for (auto &c : lower) c = tolower(c);

	// Determine loader by extension
	if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".dds") {
		tex = loadDDSFromFile(path);
	} else {
		tex = loadTextureFromFile(path);
	}

	if (!tex.pixels || tex.width == 0) {
		freeTexture(tex);
		return nullptr;
	}

	void *gpu_tex = createC3DTexture(tex);
	if (gpu_tex) {
		s_texture_cache[path] = gpu_tex;
	}
	freeTexture(tex);
	return gpu_tex;
}

// Clear the texture cache
void TextureLoader::clearCache() {
	for (auto &kv : s_texture_cache) {
		if (kv.second) {
			C3D_TexDelete((C3D_Tex*)kv.second);
			free(kv.second);
		}
	}
	s_texture_cache.clear();
	s_total_vram = 0;
}

// Get total cached texture count
int TextureLoader::getCacheSize() {
	return (int)s_texture_cache.size();
}

// Get approximate VRAM used by cached textures
uint32_t TextureLoader::getCacheVRAMSize() {
	return s_total_vram;
}

// Delete a specific cached texture
void TextureLoader::evictCachedTexture(const std::string &path) {
	auto it = s_texture_cache.find(path);
	if (it != s_texture_cache.end()) {
		if (it->second) {
			C3D_TexDelete((C3D_Tex*)it->second);
			free(it->second);
		}
		s_texture_cache.erase(it);
	}
}

// Shutdown and free all cached textures
void TextureLoader::shutdown() {
	clearCache();
	// Free animated textures
	for (auto &kv : s_animated_cache) {
		if (kv.second) {
			kv.second->clear();
			delete kv.second;
		}
	}
	s_animated_cache.clear();
}
