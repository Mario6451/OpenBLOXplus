#include "gif_decoder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <citro3d.h>

namespace GIFDecoder {

// ============================================================================
// LZW Decoder
// ============================================================================

struct LZWState {
    std::vector<std::vector<uint8_t>> dict;
    int code_size, clear_code, eoi_code, next_code;
    int prev_code;
};

static void lzw_init(LZWState& s, int cs) {
    s.code_size = cs + 1;
    s.clear_code = 1 << cs;
    s.eoi_code = s.clear_code + 1;
    s.next_code = s.eoi_code + 1;
    s.prev_code = -1;
    s.dict.clear();
    for (int i = 0; i < s.clear_code; i++) s.dict.push_back({(uint8_t)i});
}

static bool lzw_decode(const unsigned char* data, int dlen, int cs, std::vector<uint8_t>& out) {
    LZWState s;
    lzw_init(s, cs);
    out.clear();

    int byte_idx = 0, byte_bits = 0;
    int max_bytes = dlen;

    auto read_code = [&]() -> int {
        int code = 0;
        for (int i = 0; i < s.code_size; i++) {
            int bidx = byte_idx + (byte_bits + i) / 8;
            if (bidx >= max_bytes) return -1;
            code |= ((data[bidx] >> ((byte_bits + i) % 8)) & 1) << i;
        }
        byte_bits += s.code_size;
        byte_idx = byte_bits / 8;
        byte_bits %= 8;
        return code;
    };

    while (true) {
        int code = read_code();
        if (code == -1) break;
        if (code == s.clear_code) {
            lzw_init(s, cs);
            continue;
        }
        if (code == s.eoi_code) break;

        std::vector<uint8_t> entry;
        if (code < (int)s.dict.size()) {
            entry = s.dict[code];
        } else if (s.prev_code >= 0 && s.prev_code < (int)s.dict.size()) {
            entry = s.dict[s.prev_code];
            if (!entry.empty()) entry.push_back(entry[0]);
        } else break;

        out.insert(out.end(), entry.begin(), entry.end());

        if (s.prev_code >= 0) {
            std::vector<uint8_t> ne = s.dict[s.prev_code];
            if (!entry.empty()) ne.push_back(entry[0]);
            s.dict.push_back(ne);
            s.next_code++;
            if (s.next_code >= (1 << s.code_size) && s.code_size < 12)
                s.code_size++;
        }
        s.prev_code = code;
    }
    return true;
}

// ============================================================================
// AnimatedTexture Implementation
// ============================================================================

AnimatedTexture::AnimatedTexture()
    : canvas(nullptr), prevCanvas(nullptr), currentFrame(0), elapsed(0),
      playing(false), loaded(false), loopCount(-1), currentLoop(0) {}

AnimatedTexture::~AnimatedTexture() { clear(); }

bool AnimatedTexture::loadFromFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buf = (unsigned char*)malloc(sz);
    if (!buf) { fclose(f); return false; }
    fread(buf, 1, sz, f);
    fclose(f);
    bool r = loadFromMemory(buf, (int)sz);
    free(buf);
    return r;
}

