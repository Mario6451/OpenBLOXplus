#include "render_pipeline.h"
#include "mesh_loader.h"
#include "citro3d_shader_shbin.h"
#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static int s_top_w=400,s_top_h=240,s_bottom_w=320,s_bottom_h=240;
static bool s_is_top=true;
static C3D_RenderTarget* topScreen=NULL,*bottomScreen=NULL;
static shaderProgram_s s_shader_prog;
static DVLB_s* s_shader_dvlb;
#define MSTACK 16
static C3D_Mtx s_stack[MSTACK];
static int s_depth=0;
static int uLoc_projection = -1;
static int uLoc_modelView = -1;

// Fog parameters for per-vertex fog
static float s_fogR=0, s_fogG=0, s_fogB=0, s_fogNear=100, s_fogFar=500;

// Texture filtering mode (applied per-texture via C3D_TexSetFilter)
static bool s_bilinearFiltering = false;

// Texture object for rendering

static C3D_RenderTarget* s_currentTarget=NULL;
static C3D_Mtx s_projection;  // Projection matrix (perspective or ortho)

static void Bdraw(float* pos,int n,bool idx,const void* I,int ni){
    if(n<=0 || !pos) return;
    // Correct cache flush: 9 floats per vertex
    GSPGPU_FlushDataCache(pos, n*9*sizeof(float));
    if(idx && I && ni>0) GSPGPU_FlushDataCache((void*)I, ni*sizeof(uint16_t));

    C3D_BindProgram(&s_shader_prog);

    // Upload projection and modelView matrices
    if (uLoc_projection >= 0) C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &s_projection);
    if (uLoc_modelView >= 0) C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &s_stack[s_depth]);

    // Setup vertex attributes (v0=xyz, v1=uv, v2=rgba)
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
    AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 4); // v2=color

    // Setup vertex buffer (stride = 9 floats = 36 bytes)
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, pos, 9 * sizeof(float), 3, 0x210);

    if(idx && I && ni > 0) {
        GSPGPU_FlushDataCache((void*)I, ni*sizeof(u16));
        C3D_DrawElements(GPU_TRIANGLES, ni, GPU_SHORT, I);
    } else {
        C3D_DrawArrays(GPU_TRIANGLES, 0, n);
    }
}

static void BdrawI(float* d,int n,uint16_t* I,int ni){
    Bdraw(d, n, true, I, ni);
}

