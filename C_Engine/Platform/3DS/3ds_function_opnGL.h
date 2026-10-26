#pragma once

// On devkitPro we can use Citro3D for rendering; if unavailable we fall back to no-op stubs.

// Use software renderer when citro3d not present
// Use Citro3D renderer when available
#include "citro_renderer.h"
#include <3ds.h>

#define GL_TRIANGLES 0x0004

inline void initOpenGL() { CitroRenderer::init(400,240); }
inline void perspectiveModeGL() { CitroRenderer::perspective(70.0f, 400.0f/240.0f, 0.1f, 1024.0f); }
inline void pushMatrix() { CitroRenderer::pushMatrix(); }
inline void popMatrix() { CitroRenderer::popMatrix(); }
inline void endFrame() { CitroRenderer::endFrame(); }
inline void start3DFrame() { CitroRenderer::startFrame(); }
inline void glSetColor(int r, int g, int b, float transparency) { CitroRenderer::setColor(r/255.0f,g/255.0f,b/255.0f); }
inline void setupLighting() {}
inline void glBegin(int mode) { if(mode==GL_TRIANGLES) CitroRenderer::beginTriangles(); }
inline void glEnd() { CitroRenderer::endPrimitives(); }
inline void glVertex3f(float x, float y, float z) { CitroRenderer::vertex3f(x,y,z); }
inline void glNormal3f(float x, float y, float z) { CitroRenderer::normal3f(x,y,z); }
inline void glTranslatef(float x, float y, float z) { CitroRenderer::translatef(x,y,z); }
inline void glRotatef(float angle, float x, float y, float z) { CitroRenderer::rotatef(angle,x,y,z); }
inline void glScalef(float x, float y, float z) { CitroRenderer::scalef(x,y,z); }
inline void glPushMatrix() { CitroRenderer::pushMatrix(); }
inline void glPopMatrix() { CitroRenderer::popMatrix(); }
inline void glEnable(int x) {}
inline void glDisable(int x) {}
inline void glMatrixMode(int x) {}
inline void glLoadIdentity() { CitroRenderer::loadIdentity(); }
inline void gluPerspective(float fovy, float aspect, float zNear, float zFar) { CitroRenderer::perspective(fovy, aspect, zNear, zFar); }
inline void gluLookAt(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz) { CitroRenderer::lookAt(ex,ey,ez,cx,cy,cz,ux,uy,uz); }
inline void glClearColor(float r, float g, float b, float a) { /* handled by citro renderer if needed */ }
inline void glClearDepth(float d) {}
inline void glViewport(int x, int y, int w, int h) {}
inline void glFlush(int x) {}
inline void glColor3b(int r, int g, int b) { CitroRenderer::setColor(r/255.0f,g/255.0f,b/255.0f); }
inline void glMaterialf(int a, int b) {}