bool AnimatedTexture::loadFromMemory(const unsigned char* data, int size) {
    clear();
    if (size < 13) return false;
    if (memcmp(data, "GIF87a", 6) && memcmp(data, "GIF89a", 6)) return false;

    header.width = data[6] | (data[7] << 8);
    header.height = data[8] | (data[9] << 8);
    uint8_t packed = data[10];
    header.background_color_index = data[13];

    int offset = 13;
    bool has_gct = (packed & 0x80) != 0;
    if (has_gct) {
        int gct_count = 1 << (((packed & 7) + 1) + 1);
        header.global_color_count = gct_count;
        header.global_color_table.resize(gct_count * 3);
        if (offset + gct_count * 3 > size) return false;
        memcpy(header.global_color_table.data(), &data[offset], gct_count * 3);
        offset += gct_count * 3;
    }

    // Parse GIF blocks
    while (offset < size) {
        if (data[offset] == ';') break;

        if (data[offset] == 0x21) { // Extension
            offset++;
            int label = data[offset++];
            if (label == 0xF9) { // Graphic Control Extension
                if (offset + 4 < size) {
                    int bs = data[offset++];
                    uint8_t gce = data[offset];
                    // Create placeholder frame if none exists
                    if (frames.empty()) frames.push_back({});
                    GIFFrame& f = frames.back();
                    f.delay = data[offset + 1] | (data[offset + 2] << 8);
                    f.disposal = (gce >> 2) & 3;
                    f.has_transparency = (gce & 1) != 0;
                    f.transparent_index = data[offset + 3];
                    if (f.delay == 0) f.delay = 10;
                    offset += bs;
                    while (offset < size) { int b = data[offset++]; if (b == 0) break; offset += b; }
                }
            } else {
                while (offset < size) { int b = data[offset++]; if (b == 0) break; offset += b; }
            }
            continue;
        }

        if (data[offset] == 0x2C) { // Image Descriptor
            offset++;
            if (offset + 10 > size) break;
            GIFFrame frame;
            frame.pixels = nullptr;
            frame.x = data[offset] | (data[offset+1] << 8);
            frame.y = data[offset+2] | (data[offset+3] << 8);
            frame.width = data[offset+4] | (data[offset+5] << 8);
            frame.height = data[offset+6] | (data[offset+7] << 8);
            uint8_t ipacked = data[offset+8];
            offset += 9;
            int min_cs = data[offset++];

            // Read LZW data blocks
            std::vector<unsigned char> lzw;
            while (offset < size) {
                int b = data[offset++];
                if (b == 0) break;
                if (offset + b > size) break;
                lzw.insert(lzw.end(), &data[offset], &data[offset + b]);
                offset += b;
            }

            // Decode
            std::vector<uint8_t> pixels;
            lzw_decode(lzw.data(), (int)lzw.size(), min_cs, pixels);

            // Color table
            const uint8_t* ct = nullptr;
            int ct_count = 0;
            if ((ipacked & 0x80) != 0) {
                int lct_count = 1 << (((ipacked & 7) + 1) + 1);
                ct_count = lct_count;
                ct = &data[offset];
                offset += lct_count * 3;
            } else if (!header.global_color_table.empty()) {
                ct = header.global_color_table.data();
                ct_count = header.global_color_count;
            }

            // Convert to RGBA
            if (ct && ct_count > 0 && frame.width > 0 && frame.height > 0) {
                int total = frame.width * frame.height;
                frame.pixels = (unsigned char*)calloc(total * 4, 1);
                if (!frames.empty()) {
                    frame.has_transparency = frames.back().has_transparency;
                    frame.transparent_index = frames.back().transparent_index;
                    frame.disposal = frames.back().disposal;
                    frame.delay = frames.back().delay;
                }
                if (frame.delay == 0) frame.delay = 10;

                for (int i = 0; i < total && i < (int)pixels.size(); i++) {
                    uint8_t idx = pixels[i];
                    int dst = i * 4;
                    if (frame.has_transparency && idx == frame.transparent_index) {
                        frame.pixels[dst + 3] = 0;
                    } else if (idx < ct_count) {
                        frame.pixels[dst+0] = ct[idx*3+0];
                        frame.pixels[dst+1] = ct[idx*3+1];
                        frame.pixels[dst+2] = ct[idx*3+2];
                        frame.pixels[dst+3] = 255;
                    }
                }
            }

            frames.push_back(frame);
            continue;
        }
        offset++;
    }

    if (frames.empty()) return false;

    // Create canvases
    canvas = (unsigned char*)calloc(header.width * header.height * 4, 1);
    prevCanvas = (unsigned char*)calloc(header.width * header.height * 4, 1);

    // Fill with background
    if (has_gct && header.background_color_index < header.global_color_count) {
        int bg = header.background_color_index;
        uint8_t r = header.global_color_table[bg*3];
        uint8_t g = header.global_color_table[bg*3+1];
        uint8_t b = header.global_color_table[bg*3+2];
        for (int i = 0; i < header.width * header.height * 4; i += 4) {
            canvas[i]=r; canvas[i+1]=g; canvas[i+2]=b; canvas[i+3]=255;
            prevCanvas[i]=r; prevCanvas[i+1]=g; prevCanvas[i+2]=b; prevCanvas[i+3]=255;
        }
    }

    loaded = true;
    playing = true;
    return true;
}

void AnimatedTexture::applyFrameToCanvas(const GIFFrame& frame, int /*colorCount*/, const uint8_t* /*colorTable*/) {
    if (!frame.pixels || frame.width == 0 || frame.height == 0) return;
    for (int y = 0; y < frame.height; y++) {
        for (int x = 0; x < frame.width; x++) {
            int src = (y * frame.width + x) * 4;
            int cx = frame.x + x;
            int cy = frame.y + y;
            if (cx < 0 || cy < 0 || cx >= header.width || cy >= header.height) continue;
            int dst = (cy * header.width + cx) * 4;
            uint8_t a = frame.pixels[src + 3];
            if (a == 255) {
                canvas[dst+0]=frame.pixels[src+0];
                canvas[dst+1]=frame.pixels[src+1];
                canvas[dst+2]=frame.pixels[src+2];
                canvas[dst+3]=255;
            } else if (a > 0) {
                float aa = a / 255.0f;
                canvas[dst+0]=(uint8_t)(frame.pixels[src+0]*aa + canvas[dst+0]*(1.0f-aa));
                canvas[dst+1]=(uint8_t)(frame.pixels[src+1]*aa + canvas[dst+1]*(1.0f-aa));
                canvas[dst+2]=(uint8_t)(frame.pixels[src+2]*aa + canvas[dst+2]*(1.0f-aa));
            }
        }
    }
}