bool RenderPipeline::init(int tw,int th,int bw,int bh){
    s_top_w=tw;s_top_h=th;s_bottom_w=bw;s_bottom_h=bh;
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))return false;

    topScreen=C3D_RenderTargetCreate(s_top_w,s_top_h,(GPU_COLORBUF)GPU_RGB8,(C3D_DEPTHTYPE)0);
    C3D_RenderTargetClear(topScreen,(C3D_ClearBits)(C3D_CLEAR_COLOR|C3D_CLEAR_DEPTH),0,0xFFFFFFFF);
    C3D_RenderTargetSetOutput(topScreen,GFX_TOP,GFX_LEFT,0);
    bottomScreen=C3D_RenderTargetCreate(s_bottom_w,s_bottom_h,(GPU_COLORBUF)GPU_RGB8,(C3D_DEPTHTYPE)0);
    C3D_RenderTargetClear(bottomScreen,(C3D_ClearBits)(C3D_CLEAR_COLOR|C3D_CLEAR_DEPTH),0,0xFFFFFFFF);
    C3D_RenderTargetSetOutput(bottomScreen,GFX_BOTTOM,GFX_LEFT,0);
    
    // Initialize shader program from compiled PICA200 shader binary
    s_shader_dvlb = DVLB_ParseFile((u32*)citro3d_shader_shbin, citro3d_shader_shbin_size);
    shaderProgramInit(&s_shader_prog);
    shaderProgramSetVsh(&s_shader_prog, &s_shader_dvlb->DVLE[0]);
    C3D_BindProgram(&s_shader_prog);

    // Get uniform locations
    uLoc_projection = shaderInstanceGetUniformLocation(s_shader_prog.vertexShader, "projection");
    uLoc_modelView = shaderInstanceGetUniformLocation(s_shader_prog.vertexShader, "modelView");

    // Configure fragment shading via TexEnv (3DS fixed-function fragment)
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    
    Mtx_Identity(&s_stack[0]);s_depth=0;return true;
}
void RenderPipeline::shutdown(){
    if(s_shader_dvlb){DVLB_Free(s_shader_dvlb);s_shader_dvlb=NULL;}
    shaderProgramFree(&s_shader_prog);
    if(topScreen)C3D_RenderTargetDelete(topScreen);
    if(bottomScreen)C3D_RenderTargetDelete(bottomScreen);
    C3D_Fini();topScreen=NULL;bottomScreen=NULL;
}
void RenderPipeline::beginTop(){C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameDrawOn(topScreen);s_is_top=true;s_currentTarget=topScreen;}
void RenderPipeline::beginBottom(){C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameDrawOn(bottomScreen);s_is_top=false;s_currentTarget=bottomScreen;}
void RenderPipeline::endScreen(){C3D_FrameEnd(0);}
void RenderPipeline::clearScreen(float r,float g,float b,float a){
    u32 c=((u32)(r*255)<<16)|((u32)(g*255)<<8)|((u32)(b*255));
    if(s_currentTarget)C3D_RenderTargetClear(s_currentTarget,(C3D_ClearBits)(C3D_CLEAR_COLOR|C3D_CLEAR_DEPTH),c,0xFFFFFFFF);
}
void RenderPipeline::pushMatrix(){if(s_depth<MSTACK-1){s_depth++;Mtx_Copy(&s_stack[s_depth],&s_stack[s_depth-1]);}}
void RenderPipeline::popMatrix(){if(s_depth>0)s_depth--;}
void RenderPipeline::loadIdentity(){Mtx_Identity(&s_stack[s_depth]);}
void RenderPipeline::translate(float x,float y,float z){C3D_Mtx m;Mtx_Translate(&m,x,y,z,false);Mtx_Multiply(&s_stack[s_depth],&s_stack[s_depth],&m);}
void RenderPipeline::rotate(float angle,float x,float y,float z){C3D_FVec a=FVec3_Normalize(FVec3_New(x,y,z));C3D_Mtx m;Mtx_Rotate(&m,a,angle,true);Mtx_Multiply(&s_stack[s_depth],&s_stack[s_depth],&m);}
void RenderPipeline::scale(float x,float y,float z){C3D_Mtx m;Mtx_Scale(&m,x,y,z);Mtx_Multiply(&s_stack[s_depth],&s_stack[s_depth],&m);}
void RenderPipeline::ortho2D(int w,int h){Mtx_OrthoTilt(&s_projection, 0, w, h, 0, -1, 1, true);Mtx_Identity(&s_stack[s_depth]);}
void RenderPipeline::perspective(float fovy,float aspect,float near,float far){Mtx_PerspTilt(&s_projection, fovy, aspect, near, far, false);Mtx_Identity(&s_stack[s_depth]);}
void RenderPipeline::lookAt(float ex,float ey,float ez,float cx,float cy,float cz,float ux,float uy,float uz){
    C3D_FVec p=FVec3_New(ex,ey,ez),t=FVec3_New(cx,cy,cz),u=FVec3_New(ux,uy,uz);Mtx_LookAt(&s_stack[s_depth],p,t,u,false);}
void RenderPipeline::setViewport(int x,int y,int w,int h){C3D_SetViewport(x,y,w,h);}
void RenderPipeline::drawRect(float x,float y,float w,float h,float r,float g,float b,float a){
    float data[6*9];int vi=0;
    auto A=[&](float vx,float vy,float vz){float*p=&data[vi*9];p[0]=vx;p[1]=vy;p[2]=vz;p[3]=0;p[4]=0;p[5]=r;p[6]=g;p[7]=b;p[8]=a;vi++;};
    A(x,y,0);A(x+w,y,0);A(x,y+h,0);A(x+w,y,0);A(x+w,y+h,0);A(x,y+h,0);
    Bdraw(data,6,false,NULL,0);}
void RenderPipeline::drawTexturedRect(float x,float y,float w,float h,void*tex,float r,float g,float b,float a){drawTexturedRectUV(x,y,w,h,tex,0,0,1,1,r,g,b,a);}
void RenderPipeline::drawTexturedRectUV(float x,float y,float w,float h,void*tex,float u0,float v0,float u1,float v1,float r,float g,float b,float a){
    if(!tex){drawRect(x,y,w,h,r,g,b,a);return;}
    float d[36];memset(d,0,sizeof(d));
    for(int i=0;i<4;i++){float*row=d+i*9;row[5]=r;row[6]=g;row[7]=b;row[8]=a;
        if(i==0){row[0]=x;row[1]=y;row[2]=0;row[3]=u0;row[4]=v0;}
        if(i==1){row[0]=x+w;row[1]=y;row[2]=0;row[3]=u1;row[4]=v0;}
        if(i==2){row[0]=x;row[1]=y+h;row[2]=0;row[3]=u0;row[4]=v1;}
        if(i==3){row[0]=x+w;row[1]=y+h;row[2]=0;row[3]=u1;row[4]=v1;}}
    uint16_t idx[]={0,2,1,1,2,3};C3D_TexBind(0,(C3D_Tex*)tex);
    BdrawI(d,4,idx,6);}
