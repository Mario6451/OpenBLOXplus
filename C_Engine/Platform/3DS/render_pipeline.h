#pragma once

#include <3ds.h>
#include <citro3d.h>
#include <string>

// High-level rendering pipeline wrapper
// Provides: GPU rendering, UI primitives, mesh drawing, texture binding

namespace RenderPipeline {

    // Initialize the rendering pipeline (call once at startup)
    bool init(int top_w = 400, int top_h = 240, int bottom_w = 320, int bottom_h = 240);

    // Shutdown and free all resources
    void shutdown();

    // Begin rendering to top screen
    void beginTop();

    // Begin rendering to bottom screen
    void beginBottom();

    // End frame on current screen
    void endScreen();

    // Clear screen with color
    void clearScreen(float r, float g, float b, float a = 1.0f);

    // Matrix operations
    void pushMatrix();
    void popMatrix();
    void loadIdentity();
    void translate(float x, float y, float z);
    void rotate(float angle, float x, float y, float z);
    void scale(float x, float y, float z);
    void ortho2D(int w, int h);   // 2D orthographic projection
    void perspective(float fovy, float aspect, float near_z, float far_z);
    void lookAt(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz);

    // Set viewport (defaults to full screen)
    void setViewport(int x, int y, int w, int h);

    // UI Drawing Primitives (2D, screen-space)
    // Draw a filled rectangle with color
    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);

    // Draw a textured rectangle
    void drawTexturedRect(float x, float y, float w, float h, void *texture, float r, float g, float b, float a = 1.0f);

    // Draw a textured rectangle with UV coordinates
    void drawTexturedRectUV(float x, float y, float w, float h, void *texture, float u0, float v0, float u1, float v1, float r, float g, float b, float a = 1.0f);

    // Bind a texture for drawing
    void bindTexture(void *texture);

    // Set 3D lighting
    void enableLighting(bool enable);
    void setAmbientLight(float r, float g, float b, float a);
    void setDiffuseLight(float r, float g, float b, float a, float lx, float ly, float lz);

    // Draw 3D mesh (GPU_Mesh)
    void drawMesh(void *gpu_mesh, float r, float g, float b, float a = 1.0f);

    // Enable/disable depth testing
    void setDepthTest(bool enable);

    // Enable/disable blending (for transparency)
    void setBlend(bool enable);

    // Enable/disable backface culling
    void setCulling(bool enable);

    // Draw a simple 3D box (useful for debugging)
    void drawBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a = 1.0f);

    // Draw a 3D quad (6 coords + color)
    void drawQuad(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float r, float g, float b, float a = 1.0f);

    // Draw a 3D circle (approximated as small quad)
    void drawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f);

    // Draw a 3D pixel (tiny quad)
    void drawPixel(float x, float y, float z, float r, float g, float b, float a = 1.0f);

    // Set the projection matrix mode (2D or 3D)
    enum ProjectionMode { PROJECTION_2D, PROJECTION_3D };
    void setProjectionMode(ProjectionMode mode, int w = 400, int h = 240);

    // 3D shape primitives
    void drawCylinder(float x, float y, float z, float r, float h, float r_c, float g_c, float b_c, float a = 1.0f, int segments = 16);
    void drawWedge(float x, float y, float z, float w, float h, float d, float r_c, float g_c, float b_c, float a = 1.0f);
    void drawCornerWedge(float x, float y, float z, float w, float h, float d, float r_c, float g_c, float b_c, float a = 1.0f);
    void drawTruss(float x, float y, float z, float w, float h, float d, float r_c, float g_c, float b_c, float a = 1.0f);

    // Content-year rendering features
    void drawBoxWithOutline(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a, float outlineThickness, float outlineR, float outlineG, float outlineB);
    void drawBoxWithBevels(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a, float bevelR, float bevelG, float bevelB);

    // Constraint visualization (draw small joint indicator at position)
    void drawWeldConstraint(float x, float y, float z, float size, float r, float g, float b);
    void drawHingeConstraint(float x, float y, float z, float size, float r, float g, float b);
    void drawMotorConstraint(float x, float y, float z, float size, float r, float g, float b);
    void drawBallJointConstraint(float x, float y, float z, float size, float r, float g, float b);

    // 3DS-compatible effects
    void setFog(float r, float g, float b, float near, float far);
    void enableBilinearFiltering(bool en);
    void setResolutionScale(int pct); // 50-100 percent

    // Draw raw triangles from vertex array (9 floats per vertex: x,y,z,u,v,r,g,b,a)
    void drawTriangles(float *verts, int vertCount, const uint16_t *indices, int indexCount);

    // Shadow planes
    void drawShadowPlane(float x, float y, float z, float hw, float hd, float r, float g, float b, float a, bool softEdges);
}