void AnimatedTexture::renderFrameToTexture(int frameIndex) {
    if (frameIndex < 0 || frameIndex >= (int)frames.size()) return;
    const GIFFrame& frame = frames[frameIndex];

    // Handle disposal of previous frame
    if (frameIndex > 0) {
        const GIFFrame& prev = frames[frameIndex - 1];
        if (prev.disposal == 1 || prev.disposal == 3) {
            for (int y = 0; y < prev.height; y++) {
                for (int x = 0; x < prev.width; x++) {
                    int cx = prev.x + x;
                    int cy = prev.y + y;
                    if (cx < 0 || cy < 0 || cx >= header.width || cy >= header.height) continue;
                    int dst = (cy * header.width + cx) * 4;
                    if (!header.global_color_table.empty() && header.background_color_index < header.global_color_count) {
                        int bg = header.background_color_index;
                        canvas[dst+0]=header.global_color_table[bg*3];
                        canvas[dst+1]=header.global_color_table[bg*3+1];
                        canvas[dst+2]=header.global_color_table[bg*3+2];
                    }
                }
            }
        } else if (prev.disposal == 2 && prevCanvas) {
            memcpy(canvas, prevCanvas, header.width * header.height * 4);
        }
    }

    // Save state
    if (prevCanvas) memcpy(prevCanvas, canvas, header.width * header.height * 4);

    // Apply frame
    applyFrameToCanvas(frame, 0, nullptr);

    // Upload to GPU
    if (frameIndex < (int)gpuTextures.size() && gpuTextures[frameIndex]) {
        C3D_Tex* tex = (C3D_Tex*)gpuTextures[frameIndex];
        unsigned char* buf = (unsigned char*)malloc(header.width * header.height * 4);
        for (int y = 0; y < header.height; y++) {
            const unsigned char* src = canvas + (header.height-1-y) * header.width * 4;
            unsigned char* dst = buf + y * header.width * 4;
            for (int x = 0; x < header.width; x++) {
                dst[x*4+0]=src[x*4+2]; dst[x*4+1]=src[x*4+1];
                dst[x*4+2]=src[x*4+0]; dst[x*4+3]=src[x*4+3];
            }
        }
        C3D_TexUpload(tex, buf);
        free(buf);
    } else if (frameIndex == (int)gpuTextures.size()) {
        TextureData td;
        td.width=header.width; td.height=header.height; td.channels=4;
        td.pixels=(unsigned char*)malloc(header.width*header.height*4);
        for (int y = 0; y < header.height; y++) {
            const unsigned char* src = canvas + (header.height-1-y)*header.width*4;
            unsigned char* dst = td.pixels + y*header.width*4;
            for (int x = 0; x < header.width; x++) {
                dst[x*4+0]=src[x*4+2]; dst[x*4+1]=src[x*4+1];
                dst[x*4+2]=src[x*4+0]; dst[x*4+3]=src[x*4+3];
            }
        }
        gpuTextures.push_back(TextureLoader::createC3DTextureEx(td, false, false));
        free(td.pixels);
    }
}

void AnimatedTexture::update(float dt) {
    if (!playing || !loaded || frames.empty()) return;
    elapsed += dt;
    float fd = frames[currentFrame].delay * 0.01f;
    if (elapsed >= fd) {
        elapsed = 0;
        currentFrame++;
        if (currentFrame >= (int)frames.size()) {
            currentLoop++;
            if (loopCount >= 0 && currentLoop >= loopCount) {
                playing = false;
                currentFrame = (int)frames.size() - 1;
                return;
            }
            currentFrame = 0;
        }
        renderFrameToTexture(currentFrame);
    }
}

void AnimatedTexture::play() { playing=true; elapsed=0; currentLoop=0; }
void AnimatedTexture::pause() { playing=false; }
void AnimatedTexture::stop() {
    playing=false; currentFrame=0; elapsed=0; currentLoop=0;
    if (loaded) renderFrameToTexture(0);
}
void AnimatedTexture::setFrame(int i) {
    if (i >= 0 && i < (int)frames.size()) { currentFrame=i; elapsed=0; renderFrameToTexture(i); }
}
void AnimatedTexture::setLoopCount(int c) { loopCount=c; }

bool AnimatedTexture::isComplete() const {
    if (!loaded || frames.empty()) return true;
    return loopCount >= 0 && currentLoop >= loopCount && !playing;
}

void* AnimatedTexture::getCurrentTexture() {
    if (!loaded || gpuTextures.empty() || currentFrame >= (int)gpuTextures.size()) return nullptr;
    return gpuTextures[currentFrame];
}

void AnimatedTexture::clear() {
    for (void* t : gpuTextures) { if (t) { C3D_TexDelete((C3D_Tex*)t); free(t); } }
    gpuTextures.clear();
    for (GIFFrame& f : frames) { if (f.pixels) { free(f.pixels); f.pixels=nullptr; } }
    frames.clear();
    if (canvas) { free(canvas); canvas=nullptr; }
    if (prevCanvas) { free(prevCanvas); prevCanvas=nullptr; }
    loaded=false; currentFrame=0; elapsed=0; playing=false; currentLoop=0; loopCount=-1;
}

}  // namespace GIFDecoder