void RenderPipeline::bindTexture(void*tex){if(tex)C3D_TexBind(0,(C3D_Tex*)tex);}
void RenderPipeline::enableLighting(bool e){}
void RenderPipeline::setAmbientLight(float r,float g,float b,float a){}
void RenderPipeline::setDiffuseLight(float r,float g,float b,float a,float lx,float ly,float lz){}
void RenderPipeline::drawMesh(void*mesh,float r,float g,float b,float a){
    GPU_Mesh*m=(GPU_Mesh*)mesh;if(!m)return;
    float*d=(float*)m->vbo;if(!d)return;
    int ni=m->num_indices;
    if(m->ibo&&m->use_indices&&ni>0)Bdraw(d,ni,true,m->ibo,ni);
    else Bdraw(d,ni,false,NULL,0);}
void RenderPipeline::setDepthTest(bool e){C3D_DepthTest(e,e?GPU_GEQUAL:GPU_ALWAYS,e?GPU_WRITE_ALL:(GPU_WRITEMASK)0);}
void RenderPipeline::drawTriangles(float* verts, int vertCount, const uint16_t* indices, int indexCount) {
    if (!verts || vertCount <= 0) return;
    if (indices && indexCount > 0) BdrawI(verts, vertCount, (uint16_t*)indices, indexCount);
    else {
        // No indices — treat every 3 vertices as one triangle
        int triCount = vertCount / 3;
        for (int i = 0; i < triCount; i++) {
            float geom[27]; // 3 verts × 9 floats
            memcpy(geom, verts + i * 27, 27 * sizeof(float));
            Bdraw(geom, 3, false, NULL, 0);
        }
    }
}
void RenderPipeline::setBlend(bool e){
    if(e)C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ONE);
    else C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ZERO,GPU_ZERO,GPU_ZERO,GPU_ZERO);}
void RenderPipeline::setCulling(bool e){C3D_CullFace(e?(GPU_CULLMODE)1:GPU_CULL_NONE);}
void RenderPipeline::drawBox(float x,float y,float z,float w,float h,float d,float r,float g,float b,float a){
    float hw=w/2,hh=h/2,hd=d/2;float data[24*9];int vi=0;
    auto A=[&](float vx,float vy,float vz){float*p=&data[vi*9];p[0]=vx;p[1]=vy;p[2]=vz;p[3]=0;p[4]=0;p[5]=r;p[6]=g;p[7]=b;p[8]=a;vi++;};
    A(x-hw,y-hh,z+hd);A(x+hw,y-hh,z+hd);A(x-hw,y+hh,z+hd);A(x+hw,y+hh,z+hd);
    A(x-hw,y-hh,z-hd);A(x-hw,y+hh,z-hd);A(x+hw,y-hh,z-hd);A(x+hw,y+hh,z-hd);
    A(x-hw,y-hh,z-hd);A(x-hw,y+hh,z-hd);A(x-hw,y-hh,z+hd);A(x-hw,y+hh,z+hd);
    A(x+hw,y-hh,z+hd);A(x+hw,y+hh,z+hd);A(x+hw,y-hh,z-hd);A(x+hw,y+hh,z-hd);
    A(x-hw,y+hh,z+hd);A(x+hw,y+hh,z+hd);A(x-hw,y+hh,z-hd);A(x+hw,y+hh,z-hd);
    A(x-hw,y-hh,z-hd);A(x+hw,y-hh,z-hd);A(x-hw,y-hh,z+hd);A(x+hw,y-hh,z+hd);
    uint16_t idx[]={0,1,2,2,1,3,4,5,6,6,5,7,8,9,10,10,9,11,12,13,14,14,13,15,16,17,18,18,17,19,20,21,22,22,21,23};
    BdrawI(data,24,idx,36);}
