#pragma once

#include <3ds.h>

// Haptic feedback system for 3DS
// Note: Original 3DS doesn't have a motor. New 3DS/2XL have limited vibration.
// This module wraps the available APIs and degrades gracefully.

namespace Haptic {
    enum class Strength {
        Light,    // Subtle tap
        Medium,   // Noticeable buzz
        Heavy     // Strong vibration
    };

    // Initialize haptic system
    bool init();

    // Trigger haptic feedback
    void trigger(Strength strength = Strength::Medium, int durationMs = 100);

    // Trigger haptic for specific events
    void onDamage(int damageAmount);
    void onJump();
    void onToolUse();
    void onItemPickup();
    void onDeath();
    void onExplosion();
    void onStep(bool isEnabled);

    // Enable/disable haptics globally
    void enable(bool enabled);

    // Check if hardware supports haptics
    bool isSupported();

    // Shutdown
    void shutdown();
}