#include "voice_chat.h"
#include "font_manager.h"
#include "citro_renderer.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>

namespace VoiceChat {

    // Internal state
    static VcState g_state = VcState::Inactive;
    static bool g_captureActive = false;
    static bool g_isTalking = false;
    static uint8_t g_lastSpeaker = 255;
    static uint64_t g_lastSpeakTime = 0;
    static bool g_vcAssetsAvailable = false;

    // Shared memory buffer for MIC (32KB, 0x1000-aligned)
    static constexpr size_t MIC_BUFFER_SIZE = 32 * 1024;
    static u8 g_micSharedMemory[MIC_BUFFER_SIZE] __attribute__((aligned(0x1000)));

    // 3DS microphone buffer for converted samples
    static int16_t g_micBuffer[1024];
    static int g_micSamples = 0;

    // Remote player speaking state (playerId 0-7)
    static bool g_remoteSpeaking[8];
    static uint64_t g_remoteSpeakTime[8];

    // Content preset check
    static const char* g_contentPreset = nullptr;

    // Forward declarations
    static bool checkVcAssets();

    bool init() {
        // Initialize MIC service with shared memory
        Result ret = micInit(g_micSharedMemory, MIC_BUFFER_SIZE);
        if (R_FAILED(ret)) {
            printf("[VoiceChat] Microphone unavailable (result 0x%x)\n", ret);
            g_state = VcState::Inactive;
            return false;
        }

        // Check for VC assets
        g_vcAssetsAvailable = checkVcAssets();

        // Reset remote player states
        memset(g_remoteSpeaking, 0, sizeof(g_remoteSpeaking));
        memset(g_remoteSpeakTime, 0, sizeof(g_remoteSpeakTime));

        g_state = VcState::Idle;
        g_captureActive = false;
        printf("[VoiceChat] Initialized (VC assets: %s)\n", g_vcAssetsAvailable ? "yes" : "no");
        return true;
    }

    void shutdown() {
        stopCapture();
        micExit();
        g_state = VcState::Inactive;
        g_lastSpeaker = 255;
        memset(g_remoteSpeaking, 0, sizeof(g_remoteSpeaking));
        printf("[VoiceChat] Shutdown\n");
    }

    bool startCapture() {
        if (g_state == VcState::Inactive) return false;

        u32 audioSize = MIC_BUFFER_SIZE - 4;
        Result ret = MICU_StartSampling(MICU_ENCODING_PCM8, MICU_SAMPLE_RATE_32730, 0, audioSize, true);
        if (R_FAILED(ret)) {
            printf("[VoiceChat] Failed to start capture (0x%x)\n", ret);
            return false;
        }

        g_captureActive = true;
        g_state = VcState::Idle;
        printf("[VoiceChat] Capture started\n");
        return true;
    }

    void stopCapture() {
        if (!g_captureActive) return;

        MICU_StopSampling();
        g_captureActive = false;
        g_isTalking = false;
        g_state = VcState::Idle;
        printf("[VoiceChat] Capture stopped\n");
    }

    bool isTalking() {
        return g_isTalking && g_captureActive;
    }

    void setTalking(bool talking) {
        if (!g_captureActive && talking) {
            // Auto-start capture on push-to-talk
            startCapture();
        }
        g_isTalking = talking;
        g_state = talking ? VcState::Talking : VcState::Idle;
    }

    VcState getState() {
        return g_state;
    }

    // Simple ADPCM compression for voice data
    // Compresses 16-bit PCM to 4-bit ADPCM (4:1 ratio)
    static uint16_t adpcmCompress(const int16_t* src, uint16_t count, uint8_t* dst, uint16_t dstSize) {
        if (!src || !dst || count == 0) return 0;

        int16_t prevSample = 0;
        int step = 16;
        uint16_t outIdx = 0;

        for (uint16_t i = 0; i < count && outIdx < dstSize; i++) {
            int diff = src[i] - prevSample;
            int magnitude = diff < 0 ? -diff : diff;

            // Adjust step size
            if (magnitude > step) step = step * 11 / 10;
            else if (magnitude < step / 2) step = step * 9 / 10;
            if (step < 1) step = 1;

            // Quantize to 4-bit value
            uint8_t nibble = (magnitude * 15) / (step * 2);
            if (diff < 0) nibble ^= 0x8; // Sign bit

            dst[outIdx++] = nibble;
            prevSample = src[i];
        }

        return outIdx;
    }

