#include "r15_character.h"
#include "citro_renderer.h"
#include "texture_loader.h"
#include "../../CoreEngine/Node/nodes.h"
#include "../../rbxlb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>

namespace R15Character {

    // Helper: Create a simple box mesh for a body part
    static PartMesh createBoxMesh(R15Part part, float w, float h, float d, float offsetX, float offsetY, float offsetZ) {
        PartMesh mesh;
        mesh.part = part;
        mesh.isSkin = true;
        
        // 8 vertices for a box
        float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;
        
        // Vertices: -x-y-z, +x-y-z, -x+y-z, +x+y-z, -x-y+z, +x-y+z, -x+y+z, +x+y+z
        mesh.vertices = {
            -hw + offsetX, -hh + offsetY, -hd + offsetZ,
             hw + offsetX, -hh + offsetY, -hd + offsetZ,
            -hw + offsetX,  hh + offsetY, -hd + offsetZ,
             hw + offsetX,  hh + offsetY, -hd + offsetZ,
            -hw + offsetX, -hh + offsetY,  hd + offsetZ,
             hw + offsetX, -hh + offsetY,  hd + offsetZ,
            -hw + offsetX,  hh + offsetY,  hd + offsetZ,
             hw + offsetX,  hh + offsetY,  hd + offsetZ
        };
        
        // Normals for each face (6 faces * 4 vertices)
        mesh.normals = {
            // Front (-Z)
            0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
            // Back (+Z)
            0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
            // Top (+Y)
            0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
            // Bottom (-Y)
            0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
            // Left (-X)
            -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
            // Right (+X)
            1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0
        };
        
        // UV coordinates
        mesh.uv = {
            0, 1, 1, 1, 0, 0, 1, 0,
            0, 1, 1, 1, 0, 0, 1, 0,
            0, 1, 1, 1, 0, 0, 1, 0,
            0, 1, 1, 1, 0, 0, 1, 0,
            0, 1, 1, 1, 0, 0, 1, 0,
            0, 1, 1, 1, 0, 0, 1, 0
        };
        
        // 36 indices (6 faces * 2 triangles * 3 indices)
        mesh.indices = {
            // Front
            0, 1, 2, 1, 3, 2,
            // Back
            5, 4, 7, 4, 6, 7,
            // Top
            2, 3, 6, 3, 7, 6,
            // Bottom
            4, 5, 0, 5, 1, 0,
            // Left
            0, 4, 2, 4, 6, 2,
            // Right
            1, 5, 3, 5, 7, 3
        };
        
        return mesh;
    }

