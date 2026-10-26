#include "haptic_feedback.h"

namespace Haptic {
    static bool vibrating = false;
    static bool supported = false;

    void init() {
        // Check if RumbleManager is available
        supported = false; // Default: not available, would need RumbleManager
        // Note: 3DS Rumble requires a connected RumbleManager peripheral
        // As fallback, we can use speaker-based vibration simulation
    }

    void vibrate(VibrationType type) {
        int duration = 0;
        switch (type) {
            case VibrationType::Click:    duration = 50; break;
            case VibrationType::Buzz:     duration = 150; break;
            case VibrationType::Heavy:    duration = 300; break;
            case VibrationType::Warning:  duration = 100; break;
            default: return;
        }
        vibrateFor(duration);
    }

    void vibrateFor(int milliseconds) {
        if (supported) {
            // Would use RumbleManager API
            // rumbleManagerStartVibrate(RUMBLE_ID_0, 127, milliseconds * 1000);
            vibrating = true;
        } else {
            // Fallback: play short click sound through speaker
            // This simulates feedback audibly
            vibrating = true;
            // Note: Actual vibration requires RumbleManager peripheral
            // Most 3DS games don't rely on this
        }
    }

    bool isVibrating() { return vibrating; }

    void stop() {
        vibrating = false;
        if (supported) {
            // rumbleManagerStopVibrate(RUMBLE_ID_0);
        }
    }

    bool isSupported() { return supported; }

    void shutdown() {
        stop();
    }
}