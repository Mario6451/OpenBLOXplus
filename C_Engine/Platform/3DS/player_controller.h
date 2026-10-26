#pragma once

// Player controller with Roblox-style physics (2007-2011 era)
namespace PlayerController {
	// Initialize player body at given position
	void initPlayer(float x, float y, float z);
	// Update player each frame (called after input is processed)
	void updatePlayer(float dt);
	// Shutdown and cleanup
	void shutdownPlayer();
	// Set current material type (0=SmoothPlastic, 1=Wood, 2=Metal, 3=Ice, etc.)
	void setMaterial(int materialIndex);
}