    // Create default R15 skeleton
    static std::vector<Joint> createSkeleton() {
        std::vector<Joint> joints;
        memset(&joints[0], 0, sizeof(Joint) * (int)R15Part::Count);
        joints.resize((int)R15Part::Count);
        
        // Torso (root)
        joints[(int)R15Part::Torso].name = "Torso";
        joints[(int)R15Part::Torso].position[0] = 0; joints[(int)R15Part::Torso].position[1] = 2; joints[(int)R15Part::Torso].position[2] = 0;
        joints[(int)R15Part::Torso].scale[0] = 1; joints[(int)R15Part::Torso].scale[1] = 1; joints[(int)R15Part::Torso].scale[2] = 1;
        joints[(int)R15Part::Torso].parentIndex = -1;
        joints[(int)R15Part::Torso].childCount = 5;
        joints[(int)R15Part::Torso].childIndices[0] = (int)R15Part::LeftUpperArm;
        joints[(int)R15Part::Torso].childIndices[1] = (int)R15Part::RightUpperArm;
        joints[(int)R15Part::Torso].childIndices[2] = (int)R15Part::LeftUpperLeg;
        joints[(int)R15Part::Torso].childIndices[3] = (int)R15Part::RightUpperLeg;
        joints[(int)R15Part::Torso].childIndices[4] = (int)R15Part::Neck;
        
        // Neck -> Head
        joints[(int)R15Part::Neck].name = "Neck";
        joints[(int)R15Part::Neck].position[0] = 0; joints[(int)R15Part::Neck].position[1] = 1; joints[(int)R15Part::Neck].position[2] = 0;
        joints[(int)R15Part::Neck].parentIndex = (int)R15Part::Torso;
        joints[(int)R15Part::Neck].childCount = 1;
        joints[(int)R15Part::Neck].childIndices[0] = (int)R15Part::Head;
        
        joints[(int)R15Part::Head].name = "Head";
        joints[(int)R15Part::Head].position[0] = 0; joints[(int)R15Part::Head].position[1] = 1; joints[(int)R15Part::Head].position[2] = 0;
        joints[(int)R15Part::Head].parentIndex = (int)R15Part::Neck;
        
        // Arms
        joints[(int)R15Part::LeftUpperArm].name = "LeftUpperArm";
        joints[(int)R15Part::LeftUpperArm].position[0] = -1.25f; joints[(int)R15Part::LeftUpperArm].position[1] = 0; joints[(int)R15Part::LeftUpperArm].position[2] = 0;
        joints[(int)R15Part::LeftUpperArm].parentIndex = (int)R15Part::Torso;
        joints[(int)R15Part::LeftUpperArm].childCount = 1;
        joints[(int)R15Part::LeftUpperArm].childIndices[0] = (int)R15Part::LeftLowerArm;
        
        joints[(int)R15Part::RightUpperArm].name = "RightUpperArm";
        joints[(int)R15Part::RightUpperArm].position[0] = 1.25f; joints[(int)R15Part::RightUpperArm].position[1] = 0; joints[(int)R15Part::RightUpperArm].position[2] = 0;
        joints[(int)R15Part::RightUpperArm].parentIndex = (int)R15Part::Torso;
        joints[(int)R15Part::RightUpperArm].childCount = 1;
        joints[(int)R15Part::RightUpperArm].childIndices[0] = (int)R15Part::RightLowerArm;
        
        joints[(int)R15Part::LeftLowerArm].name = "LeftLowerArm";
        joints[(int)R15Part::LeftLowerArm].position[0] = 0; joints[(int)R15Part::LeftLowerArm].position[1] = -1; joints[(int)R15Part::LeftLowerArm].position[2] = 0;
        joints[(int)R15Part::LeftLowerArm].parentIndex = (int)R15Part::LeftUpperArm;
        
        joints[(int)R15Part::RightLowerArm].name = "RightLowerArm";
        joints[(int)R15Part::RightLowerArm].position[0] = 0; joints[(int)R15Part::RightLowerArm].position[1] = -1; joints[(int)R15Part::RightLowerArm].position[2] = 0;
        joints[(int)R15Part::RightLowerArm].parentIndex = (int)R15Part::RightUpperArm;
        
        // Legs
        joints[(int)R15Part::LeftUpperLeg].name = "LeftUpperLeg";
        joints[(int)R15Part::LeftUpperLeg].position[0] = -0.5f; joints[(int)R15Part::LeftUpperLeg].position[1] = -1; joints[(int)R15Part::LeftUpperLeg].position[2] = 0;
        joints[(int)R15Part::LeftUpperLeg].parentIndex = (int)R15Part::Torso;
        joints[(int)R15Part::LeftUpperLeg].childCount = 1;
        joints[(int)R15Part::LeftUpperLeg].childIndices[0] = (int)R15Part::LeftLowerLeg;
        
        joints[(int)R15Part::RightUpperLeg].name = "RightUpperLeg";
        joints[(int)R15Part::RightUpperLeg].position[0] = 0.5f; joints[(int)R15Part::RightUpperLeg].position[1] = -1; joints[(int)R15Part::RightUpperLeg].position[2] = 0;
        joints[(int)R15Part::RightUpperLeg].parentIndex = (int)R15Part::Torso;
        joints[(int)R15Part::RightUpperLeg].childCount = 1;
        joints[(int)R15Part::RightUpperLeg].childIndices[0] = (int)R15Part::RightLowerLeg;
        
        joints[(int)R15Part::LeftLowerLeg].name = "LeftLowerLeg";
        joints[(int)R15Part::LeftLowerLeg].position[0] = 0; joints[(int)R15Part::LeftLowerLeg].position[1] = -1; joints[(int)R15Part::LeftLowerLeg].position[2] = 0;
        joints[(int)R15Part::LeftLowerLeg].parentIndex = (int)R15Part::LeftUpperLeg;
        joints[(int)R15Part::LeftLowerLeg].childCount = 1;
        joints[(int)R15Part::LeftLowerLeg].childIndices[0] = (int)R15Part::LeftFoot;
        
        joints[(int)R15Part::RightLowerLeg].name = "RightLowerLeg";
        joints[(int)R15Part::RightLowerLeg].position[0] = 0; joints[(int)R15Part::RightLowerLeg].position[1] = -1; joints[(int)R15Part::RightLowerLeg].position[2] = 0;
        joints[(int)R15Part::RightLowerLeg].parentIndex = (int)R15Part::RightUpperLeg;
        joints[(int)R15Part::RightLowerLeg].childCount = 1;
        joints[(int)R15Part::RightLowerLeg].childIndices[0] = (int)R15Part::RightFoot;
        
        joints[(int)R15Part::LeftFoot].name = "LeftFoot";
        joints[(int)R15Part::LeftFoot].position[0] = 0; joints[(int)R15Part::LeftFoot].position[1] = -1; joints[(int)R15Part::LeftFoot].position[2] = 0;
        joints[(int)R15Part::LeftFoot].parentIndex = (int)R15Part::LeftLowerLeg;
        
        joints[(int)R15Part::RightFoot].name = "RightFoot";
        joints[(int)R15Part::RightFoot].position[0] = 0; joints[(int)R15Part::RightFoot].position[1] = -1; joints[(int)R15Part::RightFoot].position[2] = 0;
        joints[(int)R15Part::RightFoot].parentIndex = (int)R15Part::RightLowerLeg;
        
        return joints;
    }