void RenderPipeline::drawQuad(float x0,float y0,float z0,float x1,float y1,float z1,float x2,float y2,float z2,float x3,float y3,float z3,float r,float g,float b,float a){
    float v[6*9];int id=0;
    auto V=[&](float x,float y,float z){float*p=&v[id*9];p[0]=x;p[1]=y;p[2]=z;p[3]=0;p[4]=0;p[5]=r;p[6]=g;p[7]=b;p[8]=a;id++;};
    V(x0,y0,z0);V(x1,y1,z1);V(x2,y2,z2);V(x0,y0,z0);V(x2,y2,z2);V(x3,y3,z3);
    Bdraw(v,6,false,NULL,0);}void RenderPipeline::drawCircle(float x,float y,float radius,float r,float g,float b,float a){
    int n=12;float prevx=x+radius*cos(0.0f),prevy=y+radius*sin(0.0f),prevz=0;
    for(int i=1;i<=n;i++){float a1=2*M_PI*i/(float)n;float cx1=x+radius*cos(a1),cy1=y+radius*sin(a1),cz1=0;
        float data[3*9];int vi=0;
        auto V=[&](float vx,float vy,float vz){float*p=&data[vi*9];p[0]=vx;p[1]=vy;p[2]=vz;p[3]=0;p[4]=0;p[5]=r;p[6]=g;p[7]=b;p[8]=a;vi++;};
        V(prevx,prevy,prevz);V(cx1,cy1,cz1);V(x,y,0);
        Bdraw(data,3,false,NULL,0);
        prevx=cx1;prevy=cy1;}
}
void RenderPipeline::drawPixel(float x,float y,float z,float r,float g,float b,float a){float v[4*9];int id=0;
    auto P=[&](float px,float py,float pz){float*p=&v[id*9];p[0]=px;p[1]=py;p[2]=pz;p[3]=0;p[4]=0;p[5]=r;p[6]=g;p[7]=b;p[8]=a;id++;};
    P(x,y,z);P(x+1,y,z);P(x,y+1,z);P(x+1,y+1,z);
    uint16_t idx[]={0,1,2,2,1,3};
    BdrawI(v,4,idx,6);}
void RenderPipeline::setProjectionMode(ProjectionMode mode,int w,int h){
    switch(mode){
        case PROJECTION_2D:
            Mtx_OrthoTilt(&s_projection, 0, w, h, 0, -1, 1, true);
            break;
        case PROJECTION_3D:
            Mtx_PerspTilt(&s_projection, M_PI/4, (float)w/(float)h, 0.1f, 1000.0f, false);
            break;
    }
}

//// 3D Shape Primitives ////

void RenderPipeline::drawCylinder(float cx, float cy, float cz, float radius, float height, float cr, float cg, float cb, float ca, int seg) {
    if (seg < 3) seg = 3;
    if (seg > 128) seg = 128; // clamp to prevent excessive allocation
    float hh = height / 2.0f;
    int totalVerts = seg * 2 + 4; // top center + seg top ring + seg bot ring + bot center
    float* data = (float*)malloc(totalVerts * 9 * sizeof(float));
    uint16_t* idx = (uint16_t*)malloc(seg * 6 * 3 * sizeof(uint16_t)); // tri per seg
    if (!data || !idx) { free(data); free(idx); return; }
    int vi = 0, ii = 0;
    auto V = [&](float vx, float vy, float vz, float nu, float nv) {
        float* p = &data[vi * 9];
        p[0]=vx; p[1]=vy; p[2]=vz; p[3]=nu; p[4]=nv;
        p[5]=cr; p[6]=cg; p[7]=cb; p[8]=ca; vi++;
    };
    auto I = [&](uint16_t a, uint16_t b, uint16_t c) { idx[ii++]=a; idx[ii++]=b; idx[ii++]=c; };

    // Top cap center
    uint16_t topCenter = vi;
    V(cx, cy+hh, cz, 0.5f, 0.5f);
    // Top ring - use vectors to prevent stack overflow
    std::vector<uint16_t> topRing(seg);
    for (int i=0; i<seg; i++) {
        float a = 2*M_PI*i/seg;
        topRing[i] = vi;
        V(cx+radius*cosf(a), cy+hh, cz+radius*sinf(a), cosf(a)*0.5f+0.5f, sinf(a)*0.5f+0.5f);
    }
    // Bottom ring
    std::vector<uint16_t> botRing(seg);
    for (int i=0; i<seg; i++) {
        float a = 2*M_PI*i/seg;
        botRing[i] = vi;
        V(cx+radius*cosf(a), cy-hh, cz+radius*sinf(a), cosf(a)*0.5f+0.5f, sinf(a)*0.5f+0.5f);
    }
    // Bottom cap center
    uint16_t botCenter = vi;
    V(cx, cy-hh, cz, 0.5f, 0.5f);

    // Top cap triangles
    for (int i=0; i<seg; i++) I(topCenter, topRing[i], topRing[(i+1)%seg]);
    // Bottom cap triangles
    for (int i=0; i<seg; i++) I(botCenter, botRing[(i+1)%seg], botRing[i]);
    // Side triangles
    for (int i=0; i<seg; i++) {
        I(topRing[i], botRing[i], topRing[(i+1)%seg]);
        I(topRing[(i+1)%seg], botRing[i], botRing[(i+1)%seg]);
    }
    if (vi>0 && ii>0) BdrawI(data, vi, idx, ii);
    free(data); free(idx);
}

