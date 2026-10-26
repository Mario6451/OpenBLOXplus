#include "3ds_soft_renderer.h"
#include <string.h>
#include <math.h>

namespace Soft3D {

static const int WIDTH = 400;
static const int HEIGHT = 240;

struct Vec3 { float x,y,z; };
struct Vertex { Vec3 pos; Vec3 norm; uint8_t r,g,b; };

static std::vector<Vertex> vbuf;
static float zbuffer[WIDTH * HEIGHT];
static u32* topfb = nullptr;
static uint8_t clear_r=0, clear_g=0, clear_b=0;

// Simple 4x4 matrix stack
struct Mat4 { float m[16]; };
static Mat4 matStack[32];
static int matTop = 0;
static Mat4 projMat;

static uint8_t cur_r=255, cur_g=255, cur_b=255;

static inline void matIdentity(Mat4 &a){ memset(a.m,0,sizeof(a.m)); a.m[0]=a.m[5]=a.m[10]=a.m[15]=1.0f; }
static inline Mat4 matMul(const Mat4 &a,const Mat4 &b){ Mat4 r; for(int i=0;i<4;i++) for(int j=0;j<4;j++){ r.m[i*4+j]=0; for(int k=0;k<4;k++) r.m[i*4+j]+=a.m[i*4+k]*b.m[k*4+j]; } return r; }

static Vec3 transform(const Mat4 &m, const Vec3 &v){ Vec3 out; float x=v.x,y=v.y,z=v.z,w=1.0f; out.x = m.m[0]*x + m.m[4]*y + m.m[8]*z + m.m[12]*w; out.y = m.m[1]*x + m.m[5]*y + m.m[9]*z + m.m[13]*w; out.z = m.m[2]*x + m.m[6]*y + m.m[10]*z + m.m[14]*w; float ww = m.m[3]*x + m.m[7]*y + m.m[11]*z + m.m[15]*w; if (ww!=0.0f){ out.x/=ww; out.y/=ww; out.z/=ww; } return out; }

void init(){
	// mat stack
	matTop = 0; matIdentity(matStack[0]); matIdentity(projMat);
	vbuf.clear();
}

void shutdown(){
	vbuf.clear();
}

void startFrame(){
	// get top framebuffer pointer
	topfb = (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr);
	// clear zbuffer
	for(int i=0;i<WIDTH*HEIGHT;i++) zbuffer[i]=1e9f;
	// clear fb
	u32 packed = 0xFF000000 | (clear_r<<16) | (clear_g<<8) | (clear_b);
	for(int i=0;i<WIDTH*HEIGHT;i++) topfb[i]=packed;
}

void endFrame(){
	// flush handled by platform after calling endFrame
}

void clearColor(uint8_t r, uint8_t g, uint8_t b){ clear_r=r; clear_g=g; clear_b=b; }

void pushMatrix(){ if(matTop<31){ matStack[matTop+1]=matStack[matTop]; matTop++; } }
void popMatrix(){ if(matTop>0) matTop--; }
void loadIdentity(){ matIdentity(matStack[matTop]); }
void translatef(float x,float y,float z){ Mat4 t; matIdentity(t); t.m[12]=x; t.m[13]=y; t.m[14]=z; matStack[matTop]=matMul(matStack[matTop],t); }
void scalef(float x,float y,float z){ Mat4 s; matIdentity(s); s.m[0]=x; s.m[5]=y; s.m[10]=z; matStack[matTop]=matMul(matStack[matTop],s); }
void rotatef(float angleDeg,float x,float y,float z){ float a = angleDeg * (3.14159265f/180.0f); float c=cosf(a), s=sinf(a); // only support simple axis-aligned rotations approx
	Mat4 r; matIdentity(r);
	if (x==1.0f){ r.m[5]=c; r.m[6]=s; r.m[9]=-s; r.m[10]=c; }
	else if (y==1.0f){ r.m[0]=c; r.m[2]=-s; r.m[8]=s; r.m[10]=c; }
	else if (z==1.0f){ r.m[0]=c; r.m[1]=s; r.m[4]=-s; r.m[5]=c; }
	matStack[matTop]=matMul(matStack[matTop], r);
}

void perspective(float fovy,float aspect,float zNear,float zFar){ float f = 1.0f/tanf(fovy*(3.14159265f/180.0f)/2.0f); Mat4 p; memset(p.m,0,sizeof(p.m)); p.m[0]=f/aspect; p.m[5]=f; p.m[10]=(zFar+zNear)/(zNear-zFar); p.m[11]=-1.0f; p.m[14]=(2*zFar*zNear)/(zNear-zFar); projMat = p; }

void lookAt(float ex,float ey,float ez,float cx,float cy,float cz,float ux,float uy,float uz){ /* naive: ignore, assume identity */ }

void setColor(uint8_t r,uint8_t g,uint8_t b){ cur_r=r; cur_g=g; cur_b=b; }

void beginTriangles(){ vbuf.clear(); }
void normal3f(float x,float y,float z){ /* store last normal on next vertex */ }

static Vec3 lastNormal={0,0,1};
void vertex3f(float x,float y,float z){ Vertex v; v.pos.x=x; v.pos.y=y; v.pos.z=z; v.norm=lastNormal; v.r=cur_r; v.g=cur_g; v.b=cur_b; vbuf.push_back(v); }

static inline void putPixel(int sx,int sy,u32 color){ if(sx<0||sx>=WIDTH||sy<0||sy>=HEIGHT) return; topfb[sy*WIDTH + sx] = color; }

static void rasterizeTriangle(const Vertex &v0,const Vertex &v1,const Vertex &v2){
	// Transform via modelview and projection
	Vec3 p0 = transform(projMat, transform(matStack[matTop], v0.pos));
	Vec3 p1 = transform(projMat, transform(matStack[matTop], v1.pos));
	Vec3 p2 = transform(projMat, transform(matStack[matTop], v2.pos));

	// NDC coordinates [-1,1]
	float x0 = (p0.x*0.5f + 0.5f) * WIDTH;
	float y0 = (1.0f - (p0.y*0.5f + 0.5f)) * HEIGHT;
	float x1 = (p1.x*0.5f + 0.5f) * WIDTH;
	float y1 = (1.0f - (p1.y*0.5f + 0.5f)) * HEIGHT;
	float x2 = (p2.x*0.5f + 0.5f) * WIDTH;
	float y2 = (1.0f - (p2.y*0.5f + 0.5f)) * HEIGHT;

	int minX = fmax(0, (int)floorf(fmin(fmin(x0,x1),x2)));
	int maxX = fmin(WIDTH-1, (int)ceilf(fmax(fmax(x0,x1),x2)));
	int minY = fmax(0, (int)floorf(fmin(fmin(y0,y1),y2)));
	int maxY = fmin(HEIGHT-1, (int)ceilf(fmax(fmax(y0,y1),y2)));

	float denom = ( (y1 - y2)*(x0 - x2) + (x2 - x1)*(y0 - y2) );
	if (fabs(denom) < 1e-6f) return;

	for(int yy=minY; yy<=maxY; ++yy){
		for(int xx=minX; xx<=maxX; ++xx){
			float a = ( (y1 - y2)*(xx - x2) + (x2 - x1)*(yy - y2) ) / denom;
			float b = ( (y2 - y0)*(xx - x2) + (x0 - x2)*(yy - y2) ) / denom;
			float c = 1.0f - a - b;
			if (a>=0 && b>=0 && c>=0){
				// simple depth: use interpolated z
				float z = a*p0.z + b*p1.z + c*p2.z;
				int idx = yy*WIDTH + xx;
				if (z < zbuffer[idx]){
					zbuffer[idx] = z;
					uint8_t r = (uint8_t)(a*v0.r + b*v1.r + c*v2.r);
					uint8_t g = (uint8_t)(a*v0.g + b*v1.g + c*v2.g);
					uint8_t bcol = (uint8_t)(a*v0.b + b*v1.b + c*v2.b);
					u32 color = 0xFF000000 | (r<<16) | (g<<8) | (bcol);
					putPixel(xx, yy, color);
				}
			}
		}
	}
}

void endPrimitives(){
	// assume triangles
	for(size_t i=0;i+2<vbuf.size(); i+=3){ rasterizeTriangle(vbuf[i], vbuf[i+1], vbuf[i+2]); }
}

} // namespace