    R15Character* createDefault() {
        R15Character* ch = new R15Character();
        
        ch->joints = createSkeleton();
        ch->playerName = "Player";
        
        // Create meshes for each body part
        ch->meshes.push_back(createBoxMesh(R15Part::Head, 1, 1, 1, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::Torso, 2, 2, 1, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::LeftUpperArm, 0.5f, 1, 0.5f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::RightUpperArm, 0.5f, 1, 0.5f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::LeftLowerArm, 0.5f, 1, 0.5f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::RightLowerArm, 0.5f, 1, 0.5f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::LeftUpperLeg, 0.6f, 1, 0.6f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::RightUpperLeg, 0.6f, 1, 0.6f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::LeftLowerLeg, 0.6f, 1, 0.6f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::RightLowerLeg, 0.6f, 1, 0.6f, 0, 0, 0));
        ch->meshes.push_back(createBoxMesh(R15Part::LeftFoot, 0.6f, 0.5f, 1, 0, 0, 0.2f));
        ch->meshes.push_back(createBoxMesh(R15Part::RightFoot, 0.6f, 0.5f, 1, 0, 0, 0.2f));
        
        // Default state
        memset(ch->jointRotation, 0, sizeof(ch->jointRotation));
        memset(ch->jointPosition, 0, sizeof(ch->jointPosition));
        ch->isWalking = false;
        ch->isRunning = false;
        ch->isJumping = false;
        ch->isDead = false;
        ch->walkCycle = 0;
        
        // Default skin color (yellow)
        ch->skinColor = 0xFFFFCC00;
        for (int i = 0; i < (int)R15Part::Count; i++) {
            ch->bodyColors[i] = ch->skinColor;
        }
        
        // Torso and legs default to blue pants
        ch->bodyColors[(int)R15Part::Torso] = 0xFF0000AA;
        ch->bodyColors[(int)R15Part::LeftUpperLeg] = 0xFF0000AA;
        ch->bodyColors[(int)R15Part::RightUpperLeg] = 0xFF0000AA;
        ch->bodyColors[(int)R15Part::LeftLowerLeg] = 0xFF0000AA;
        ch->bodyColors[(int)R15Part::RightLowerLeg] = 0xFF0000AA;
        ch->bodyColors[(int)R15Part::LeftFoot] = 0xFF0000AA;
        ch->bodyColors[(int)R15Part::RightFoot] = 0xFF0000AA;
        
        // Position
        ch->worldPosition[0] = 0; ch->worldPosition[1] = 0; ch->worldPosition[2] = 0;
        ch->worldRotation[0] = 0; ch->worldRotation[1] = 0; ch->worldRotation[2] = 0;
        ch->worldScale[0] = 1; ch->worldScale[1] = 1; ch->worldScale[2] = 1;
        
        // Bounding box
        ch->minX = -1; ch->maxX = 1;
        ch->minY = 0; ch->maxY = 4;
        ch->minZ = -0.5f; ch->maxZ = 0.5f;
        
        ch->rendered = false;
        memset(ch->cachedTextures, 0, sizeof(ch->cachedTextures));
        
        return ch;
    }

