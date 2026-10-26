#include "haptic.h"

// 3DS has no vibration hardware. All functions are no-op stubs.

bool Haptic::init() {
    return true;
}

void Haptic::trigger(Strength, int) {}

void Haptic::onDamage(int) {}

void Haptic::onJump() {}

void Haptic::onToolUse() {}

void Haptic::onItemPickup() {}

void Haptic::onDeath() {}

void Haptic::onExplosion() {}

void Haptic::onStep(bool) {}

void Haptic::enable(bool) {}

bool Haptic::isSupported() {
    return false;
}

void Haptic::shutdown() {}