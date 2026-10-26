#pragma once
#include <vector>
#include <3ds.h>

// Simple software rasterizer for 3DS top screen (400x240)
namespace Soft3D {
	void init();
	void shutdown();
	void startFrame();
	void endFrame();

	void clearColor(uint8_t r, uint8_t g, uint8_t b);

	void pushMatrix();
	void popMatrix();
	void loadIdentity();
	void translatef(float x, float y, float z);
	void rotatef(float angleDeg, float x, float y, float z);
	void scalef(float x, float y, float z);
	void perspective(float fovy, float aspect, float zNear, float zFar);
	void lookAt(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz);

	void beginTriangles();
	void vertex3f(float x, float y, float z);
	void normal3f(float x, float y, float z);
	void endPrimitives();

	void setColor(uint8_t r, uint8_t g, uint8_t b);
}
