// // GPU-accelerated renderer using Citro3D + RenderPipeline
#include "citro_renderer.h"
#include "render_pipeline.h"
#include <string.h>
#include <stdlib.h>

using namespace CitroRenderer;

struct ImmediateVertex { float x,y,z,u,v,r,g,b,a; };

static std::vector<ImmediateVertex> s_immediate_verts;
static std::vector<uint16_t> s_immediate_idx;
static float s_cur_r=1,s_cur_g=1,s_cur_b=1,s_cur_a=1;
static float s_cur_u=0,s_cur_v=0;
static C3D_BufInfo s_immediate_buf;
static bool s_using_gpu = true;

bool CitroRenderer::init(int w, int h) {
    s_using_gpu = RenderPipeline::init(w, h, 320, 240);
    BufInfo_Init(&s_immediate_buf);
    return s_using_gpu;
}

void CitroRenderer::shutdown() {
    if (s_using_gpu) RenderPipeline::shutdown();
}

void CitroRenderer::startFrame() {
    if (!s_using_gpu) return;
    // Start top screen rendering
    RenderPipeline::beginTop();
    RenderPipeline::setProjectionMode(RenderPipeline::PROJECTION_3D, 400, 240);
    RenderPipeline::setDepthTest(true);
    RenderPipeline::setCulling(true);
}

void CitroRenderer::endFrame() {
    if (!s_using_gpu) return;
    RenderPipeline::endScreen();
    gfxFlushBuffers();
    RenderPipeline::beginBottom();
    RenderPipeline::setProjectionMode(RenderPipeline::PROJECTION_2D, 320, 240);
    RenderPipeline::setDepthTest(false);
    RenderPipeline::setCulling(false);
    RenderPipeline::endScreen();
    gfxFlushBuffers();
    gspWaitForVBlank();
}

void CitroRenderer::pushMatrix() { if(s_using_gpu) RenderPipeline::pushMatrix(); }
void CitroRenderer::popMatrix() { if(s_using_gpu) RenderPipeline::popMatrix(); }
void CitroRenderer::loadIdentity() { if(s_using_gpu) RenderPipeline::loadIdentity(); }
void CitroRenderer::translatef(float x,float y,float z) { if(s_using_gpu) RenderPipeline::translate(x,y,z); }
void CitroRenderer::rotatef(float angle,float x,float y,float z) { if(s_using_gpu) RenderPipeline::rotate(angle,x,y,z); }
void CitroRenderer::scalef(float x,float y,float z) { if(s_using_gpu) RenderPipeline::scale(x,y,z); }
void CitroRenderer::perspective(float fovy,float aspect,float zNear,float zFar) { if(s_using_gpu) RenderPipeline::perspective(fovy,aspect,zNear,zFar); }
void CitroRenderer::lookAt(float ex,float ey,float ez,float cx,float cy,float cz,float ux,float uy,float uz) {
    if(s_using_gpu) RenderPipeline::lookAt(ex,ey,ez,cx,cy,cz,ux,uy,uz);
}

void CitroRenderer::beginTriangles() {
    s_immediate_verts.clear();
    s_immediate_idx.clear();
}

void CitroRenderer::vertex3f(float x,float y,float z) {
    ImmediateVertex v = {x,y,z, s_cur_u,s_cur_v, s_cur_r,s_cur_g,s_cur_b,s_cur_a};
    s_immediate_verts.push_back(v);
    s_immediate_idx.push_back((uint16_t)(s_immediate_verts.size()-1));
}

void CitroRenderer::normal3f(float x,float y,float z) { /* Normals handled in shader */ }
void CitroRenderer::texCoord2f(float u,float v) { s_cur_u = u; s_cur_v = v; }
void CitroRenderer::endPrimitives() {
    if (s_immediate_verts.empty()) return;
    int n = (int)s_immediate_verts.size();
    // Convert ImmediateVertex array (9 floats: x,y,z,u,v,r,g,b,a) to flat float buffer
    float *verts = (float*)s_immediate_verts.data();
    RenderPipeline::drawTriangles(verts, n, s_immediate_idx.data(), (int)s_immediate_idx.size());
    s_immediate_verts.clear();
    s_immediate_idx.clear();
}

void CitroRenderer::setColor(float r,float g,float b) {
    s_cur_r = r; s_cur_g = g; s_cur_b = b;
}

void CitroRenderer::drawBox(float x, float y, float z, float w, float h, float d, float r, float g, float b) {
    if (!s_using_gpu) return;
    RenderPipeline::drawBox(x, y, z, w, h, d, r, g, b, 1.0f);
}

