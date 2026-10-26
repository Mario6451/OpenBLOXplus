#pragma once
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>

// Voice chat system for OpenBLOX 3DS
// Uses 3DS built-in microphone via CTR_AUDIO_CaptureStart/Read
// Push-to-talk via touch hold on VC button
// Audio compressed with simple ADPCM, streamed over multiplayer UDP

namespace VoiceChat {

    enum class VcState {
        Inactive,     // VC not enabled
        Idle,         // Ready but not talking
        Talking       // Currently pushing talk button
    };

    struct VcPacket {
        uint8_t playerId;
        uint16_t sampleCount;
        uint8_t compressedData[512];
        uint16_t compressedSize;
    };

    // Initialize voice chat subsystem
    // Returns true if hardware microphone is available
    bool init();

    // Shutdown and release resources
    void shutdown();

    // Start microphone capture (called when VC enabled)
    bool startCapture();

    // Stop microphone capture (called when leaving VC)
    void stopCapture();

    // Check if player is holding push-to-talk button
    // Returns true if mic is active and player is talking
    bool isTalking();

    // Set push-to-talk state (true = talking, false = idle)
    void setTalking(bool talking);

    // Get current voice chat state
    VcState getState();

    // Capture one frame of audio
    // Returns compressed data and size, or 0 if no data
    uint16_t captureFrame(uint8_t* buffer, uint16_t maxSize);

    // Process incoming voice packet from network
    void processPacket(const VcPacket* packet);

    // Check if a remote player is speaking
    bool isRemotePlayerSpeaking(uint8_t playerId);

    // Get last speaking player ID (0-7)
    uint8_t getLastSpeaker();

    // Check if content preset supports voice chat assets (content2022 only)
    bool hasVcAssets();

    // Get VC asset path (microphone/speaker icon)
    std::string getVcAssetPath(const std::string& assetName);

    // Render speaker bubble above speaking player's head
    void renderSpeakerBubble(int playerX, int playerY);
}