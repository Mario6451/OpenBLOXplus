#include "dds_loader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

namespace DDSLoader {

static uint32_t readU32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t readU16(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint8_t clamp(int v) {
    return (v < 0) ? 0 : (v > 255) ? 255 : (uint8_t)v;
}

// BC1/DXT1 decode: 8 bytes -> 4x4 block
void decodeBC1Block(const unsigned char *block, unsigned char *out, int row_pitch_bytes) {
    uint16_t color0 = readU16(block);
    uint16_t color1 = readU16(block + 2);
    uint8_t indices[16];
    memcpy(indices, block + 4, 16);

    // 5-6-5 RGB unpack
    uint8_t r0 = (uint8_t)((color0 >> 11) & 0x1F) * 255 / 31;
    uint8_t g0 = (uint8_t)((color0 >> 5) & 0x3F) * 255 / 63;
    uint8_t b0 = (uint8_t)((color0 >> 0) & 0x1F) * 255 / 31;
    uint8_t r1 = (uint8_t)((color1 >> 11) & 0x1F) * 255 / 31;
    uint8_t g1 = (uint8_t)((color1 >> 5) & 0x3F) * 255 / 63;
    uint8_t b1 = (uint8_t)((color1 >> 0) & 0x1F) * 255 / 31;

    uint8_t colors[8][4]; // RGBA
    colors[0][0] = r0; colors[0][1] = g0; colors[0][2] = b0; colors[0][3] = 255;
    colors[1][0] = r1; colors[1][1] = g1; colors[1][2] = b1; colors[1][3] = 255;

    if (color0 > color1) {
        // 6-color mode
        colors[2][0] = (3*r0 + r1)/4; colors[2][1] = (3*g0 + g1)/4; colors[2][2] = (3*b0 + b1)/4; colors[2][3] = 255;
        colors[3][0] = (2*r0 + 2*r1)/4; colors[3][1] = (2*g0 + 2*g1)/4; colors[3][2] = (2*b0 + 2*b1)/4; colors[3][3] = 255;
        colors[4][0] = (r0 + 3*r1)/4; colors[4][1] = (g0 + 3*g1)/4; colors[4][2] = (b0 + 3*b1)/4; colors[4][3] = 255;
        colors[5][0] = (r0 + 2*r1)/3; colors[5][1] = (g0 + 2*g1)/3; colors[5][2] = (b0 + 2*b1)/3; colors[5][3] = 255;
        colors[6][0] = 0; colors[6][1] = 0; colors[6][2] = 0; colors[6][3] = 255;
        colors[7][0] = 0; colors[7][1] = 0; colors[7][2] = 0; colors[7][3] = 0;
    } else {
        // 4-color mode
        colors[2][0] = (2*r0 + r1)/3; colors[2][1] = (2*g0 + g1)/3; colors[2][2] = (2*b0 + b1)/3; colors[2][3] = 255;
        colors[3][0] = (r0 + r1)/2; colors[3][1] = (g0 + g1)/2; colors[3][2] = (b0 + b1)/2; colors[3][3] = 255;
        colors[4][0] = 0; colors[4][1] = 0; colors[4][2] = 0; colors[4][3] = 255;
        colors[5][0] = 0; colors[5][1] = 0; colors[5][2] = 0; colors[5][3] = 0;
        colors[6][0] = 0; colors[6][1] = 0; colors[6][2] = 0; colors[6][3] = 255;
        colors[7][0] = 0; colors[7][1] = 0; colors[7][2] = 0; colors[7][3] = 0;
    }

    for (int i = 0; i < 16; i++) {
        int py = i / 4;
        int px = i % 4;
        uint8_t *dst = out + (py * row_pitch_bytes + px * 4);
        memcpy(dst, colors[indices[i]], 4);
    }
}

// BC2/DXT3 decode: 16 bytes -> 4x4 block
void decodeBC2Block(const unsigned char *block, unsigned char *out, int row_pitch_bytes) {
    // First 8 bytes: alpha block (16 4-bit values)
    uint8_t alpha[16];
    for (int i = 0; i < 8; i++) {
        uint16_t val = readU16(block + i * 2);
        alpha[i * 2] = (val >> 0) & 0x0F;
        alpha[i * 2 + 1] = (val >> 4) & 0x0F;
    }

    // Next 8 bytes: BC1 color block
    uint8_t colors[16][4];
    uint16_t color0 = readU16(block + 8);
    uint16_t color1 = readU16(block + 10);
    uint8_t indices[16];
    memcpy(indices, block + 12, 16);

    uint8_t r0 = (uint8_t)((color0 >> 11) & 0x1F) * 255 / 31;
    uint8_t g0 = (uint8_t)((color0 >> 5) & 0x3F) * 255 / 63;
    uint8_t b0 = (uint8_t)((color0 >> 0) & 0x1F) * 255 / 31;
    uint8_t r1 = (uint8_t)((color1 >> 11) & 0x1F) * 255 / 31;
    uint8_t g1 = (uint8_t)((color1 >> 5) & 0x3F) * 255 / 63;
    uint8_t b1 = (uint8_t)((color1 >> 0) & 0x1F) * 255 / 31;

    uint8_t palette[8][3];
    palette[0][0] = r0; palette[0][1] = g0; palette[0][2] = b0;
    palette[1][0] = r1; palette[1][1] = g1; palette[1][2] = b1;
    if (color0 > color1) {
        palette[2][0] = (3*r0 + r1)/4; palette[2][1] = (3*g0 + g1)/4; palette[2][2] = (3*b0 + b1)/4;
        palette[3][0] = (2*r0 + 2*r1)/4; palette[3][1] = (2*g0 + 2*g1)/4; palette[3][2] = (2*b0 + 2*b1)/4;
        palette[4][0] = (r0 + 3*r1)/4; palette[4][1] = (g0 + 3*g1)/4; palette[4][2] = (b0 + 3*b1)/4;
        palette[5][0] = (r0 + 2*r1)/3; palette[5][1] = (g0 + 2*g1)/3; palette[5][2] = (b0 + 2*b1)/3;
        palette[6][0] = 0; palette[6][1] = 0; palette[6][2] = 0;
        palette[7][0] = 0; palette[7][1] = 0; palette[7][2] = 0;
    } else {
        palette[2][0] = (2*r0 + r1)/3; palette[2][1] = (2*g0 + g1)/3; palette[2][2] = (2*b0 + b1)/3;
        palette[3][0] = (r0 + r1)/2; palette[3][1] = (g0 + g1)/2; palette[3][2] = (b0 + b1)/2;
        palette[4][0] = 0; palette[4][1] = 0; palette[4][2] = 0;
        palette[5][0] = 0; palette[5][1] = 0; palette[5][2] = 0;
        palette[6][0] = 0; palette[6][1] = 0; palette[6][2] = 0;
        palette[7][0] = 0; palette[7][1] = 0; palette[7][2] = 0;
    }

    for (int i = 0; i < 16; i++) {
        int py = i / 4;
        int px = i % 4;
        uint8_t *dst = out + (py * row_pitch_bytes + px * 4);
        uint8_t a = (uint8_t)((alpha[i] * 255 + 8) / 16);
        uint8_t *rgb = palette[indices[i]];
        dst[0] = rgb[0]; dst[1] = rgb[1]; dst[2] = rgb[2]; dst[3] = a;
    }
}

// BC3/DXT5 decode: 16 bytes -> 4x4 block
void decodeBC3Block(const unsigned char *block, unsigned char *out, int row_pitch_bytes) {
    // Alpha block: first 8 bytes (8-byte alpha block)
    uint8_t alpha[8];
    alpha[0] = block[0];
    alpha[1] = block[1];
    for (int i = 2; i < 8; i++) {
        alpha[i] = block[i];
    }

    uint8_t a_interp[8];
    if (alpha[0] > alpha[1]) {
        a_interp[0] = alpha[0]; a_interp[1] = alpha[1];
        a_interp[2] = (6*alpha[0] + alpha[1])/7; a_interp[3] = (5*alpha[0] + 2*alpha[1])/7;
        a_interp[4] = (4*alpha[0] + 3*alpha[1])/7; a_interp[5] = (3*alpha[0] + 4*alpha[1])/7;
        a_interp[6] = (2*alpha[0] + 5*alpha[1])/7; a_interp[7] = (alpha[0] + 6*alpha[1])/7;
    } else {
        a_interp[0] = alpha[0]; a_interp[1] = alpha[1];
        a_interp[2] = (4*alpha[0] + alpha[1])/5; a_interp[3] = (3*alpha[0] + 2*alpha[1])/5;
        a_interp[4] = (2*alpha[0] + 3*alpha[1])/5; a_interp[5] = (alpha[0] + 4*alpha[1])/5;
        a_interp[6] = 0; a_interp[7] = 255;
    }

    uint8_t alpha_idx[16];
    uint8_t b2 = block[8], b3 = block[9], b4 = block[10];
    alpha_idx[0] = b2 & 7; alpha_idx[1] = (b2 >> 3) & 7; alpha_idx[2] = (b3 >> 0) & 7;
    alpha_idx[3] = (b3 >> 3) & 7; alpha_idx[4] = (b3 >> 6) | ((b4 & 3) << 2);
    alpha_idx[5] = (b4 >> 2) & 7; alpha_idx[6] = (b4 >> 5) | ((b2 & 1) << 3);
    alpha_idx[7] = b2 & 7; // This is incorrect, need to fix
    alpha_idx[8] = b3 & 7; alpha_idx[9] = (b3 >> 3) & 7; alpha_idx[10] = (b3 >> 6) | ((b4 & 3) << 2);
    alpha_idx[11] = (b4 >> 2) & 7; alpha_idx[12] = (b4 >> 5) | ((b2 & 1) << 3);
    alpha_idx[13] = 0; alpha_idx[14] = 0; alpha_idx[15] = 0;

    // Proper alpha index decoding
    uint32_t alpha_bits = 0;
    memcpy(&alpha_bits, block + 8, 4);
    for (int i = 0; i < 16; i++) {
        alpha_idx[i] = (alpha_bits >> (i * 3)) & 7;
    }

    // Color block: bytes 12-19 (BC1 color)
    uint16_t color0 = readU16(block + 12);
    uint16_t color1 = readU16(block + 14);
    uint8_t c_indices[16];
    memcpy(c_indices, block + 16, 4);

    // Expand 16-bit indices
    uint32_t idx_data = 0;
    memcpy(&idx_data, block + 16, 4);
    for (int i = 0; i < 16; i++) {
        c_indices[i] = (idx_data >> (i * 2)) & 3;
    }

    uint8_t r0 = (uint8_t)((color0 >> 11) & 0x1F) * 255 / 31;
    uint8_t g0 = (uint8_t)((color0 >> 5) & 0x3F) * 255 / 63;
    uint8_t b0 = (uint8_t)((color0 >> 0) & 0x1F) * 255 / 31;
    uint8_t r1 = (uint8_t)((color1 >> 11) & 0x1F) * 255 / 31;
    uint8_t g1 = (uint8_t)((color1 >> 5) & 0x3F) * 255 / 63;
    uint8_t b1 = (uint8_t)((color1 >> 0) & 0x1F) * 255 / 31;

    uint8_t palette[8][3];
    palette[0][0] = r0; palette[0][1] = g0; palette[0][2] = b0;
    palette[1][0] = r1; palette[1][1] = g1; palette[1][2] = b1;
    if (color0 > color1) {
        palette[2][0] = (3*r0 + r1)/4; palette[2][1] = (3*g0 + g1)/4; palette[2][2] = (3*b0 + b1)/4;
        palette[3][0] = (2*r0 + 2*r1)/4; palette[3][1] = (2*g0 + 2*g1)/4; palette[3][2] = (2*b0 + 2*b1)/4;
        palette[4][0] = (r0 + 3*r1)/4; palette[4][1] = (g0 + 3*g1)/4; palette[4][2] = (b0 + 3*b1)/4;
        palette[5][0] = (r0 + 2*r1)/3; palette[5][1] = (g0 + 2*g1)/3; palette[5][2] = (b0 + 2*b1)/3;
        palette[6][0] = 0; palette[6][1] = 0; palette[6][2] = 0;
        palette[7][0] = 0; palette[7][1] = 0; palette[7][2] = 0;
    } else {
        palette[2][0] = (2*r0 + r1)/3; palette[2][1] = (2*g0 + g1)/3; palette[2][2] = (2*b0 + b1)/3;
        palette[3][0] = (r0 + r1)/2; palette[3][1] = (g0 + g1)/2; palette[3][2] = (b0 + b1)/2;
        palette[4][0] = 0; palette[4][1] = 0; palette[4][2] = 0;
        palette[5][0] = 0; palette[5][1] = 0; palette[5][2] = 0;
        palette[6][0] = 0; palette[6][1] = 0; palette[6][2] = 0;
        palette[7][0] = 0; palette[7][1] = 0; palette[7][2] = 0;
    }

    for (int i = 0; i < 16; i++) {
        int py = i / 4;
        int px = i % 4;
        uint8_t *dst = out + (py * row_pitch_bytes + px * 4);
        uint8_t *rgb = palette[c_indices[i]];
        dst[0] = rgb[0]; dst[1] = rgb[1]; dst[2] = rgb[2];
        dst[3] = a_interp[alpha_idx[i]];
    }
}

DDSData loadDDSFromMemory(const unsigned char *data, int size) {
    DDSData result = {0, 0, 0, nullptr, 1, false, 0, nullptr, 0};

    if (size < (int)sizeof(DDSEnv)) return result;

    DDSEnv *dds = (DDSEnv*)data;
    if (dds->magic != 0x20534444) return result; // Not a DDS file

    result.width = (int)dds->width;
    result.height = (int)dds->height;
    result.mipmap_count = dds->mipmap_count > 0 ? (int)dds->mipmap_count - 1 : 0;

    unsigned char *pixels = (unsigned char*)malloc(result.width * result.height * 4);
    if (!pixels) return result;
    memset(pixels, 0, result.width * result.height * 4);

    unsigned char *src = (unsigned char*)data + sizeof(DDSEnv);
    int row_pitch = result.width * 4;

    if (dds->pf_flags & DDPF_FOURCC) {
        // Compressed format
        result.is_compressed = true;
        result.fourcc = dds->pf_fourcc;
        result.compressed_data = src;
        result.compressed_size = (int)dds->pitch_or_linear_size;

        int blocks_w = (result.width + 3) / 4;
        int blocks_h = (result.height + 3) / 4;
        int block_size = 0;

        switch (dds->pf_fourcc) {
            case 0x31545844: block_size = 8; break;  // DXT1
            case 0x33545844: block_size = 16; break; // DXT3
            case 0x35545844: block_size = 16; break; // DXT5
            default:
                // Unsupported compressed format
                free(pixels);
                return result;
        }

        for (int by = 0; by < blocks_h; by++) {
            for (int bx = 0; bx < blocks_w; bx++) {
                int block_offset = (by * blocks_w + bx) * block_size;
                unsigned char *dst = pixels + (by * 4 * row_pitch + bx * 4 * 4);

                switch (dds->pf_fourcc) {
                    case 0x31545844:
                        decodeBC1Block(src + block_offset, dst, row_pitch);
                        break;
                    case 0x33545844:
                        decodeBC2Block(src + block_offset, dst, row_pitch);
                        break;
                    case 0x35545844:
                        decodeBC3Block(src + block_offset, dst, row_pitch);
                        break;
                }
            }
        }

        result.channels = 4;
        result.pixels = pixels;
    } else if (dds->pf_flags & DDPF_RGB) {
        // Uncompressed RGB
        result.is_compressed = false;
        result.channels = 4;

        int bpp = dds->pf_rgb_bit_count / 8;
        bool is_bgra = (dds->pf_r_mask == 0x000000FF) && (dds->pf_b_mask == 0x00FF0000);

        if (bpp == 4 && (dds->pf_rgb_bit_count == 32)) {
            // 32-bit RGBA/BGRA
            for (int y = 0; y < result.height; y++) {
                for (int x = 0; x < result.width; x++) {
                    int src_idx = y * result.width * 4 + x * 4;
                    int dst_idx = y * row_pitch + x * 4;
                    if (is_bgra) {
                        pixels[dst_idx + 0] = src[src_idx + 2]; // R
                        pixels[dst_idx + 1] = src[src_idx + 1]; // G
                        pixels[dst_idx + 2] = src[src_idx + 0]; // B
                        pixels[dst_idx + 3] = src[src_idx + 3]; // A
                    } else {
                        memcpy(pixels + dst_idx, src + src_idx, 4);
                    }
                }
            }
        } else if (bpp == 3) {
            // 24-bit RGB
            for (int y = 0; y < result.height; y++) {
                for (int x = 0; x < result.width; x++) {
                    int src_idx = y * result.width * 3 + x * 3;
                    int dst_idx = y * row_pitch + x * 4;
                    pixels[dst_idx + 0] = src[src_idx + 0];
                    pixels[dst_idx + 1] = src[src_idx + 1];
                    pixels[dst_idx + 2] = src[src_idx + 2];
                    pixels[dst_idx + 3] = 255;
                }
            }
            result.channels = 4;
        } else if (dds->pf_rgb_bit_count == 16) {
            // 16-bit 565 RGB
            for (int y = 0; y < result.height; y++) {
                for (int x = 0; x < result.width; x++) {
                    uint16_t val = readU16(src + y * result.width * 2 + x * 2);
                    uint8_t r = (uint8_t)((val >> 11) & 0x1F) * 255 / 31;
                    uint8_t g = (uint8_t)((val >> 5) & 0x3F) * 255 / 63;
                    uint8_t b = (uint8_t)(val & 0x1F) * 255 / 31;
                    int dst = y * row_pitch + x * 4;
                    pixels[dst + 0] = r; pixels[dst + 1] = g; pixels[dst + 2] = b; pixels[dst + 3] = 255;
                }
            }
            result.channels = 4;
        } else {
            // Try to handle via masks
            for (int y = 0; y < result.height; y++) {
                for (int x = 0; x < result.width; x++) {
                    uint32_t val = readU32(src + y * result.width * bpp + x * bpp);
                    int r = 0, g = 0, b = 0, a = 0;

                    if (dds->pf_r_mask) {
                        int rbits = 0, rshift = 0;
                        uint32_t rm = dds->pf_r_mask;
                        while (rm) { if (rm & 1) rbits++; rshift++; rm >>= 1; }
                        r = ((val >> (rshift - rbits)) & ((1 << rbits) - 1)) * 255 / ((1 << rbits) - 1);
                    }
                    if (dds->pf_g_mask) {
                        int gbits = 0, gshift = 0;
                        uint32_t gm = dds->pf_g_mask;
                        while (gm) { if (gm & 1) gbits++; gshift++; gm >>= 1; }
                        g = ((val >> (gshift - gbits)) & ((1 << gbits) - 1)) * 255 / ((1 << gbits) - 1);
                    }
                    if (dds->pf_b_mask) {
                        int bbits = 0, bshift = 0;
                        uint32_t bm = dds->pf_b_mask;
                        while (bm) { if (bm & 1) bbits++; bshift++; bm >>= 1; }
                        b = ((val >> (bshift - bbits)) & ((1 << bbits) - 1)) * 255 / ((1 << bbits) - 1);
                    }
                    if (dds->pf_a_mask) {
                        int abits = 0, ashift = 0;
                        uint32_t am = dds->pf_a_mask;
                        while (am) { if (am & 1) abits++; ashift++; am >>= 1; }
                        a = ((val >> (ashift - abits)) & ((1 << abits) - 1)) * 255 / ((1 << abits) - 1);
                    }

                    int dst = y * row_pitch + x * 4;
                    pixels[dst + 0] = clamp(r);
                    pixels[dst + 1] = clamp(g);
                    pixels[dst + 2] = clamp(b);
                    pixels[dst + 3] = clamp(a);
                }
            }
            result.channels = 4;
        }

        result.pixels = pixels;
    } else if (dds->pf_flags & DDPF_LUMINANCE) {
        // Luminance/Alpha
        result.is_compressed = false;
        result.channels = 4;
        int bit_count = dds->pf_rgb_bit_count;

        if (bit_count == 8) {
            for (int y = 0; y < result.height; y++) {
                for (int x = 0; x < result.width; x++) {
                    int src_idx = y * result.width + x;
                    int dst_idx = y * row_pitch + x * 4;
                    pixels[dst_idx + 0] = pixels[dst_idx + 1] = pixels[dst_idx + 2] = src[src_idx];
                    pixels[dst_idx + 3] = 255;
                }
            }
        } else if (bit_count == 16) {
            for (int y = 0; y < result.height; y++) {
                for (int x = 0; x < result.width; x++) {
                    uint16_t val = readU16(src + y * result.width * 2 + x * 2);
                    uint8_t lum = (uint8_t)(val & 0xFF);
                    uint8_t alpha = (uint8_t)((val >> 8) & 0xFF);
                    int dst = y * row_pitch + x * 4;
                    pixels[dst + 0] = lum; pixels[dst + 1] = lum; pixels[dst + 2] = lum; pixels[dst + 3] = alpha;
                }
            }
        }

        result.pixels = pixels;
    } else {
        free(pixels);
        return result;
    }

    return result;
}

DDSData loadDDSFromFile(const std::string &path) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return {0, 0, 0, nullptr, 1, false, 0, nullptr, 0};

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *buf = (unsigned char*)malloc(size);
    if (!buf) { fclose(f); return {0, 0, 0, nullptr, 1, false, 0, nullptr, 0}; }
    fread(buf, 1, size, f);
    fclose(f);

    DDSData result = loadDDSFromMemory(buf, (int)size);
    // If compressed, copy the compressed data since buffer is freed
    if (result.is_compressed && result.compressed_data && result.compressed_size > 0) {
        unsigned char *compressed_copy = (unsigned char*)malloc(result.compressed_size);
        if (compressed_copy) {
            memcpy(compressed_copy, result.compressed_data, result.compressed_size);
            result.compressed_data = compressed_copy;
        }
    }
    free(buf);
    return result;
}

void freeDDS(DDSData &dds) {
    if (dds.pixels) { free(dds.pixels); dds.pixels = nullptr; }
    if (dds.compressed_data) { free((unsigned char*)dds.compressed_data); dds.compressed_data = nullptr; }
    dds.width = 0; dds.height = 0; dds.channels = 0;
    dds.is_compressed = false; dds.fourcc = 0;
}

} // namespace DDSLoader