void CitroRenderer::drawTexturedQuad(float x, float y, float w, float h, void *tex, float r, float g, float b) {
    if (!s_using_gpu) return;
    RenderPipeline::drawTexturedRect(x, y, w, h, tex, r, g, b, 1.0f);
}

void CitroRenderer::clearScreen(float r, float g, float b, float a) {
    if (!s_using_gpu) return;
    RenderPipeline::clearScreen(r, g, b, a);
}

void CitroRenderer::setDepthTest(bool enable) {
    if (!s_using_gpu) return;
    RenderPipeline::setDepthTest(enable);
}

void CitroRenderer::setBlend(bool enable) {
    if (!s_using_gpu) return;
    RenderPipeline::setBlend(enable);
}

void CitroRenderer::drawCylinder(float x, float y, float z, float rRadius, float h, float r, float g, float b, float a, int segments) {
    if (!s_using_gpu) return;
    RenderPipeline::drawCylinder(x, y, z, rRadius, h, r, g, b, a, segments);
}

void CitroRenderer::drawWedge(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a) {
    if (!s_using_gpu) return;
    RenderPipeline::drawWedge(x, y, z, w, h, d, r, g, b, a);
}

void CitroRenderer::drawCornerWedge(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a) {
    if (!s_using_gpu) return;
    RenderPipeline::drawCornerWedge(x, y, z, w, h, d, r, g, b, a);
}

void CitroRenderer::drawTruss(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a) {
    if (!s_using_gpu) return;
    RenderPipeline::drawTruss(x, y, z, w, h, d, r, g, b, a);
}

void CitroRenderer::drawBoxWithOutline(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a, float outlineThickness, float outlineR, float outlineG, float outlineB) {
    if (!s_using_gpu) return;
    RenderPipeline::drawBoxWithOutline(x, y, z, w, h, d, r, g, b, a, outlineThickness, outlineR, outlineG, outlineB);
}

void CitroRenderer::drawBoxWithBevels(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a, float bevelR, float bevelG, float bevelB) {
    if (!s_using_gpu) return;
    RenderPipeline::drawBoxWithBevels(x, y, z, w, h, d, r, g, b, a, bevelR, bevelG, bevelB);
}

void CitroRenderer::drawShadowPlane(float x, float y, float z, float hw, float hd, float r, float g, float b, float a, bool softEdges) {
    if (!s_using_gpu) return;
    RenderPipeline::drawShadowPlane(x, y, z, hw, hd, r, g, b, a, softEdges);
}

void CitroRenderer::setFog(float r, float g, float b, float near, float far) {
    if (!s_using_gpu) return;
    RenderPipeline::setFog(r, g, b, near, far);
}

void CitroRenderer::enableBilinearFiltering(bool en) {
    if (!s_using_gpu) return;
    RenderPipeline::enableBilinearFiltering(en);
}

void CitroRenderer::setResolutionScale(int pct) {
    if (!s_using_gpu) return;
    RenderPipeline::setResolutionScale(pct);
}

// Simple 2D pixel drawing (bottom screen, 320x240)
// Uses framebuffer write for UI rendering
static void* s_bottomFramebuffer = nullptr;
static bool s_framebufferInitialized = false;

static void ensureFramebuffer() {
    if (!s_framebufferInitialized) {
        s_bottomFramebuffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, nullptr, nullptr);
        s_framebufferInitialized = true;
    }
}

void DrawPixel(int x, int y, int r, int g, int b) {
    if (x < 0 || x >= 320 || y < 0 || y >= 240) return;
    ensureFramebuffer();
    if (!s_bottomFramebuffer) return;
    u32* fb = (u32*)s_bottomFramebuffer;
    int idx = (y * 320 + x);
    fb[idx] = (r << 16) | (g << 8) | (b << 0) | (0xFF << 24); // RGB8 + alpha
}

void DrawRect(int x, int y, int w, int h, int r, int g, int b) {
    if (x < 0 || y < 0 || w <= 0 || h <= 0) return;
    ensureFramebuffer();
    if (!s_bottomFramebuffer) return;
    u32* fb = (u32*)s_bottomFramebuffer;
    u32 color = ((u32)r << 16) | ((u32)g << 8) | (u32)b | (0xFF << 24);
    int cl_x = x < 0 ? 0 : x;
    int cl_y = y < 0 ? 0 : y;
    int cl_w = cl_x + w > 320 ? 320 - cl_x : w;
    int cl_h = cl_y + h > 240 ? 240 - cl_y : h;
    for (int dy = 0; dy < cl_h; dy++) {
        int row = (cl_y + dy) * 320 + cl_x;
        for (int dx = 0; dx < cl_w; dx++)
            fb[row + dx] = color;
    }
}