    R15Character* loadFromRBXM(const char* rbxmPath) {
        R15Character* ch = createDefault();
        FILE* f = fopen(rbxmPath, "rb");
        if (f) {
            Node* node = parseRBXMb(f, false);
            fclose(f);
            if (node && node->children.size() > 0) {
                for (Node* child : node->children) {
                    if (strcmp(child->type, "Mesh") == 0) {
                        ch->customMesh = true;
                        break;
                    }
                }
            }
        }
        return ch;
    }

    void setBodyColor(R15Character* ch, R15Part part, u32 color) {
        ch->bodyColors[(int)part] = color;
    }

    void setSkinColor(R15Character* ch, u32 color) {
        ch->skinColor = color;
    }

    void setShirtTexture(R15Character* ch, const char* path) {
        ch->shirtTexture = path;
    }

    void setPantsTexture(R15Character* ch, const char* path) {
        ch->pantsTexture = path;
    }

    void setFaceTexture(R15Character* ch, const char* path) {
        ch->faceTexture = path;
    }

    void setHatPath(R15Character* ch, const char* path) {
        ch->hatPath = path;
    }

    void setPosition(R15Character* ch, float x, float y, float z) {
        ch->worldPosition[0] = x;
        ch->worldPosition[1] = y;
        ch->worldPosition[2] = z;
    }

    void setRotation(R15Character* ch, float x, float y, float z) {
        ch->worldRotation[0] = x;
        ch->worldRotation[1] = y;
        ch->worldRotation[2] = z;
    }

    void updateAnimation(R15Character* ch, float deltaTime, bool isMoving) {
        if (!ch || ch->isDead) return;
        
        ch->walkCycle += deltaTime * (isMoving ? 5.0f : 0.0f);
        if (ch->walkCycle > 1.0f) ch->walkCycle -= 1.0f;
        ch->walkCycle = fmod(ch->walkCycle, 1.0f);
        
        if (isMoving) {
            ch->isWalking = true;
            float swing = sinf(ch->walkCycle * M_PI * 2.0f) * 0.5f;
            
            // Leg swing
            ch->jointRotation[(int)R15Part::LeftUpperLeg][0] = swing * 0.8f;
            ch->jointRotation[(int)R15Part::RightUpperLeg][0] = -swing * 0.8f;
            ch->jointRotation[(int)R15Part::LeftLowerLeg][0] = fmaxf(0, -swing) * 0.4f;
            ch->jointRotation[(int)R15Part::RightLowerLeg][0] = fmaxf(0, swing) * 0.4f;
            
            // Arm swing (opposite to legs)
            ch->jointRotation[(int)R15Part::LeftUpperArm][0] = -swing * 0.6f;
            ch->jointRotation[(int)R15Part::RightUpperArm][0] = swing * 0.6f;
            ch->jointRotation[(int)R15Part::LeftLowerArm][0] = -0.2f;
            ch->jointRotation[(int)R15Part::RightLowerArm][0] = -0.2f;
        } else {
            ch->isWalking = false;
            // Idle pose - slowly return to rest
            for (int i = 0; i < (int)R15Part::Count; i++) {
                ch->jointRotation[i][0] *= 0.9f;
                ch->jointRotation[i][1] *= 0.9f;
                ch->jointRotation[i][2] *= 0.9f;
            }
            // Default arm hang
            ch->jointRotation[(int)R15Part::LeftUpperArm][0] = 0.1f;
            ch->jointRotation[(int)R15Part::RightUpperArm][0] = 0.1f;
            ch->jointRotation[(int)R15Part::LeftLowerArm][0] = -0.3f;
            ch->jointRotation[(int)R15Part::RightLowerArm][0] = -0.3f;
        }
    }