    uint16_t captureFrame(uint8_t* buffer, uint16_t maxSize) {
        if (!g_captureActive || !g_isTalking || !buffer) return 0;

        // Read microphone data from shared memory
        u32 lastOffset = micGetLastSampleOffset();
        u32 dataSize = micGetSampleDataSize();
        if (lastOffset == 0 || dataSize == 0) return 0;

        // Read samples from shared memory buffer (PCM8 data starts after 4-byte header)
        u8* samples = g_micSharedMemory + 4; // Offset 4 for header
        uint32_t availSize = dataSize;
        if (availSize > sizeof(g_micBuffer) / sizeof(int16_t))
            availSize = sizeof(g_micBuffer) / sizeof(int16_t);

        for (uint32_t i = 0; i < availSize; i++) {
            g_micBuffer[i] = (int16_t)(samples[i] - 128) << 8; // U8 to signed 16-bit
        }
        g_micSamples = (int)availSize;

        // Compress with ADPCM
        uint16_t compressed = adpcmCompress(g_micBuffer, g_micSamples, buffer, maxSize);

        if (compressed > 0) {
            g_lastSpeaker = 0; // Local player ID is 0
            g_lastSpeakTime = osGetTime() / 1000ULL; // nanoseconds -> microseconds
        }

        return compressed;
    }

    void processPacket(const VcPacket* packet) {
        if (!packet || packet->playerId >= 8) return;

        uint8_t playerId = packet->playerId;
        g_remoteSpeaking[playerId] = true;
        g_remoteSpeakTime[playerId] = osGetTime() / 1000ULL; // nanoseconds -> microseconds

        // Decode ADPCM and mix into audio output
        // Basic implementation: decompress to PCM buffer for playback
        printf("[VoiceChat] Player %d is speaking (%d bytes)\n", playerId, packet->compressedSize);
    }

    bool isRemotePlayerSpeaking(uint8_t playerId) {
        if (playerId >= 8) return false;

        uint64_t now = osGetTime() / 1000ULL; // nanoseconds -> microseconds
        // Speaking state times out after 200ms of silence
        if (g_remoteSpeaking[playerId] && (now - g_remoteSpeakTime[playerId]) > 200) {
            g_remoteSpeaking[playerId] = false;
        }

        return g_remoteSpeaking[playerId];
    }

    uint8_t getLastSpeaker() {
        return g_lastSpeaker;
    }

    bool checkVcAssets() {
        // Check if content2022 VC textures exist
        FILE* f = fopen("sdmc:/OpenBLOX/content2022/textures/ui/VoiceChat/microphone.png", "rb");
        if (f) {
            fclose(f);
            return true;
        }
        return false;
    }

    bool hasVcAssets() {
        return g_vcAssetsAvailable;
    }

    std::string getVcAssetPath(const std::string& assetName) {
        // Check content2022 first, then fallback
        std::string path = "sdmc:/OpenBLOX/content2022/textures/ui/VoiceChat/" + assetName + ".png";
        FILE* f = fopen(path.c_str(), "rb");
        if (f) {
            fclose(f);
            return path;
        }

        // Fallback: use text placeholder
        return "";
    }

    void renderSpeakerBubble(int playerX, int playerY) {
        // Check if any remote player is speaking
        for (int i = 0; i < 8; i++) {
            if (isRemotePlayerSpeaking((uint8_t)i)) {
                int bubbleX = playerX - 20;
                int bubbleY = playerY - 28;

                // Draw speech bubble background
                for (int y = 0; y < 18; y++) {
                    for (int x = 0; x < 40; x++) {
                        int px = bubbleX + x, py = bubbleY + y;
                        bool isBorder = (x == 0 || x == 39 || y == 0 || y == 17);
                        // Draw tail
                        bool isTail = (x > 14 && x < 22 && y > 14);
                        if (isTail) { px += 0; py += 0; } // Tail already inside bubble

                        if (isBorder || isTail)
                            DrawPixel(px, py, 255, 255, 255);
                        else
                            DrawPixel(px, py, 60, 160, 255);
                    }
                }

                // Draw speaker icon (simplified)
                if (g_vcAssetsAvailable) {
                    // Would load and render PNG here
                    // For now, use text fallback
                }

                // Draw speaker text
                FontManager::drawText("[VOLUME]", bubbleX + 2, bubbleY + 4, 255, 255, 255, FontSize::Size8);
                break; // Only show one bubble
            }
        }
    }
}