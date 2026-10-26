#pragma once
#include <3ds.h>

namespace Haptic {
    enum class VibrationType {
        None,
        Click,          // Short tap
        Buzz,           // Longer vibration
        Heavy,          // Strong impact
        Warning         // Warning pattern
    };

    void init();

    // Trigger vibration
    void vibrate(VibrationType type);

    // Vibrate for custom duration
    void vibrateFor(int milliseconds);

    // Is currently vibrating
    bool isVibrating();

    // Stop vibration
    void stop();

    // Is rumble supported (RumbleManager peripheral)
    bool isSupported();

    void shutdown();
}