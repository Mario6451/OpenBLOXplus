#pragma once

#include <stdint.h>
#include <string>

// DDS file header structures
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;              // 'DDS ' (0x20534444)
    uint32_t size;               // 124
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitch_or_linear_size;
    uint32_t depth;              // only used for volume textures
    uint32_t mipmap_count;
    uint32_t reserved1[11];
    // DDPIXELFORMAT
    uint32_t pf_size;
    uint32_t pf_flags;
    uint32_t pf_fourcc;          // DXT1, DXT3, DXT5, etc.
    uint32_t pf_rgb_bit_count;
    uint32_t pf_r_mask;
    uint32_t pf_g_mask;
    uint32_t pf_b_mask;
    uint32_t pf_a_mask;
    // DDSURFACEDESC2
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
} DDSEnv;
#pragma pack(pop)

// DDS pixel format flags
#define DDS_CAPS_MIPMAP       0x400000
#define DDS_FLAGS_MIPMAP      0x00400000
#define DDS_HEIGHT            0x00000002
#define DDS_WIDTH             0x00000004
#define DDS_PITCH             0x00000008
#define DDS_LINEARSIZE        0x00080000
#define DDS_PIXELFORMAT       0x1000
#define DDS_CAPS_VOLUME       0x00200000
#define DDS_SURFACE_FLAGS     0x00000001

// Pixel format flags
#define DDPF_FOURCC           0x00000004
#define DDPF_RGB              0x00000040
#define DDPF_LUMINANCE        0x00020000
#define DDPF_ALPHA            0x00000002

namespace DDSLoader {

    // Decoded DDS texture data
    struct DDSData {
        int width;
        int height;
        int channels;             // 3 or 4
        unsigned char *pixels;    // row-major RGBA (or RGB)
        int mipmap_count;
        // If compressed, stores original compressed data for GPU BC upload
        bool is_compressed;
        uint32_t fourcc;          // 0 if uncompressed
        const unsigned char *compressed_data;
        int compressed_size;
    };

    // Load and decode a DDS file from path
    DDSData loadDDSFromFile(const std::string &path);

    // Load DDS from memory buffer
    DDSData loadDDSFromMemory(const unsigned char *data, int size);

    // Free DDS pixel data
    void freeDDS(DDSData &dds);

    // Decode BC1/DXT1 compressed block (6x6 -> 4x4)
    void decodeBC1Block(const unsigned char *block, unsigned char *out, int row_pitch_bytes);

    // Decode BC2/DXT3 compressed block
    void decodeBC2Block(const unsigned char *block, unsigned char *out, int row_pitch_bytes);

    // Decode BC3/DXT5 compressed block
    void decodeBC3Block(const unsigned char *block, unsigned char *out, int row_pitch_bytes);
}