    // Helper: multiply matrices (4x4)
    static void matMul(float out[16], const float a[16], const float b[16]) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                out[i * 4 + j] = 0;
                for (int k = 0; k < 4; k++) {
                    out[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
                }
            }
        }
    }

    // Helper: create rotation matrix from Euler angles
    static void rotMatrix(float out[16], float rx, float ry, float rz) {
        memset(out, 0, sizeof(float) * 16);
        out[0] = out[5] = out[10] = out[15] = 1;
        
        float cx = cosf(rx), sx = sinf(rx);
        float cy = cosf(ry), sy = sinf(ry);
        float cz = cosf(rz), sz = sinf(rz);
        
        float m[16];
        matMul(m, out, out);
        
        // X rotation
        float rxm[16] = {1,0,0,0, 0,cx,-sx,0, 0,sx,cx,0, 0,0,0,1};
        matMul(out, m, rxm);
        
        // Y rotation
        float rym[16] = {cy,0,sy,0, 0,1,0,0, -sy,0,cy,0, 0,0,0,1};
        matMul(out, out, rym);
        
        // Z rotation
        float rzm[16] = {cz,-sz,0,0, sz,cz,0,0, 0,0,1,0, 0,0,0,1};
        matMul(out, out, rzm);
    }

    // Render each part mesh
    static void renderPart(R15Character* ch, PartMesh& mesh, const float transform[16], u32 color) {
        if (mesh.vertices.empty() || mesh.indices.empty()) return;
        
        // Upload mesh to GPU via CitroRenderer
        // Transform vertices by joint matrix
        std::vector<float> transformed(mesh.vertices.size());
        for (size_t i = 0; i < mesh.vertices.size() / 3; i++) {
            float x = mesh.vertices[i * 3];
            float y = mesh.vertices[i * 3 + 1];
            float z = mesh.vertices[i * 3 + 2];
            
            // Apply transform matrix
            transformed[i * 3]     = transform[0]*x + transform[4]*y + transform[8]*z + transform[12];
            transformed[i * 3 + 1] = transform[1]*x + transform[5]*y + transform[9]*z + transform[13];
            transformed[i * 3 + 2] = transform[2]*x + transform[6]*y + transform[10]*z + transform[14];
        }
        
        // Color from u32 to RGBA
        float r = (color >> 16) & 0xFF;
        float g = (color >> 8) & 0xFF;
        float b = color & 0xFF;
        
        // Draw using CitroRenderer (simplified - assumes drawColoredMesh exists)
        // For now, use drawBox as placeholder
        float cx = transform[12], cy = transform[13], cz = transform[14];
        CitroRenderer::setColor(r / 255.0f, g / 255.0f, b / 255.0f);
    }

    void render(R15Character* ch) {
        if (!ch) return;
        
        // Render each body part
        for (auto& mesh : ch->meshes) {
            int partIdx = (int)mesh.part;
            
            // Calculate joint world transform
            float partTransform[16];
            rotMatrix(partTransform, 
                ch->jointRotation[partIdx][0],
                ch->jointRotation[partIdx][1],
                ch->jointRotation[partIdx][2]);
            
            // Add world position
            partTransform[12] += ch->worldPosition[0] + ch->joints[partIdx].position[0];
            partTransform[13] += ch->worldPosition[1] + ch->joints[partIdx].position[1];
            partTransform[14] += ch->worldPosition[2] + ch->joints[partIdx].position[2];
            
            renderPart(ch, mesh, partTransform, ch->bodyColors[partIdx]);
        }
    }

    void setPose(R15Character* ch, const float rotations[(int)R15Part::Count][3]) {
        for (int i = 0; i < (int)R15Part::Count; i++) {
            ch->jointRotation[i][0] = rotations[i][0];
            ch->jointRotation[i][1] = rotations[i][1];
            ch->jointRotation[i][2] = rotations[i][2];
        }
    }

    void getJointWorldTransform(R15Character* ch, R15Part part, float outPos[3], float outRot[3]) {
        int idx = (int)part;
        outPos[0] = ch->joints[idx].position[0] + ch->worldPosition[0];
        outPos[1] = ch->joints[idx].position[1] + ch->worldPosition[1];
        outPos[2] = ch->joints[idx].position[2] + ch->worldPosition[2];
        outRot[0] = ch->jointRotation[idx][0] + ch->worldRotation[0];
        outRot[1] = ch->jointRotation[idx][1] + ch->worldRotation[1];
        outRot[2] = ch->jointRotation[idx][2] + ch->worldRotation[2];
    }

    void getBoundingBox(R15Character* ch, float out[6]) {
        out[0] = ch->minX + ch->worldPosition[0];
        out[1] = ch->maxX + ch->worldPosition[0];
        out[2] = ch->minY + ch->worldPosition[1];
        out[3] = ch->maxY + ch->worldPosition[1];
        out[4] = ch->minZ + ch->worldPosition[2];
        out[5] = ch->maxZ + ch->worldPosition[2];
    }

    void freeCharacter(R15Character* ch) {
        if (!ch) return;
        for (int i = 0; i < 8; i++) {
            if (ch->cachedTextures[i]) {
                C3D_TexDelete(ch->cachedTextures[i]);
            }
        }
        delete ch;
    }

    void init() {
        // Preload default character textures
    }

    void shutdown() {
        // Free all cached textures
    }
}