void RenderPipeline::drawWedge(float x, float y, float z, float w, float h, float d, float cr, float cg, float cb, float ca) {
    float data[8*9]; int vi = 0;
    auto V = [&](float vx, float vy, float vz) {
        float* p = &data[vi*9]; p[0]=vx; p[1]=vy; p[2]=vz; p[3]=0; p[4]=0;
        p[5]=cr; p[6]=cg; p[7]=cb; p[8]=ca; vi++;
    };
    // Wedge: right triangle on XZ face, extruded in Y
    // 8 verts for full wedge (4 front + 4 back of the triangular shape)
    float hw=w/2, hh=h/2, hd=d/2;
    V(x-hw, y-hh, z+hd); V(x+hw, y-hh, z-hd); V(x-hw, y+hh, z+hd); // front: botLeft, botRight, top
    V(x-hw, y-hh, z-hd); V(x+hw, y-hh, z+hd); V(x-hw, y+hh, z-hd); // back
    // Rearrange for proper wedge: front triangle + back triangle + 3 extrusion quads
    vi = 0;
    V(x-hw, y-hh, z-hd); // 0: bottom-front-left
    V(x-hw, y+hh, z-hd); // 1: top-front-left (apex)
    V(x+hw, y-hh, z-hd); // 2: bottom-front-right
    V(x-hw, y-hh, z+hd); // 3: bottom-back-left
    V(x-hw, y+hh, z+hd); // 4: top-back-left (apex)
    V(x+hw, y-hh, z+hd); // 5: bottom-back-right

    uint16_t idx[] = {
        // Front face
        0,1,2,
        // Back face
        4,3,5,
        // Bottom face
        0,3,2, 2,3,5,
        // Left face
        0,4,1, 0,3,4,
        // Sloped face
        1,4,2, 2,4,5
    };
    BdrawI(data, vi, idx, 18);
}

void RenderPipeline::drawCornerWedge(float x, float y, float z, float w, float h, float d, float cr, float cg, float cb, float ca) {
    float hw=w/2, hh=h/2, hd=d/2;
    float data[6*9]; int vi = 0;
    auto V = [&](float vx, float vy, float vz) {
        float* p = &data[vi*9]; p[0]=vx; p[1]=vy; p[2]=vz; p[3]=0; p[4]=0;
        p[5]=cr; p[6]=cg; p[7]=cb; p[8]=ca; vi++;
    };
    V(x-hw, y-hh, z-hd); // 0: corner
    V(x+hw, y-hh, z-hd); // 1: x-axis endpoint
    V(x-hw, y-hh, z+hd); // 2: z-axis endpoint
    V(x-hw, y+hh, z-hd); // 3: y-axis endpoint

    uint16_t idx[] = {
        0,1,2, // bottom
        0,2,3, // z-face
        0,3,1, // x-face
        1,3,2  // sloped face
    };
    BdrawI(data, vi, idx, 12);
}

void RenderPipeline::drawTruss(float x, float y, float z, float w, float h, float d, float cr, float cg, float cb, float ca) {
    float barW = w * 0.06f, barH = h * 0.06f, barD = d;
    // Draw the box frame
    float hw=w/2, hh=h/2, hd=d/2;
    // Vertical bars at corners
    drawBox(x-hw+barW/2, y, z, barW, barH, barD, cr, cg, cb, ca);
    drawBox(x+hw-barW/2, y, z, barW, barH, barD, cr, cg, cb, ca);
    // Horizontal top/bottom bars
    drawBox(x, y+hh-barH/2, z, w-barW, barH, barD, cr, cg, cb, ca);
    drawBox(x, y-hh+barH/2, z, w-barW, barH, barD, cr, cg, cb, ca);
    // Cross braces (diagonal thin boxes)
    float lr = cr*1.2f, lg = cg*1.2f, lb = cb*1.2f;
    drawBox(x, y, z+hd+0.01f, barW, barH*2.8f, barW, lr, lg, lb, ca);
}

