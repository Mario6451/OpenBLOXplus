#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <3ds.h>
#include "texture_loader.h"

namespace GIFDecoder {

struct GIFFrame {
    unsigned char* pixels;       // RGBA8 pixel data
    int width;
    int height;
    int x;                       // image offset
    int y;
    int delay;                   // in centiseconds (1/100s)
    int disposal;                // 0=none, 1=restore bg, 2=restore prev, 3=undefined
    bool has_transparency;
    uint8_t transparent_index;
};

struct GIFHeader {
    int width;
    int height;
    std::vector<uint8_t> global_color_table;  // RGB triplets
    int global_color_count;
    uint8_t background_color_index;
};

class AnimatedTexture {
public:
    AnimatedTexture();
    ~AnimatedTexture();

    bool loadFromFile(const std::string& path);
    bool loadFromMemory(const unsigned char* data, int size);

    // Animation control
    void update(float deltaTime);
    void play();
    void pause();
    void stop();
    void setFrame(int index);
    void setLoopCount(int count);  // -1 = infinite

    // Get current frame as GPU texture
    void* getCurrentTexture();
    void renderFrameToTexture(int frameIndex);

    int getFrameCount() const { return (int)frames.size(); }
    int getCurrentFrame() const { return currentFrame; }
    int getWidth() const { return header.width; }
    int getHeight() const { return header.height; }
    bool isPlaying() const { return playing; }
    bool isComplete() const;
    void clear();

private:
    bool parseGIF(const unsigned char* data, int size);
    void applyFrameToCanvas(const GIFFrame& frame, int colorCount, const uint8_t* colorTable);

    GIFHeader header;
    std::vector<GIFFrame> frames;
    std::vector<void*> gpuTextures;
    unsigned char* canvas;            // current composited canvas
    unsigned char* prevCanvas;        // previous canvas for disposal=2
    int currentFrame;
    float elapsed;
    bool playing;
    bool loaded;
    int loopCount;
    int currentLoop;
};

}  // namespace GIFDecoder