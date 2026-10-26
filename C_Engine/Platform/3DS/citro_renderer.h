#pragma once

#include <3ds.h>
#include <citro3d.h>
#include <vector>

// GPU-accelerated renderer using Citro3D + RenderPipeline

namespace CitroRenderer {
	bool init(int width=400,int height=240);
	void shutdown();
	void startFrame();
	void endFrame();

	void pushMatrix();
	void popMatrix();
	void loadIdentity();
	void translatef(float x,float y,float z);
	void rotatef(float angle,float x,float y,float z);
	void scalef(float x,float y,float z);
	void perspective(float fovy,float aspect,float zNear,float zFar);
	void lookAt(float ex,float ey,float ez,float cx,float cy,float cz,float ux,float uy,float uz);

	void beginTriangles();
	void vertex3f(float x,float y,float z);
	void normal3f(float x,float y,float z);
	void texCoord2f(float u,float v);
	void endPrimitives();

	void setColor(float r,float g,float b);

	// GPU mesh drawing
	void drawBox(float x, float y, float z, float w, float h, float d, float r, float g, float b);
	void drawTexturedQuad(float x, float y, float w, float h, void *tex, float r, float g, float b);

	// 3D shape primitives
    void drawCylinder(float x, float y, float z, float rRadius, float h, float r, float g, float b, float a = 1.0f, int segments = 16);
    void drawWedge(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a = 1.0f);
    void drawCornerWedge(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a = 1.0f);
    void drawTruss(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a = 1.0f);

    // Content-year rendering
    void drawBoxWithOutline(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a, float outlineThickness, float outlineR, float outlineG, float outlineB);
    void drawBoxWithBevels(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a, float bevelR, float bevelG, float bevelB);

    // Shadow planes
    void drawShadowPlane(float x, float y, float z, float hw, float hd, float r, float g, float b, float a, bool softEdges);

    // 3DS effects
    void setFog(float r, float g, float b, float near, float far);
    void enableBilinearFiltering(bool en);
    void setResolutionScale(int pct); // 50-100

	// Screen clearing
	void clearScreen(float r, float g, float b, float a = 1.0f);

	// Depth and blending
	void setDepthTest(bool enable);
	void setBlend(bool enable);
}

// Simple 2D drawing functions (bottom screen, 320x240)
void DrawPixel(int x, int y, int r, int g, int b);
void DrawRect(int x, int y, int w, int h, int r, int g, int b);