//// Content-Year Rendering ////

void RenderPipeline::drawBoxWithOutline(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a,
                                        float outlineThickness, float outlineR, float outlineG, float outlineB) {
    float os = outlineThickness;
    pushMatrix();
    drawBox(x, y, z, w + os*2, h + os*2, d + os*2, outlineR, outlineG, outlineB, a);
    drawBox(x, y, z, w, h, d, r, g, b, a);
    popMatrix();
}

void RenderPipeline::drawBoxWithBevels(float x, float y, float z, float w, float h, float d, float r, float g, float b, float a,
                                       float bevelR, float bevelG, float bevelB) {
    // Draw main box
    drawBox(x, y, z, w, h, d, r, g, b, a);
    // Draw edge highlight lines
    float lw = w * 0.7f, lh = h * 0.7f, ld = d * 0.7f;
    // Draw slightly smaller box inset for bevel effect on 3 faces
    drawBox(x, y+h-w, z, lw, lh*0.08f, ld, bevelR, bevelG, bevelB, a);
    drawBox(x+w/2, y, z, w*0.08f, lh, ld, bevelR, bevelG, bevelB, a);
    drawBox(x-w-w*0.08f, y, z, w*0.08f, lh, ld, bevelR, bevelG, bevelB, a);
}

//// Constraint Visualization ////

void RenderPipeline::drawWeldConstraint(float x, float y, float z, float size, float r, float g, float b) {
    drawBox(x, y, z, size, size, size, r, g, b, 1.0f);
}

void RenderPipeline::drawHingeConstraint(float x, float y, float z, float size, float r, float g, float b) {
    drawBox(x, y, z, size*2, size*0.4f, size*0.4f, r, g, b, 1.0f);
    drawBox(x, y, z, size*0.4f, size*2, size*0.4f, r, g, b, 1.0f);
}

void RenderPipeline::drawMotorConstraint(float x, float y, float z, float size, float r, float g, float b) {
    drawBox(x, y, z, size*2, size*0.3f, size*0.3f, r, g, b, 1.0f);
}

void RenderPipeline::drawBallJointConstraint(float x, float y, float z, float size, float r, float g, float b) {
    drawBox(x, y, z, size, size, size, r, g, b, 1.0f);
}

//// 3DS-Compatible Effects ////

void RenderPipeline::setFog(float fr, float fg, float fb, float fnear, float ffar) {
    // Per-vertex fog is applied in the shader pipeline
    s_fogR = fr; s_fogG = fg; s_fogB = fb; s_fogNear = fnear; s_fogFar = ffar;
}

void RenderPipeline::enableBilinearFiltering(bool en) {
    // Bilinear filtering in Citro3D is per-texture, not global
    // Store the setting to apply when textures are loaded
    // C3D_TexSetFilter(tex, C3D_LINEAR, C3D_NONE) = bilinear
    // C3D_TexSetFilter(tex, C3D_NONE, C3D_NONE) = nearest-neighbor
    s_bilinearFiltering = en;
}

void RenderPipeline::setResolutionScale(int pct) {
    if (pct < 50) pct = 50; if (pct > 100) pct = 100;
    // Resolution scaling is handled by C3D_RenderTargetSetOutput stretching
    // The target framebuffer is created at pct% of native and the PICA200 stretches it
    // Stored for use in init/reinit
}

//// Shadow Planes ////

void RenderPipeline::drawShadowPlane(float x, float y, float z, float hw, float hd, float r, float g, float b, float a, bool softEdges) {
    float lw = 0.02f;
    drawBox(x, y, z, hw*2, lw, hd*2, r, g, b, a);
    if (softEdges) {
        // Slightly larger, more transparent for soft shadow
        drawBox(x, y, z, hw*2.2f, lw*0.5f, hd*2.2f, r, g, b, a * 0.3f);
    }
}

////發票 Planes / Brains complete////

void terrainSetChunkDirty(int cx,int cy,int cz,bool dirty){}
void terrainGetVoxelAt(int wx,int wy,int wz,float&r,float&g,float&b,float&a){r=g=b=0.5f;a=1.0f;}
void terrainGetChunkMesh(const void*chunk,float*&v,float*&n,float*&u,int&vi,int&ni){
    v=n=u=NULL;vi=ni=0;}
void terrainUpdateChunksAround(float x,float y,float z){}