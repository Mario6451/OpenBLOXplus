#pragma once
#include <string>
#include <vector>
#include <3ds.h>

namespace Replay {
    struct ReplayFrame {
        u64 timestamp;
        u32 buttons;       // Held buttons
        float analogX;     // Circlepad X (-1.0 to 1.0)
        float analogY;     // Circlepad Y (-1.0 to 1.0)
    };

    void init();

    // Start recording
    void startRecording();

    // Stop recording and save
    bool stopRecording(const std::string& name);

    // Is currently recording
    bool isRecording();

    // Load replay for playback
    bool loadReplay(const std::string& name);

    // Start playing back
    void startPlayback();

    // Stop playback
    void stopPlayback();

    // Is currently playing back
    bool isPlaying();

    // Get current frame during playback
    ReplayFrame getCurrentFrame();

    // Get replay duration in seconds
    float getDuration();

    // List saved replays
    std::vector<std::string> listReplays();

    // Delete replay by name
    bool deleteReplay(const std::string& name);

    void shutdown();
}