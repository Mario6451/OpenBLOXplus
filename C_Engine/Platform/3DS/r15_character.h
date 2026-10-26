#pragma once
#include <string>
#include <vector>
#include <map>
#include <3ds.h>
#include <citro3d.h>
#include <citro3d.h>

// R15 Character Model Support for OpenBLOX
// Loads and renders Roblox R15 character models from RBXM files

namespace R15Character {

    // R15 body parts (13 parts total)
    enum class R15Part {
        Head,
        Torso,
        LeftUpperArm,
        RightUpperArm,
        LeftLowerArm,
        RightLowerArm,
        LeftUpperLeg,
        RightUpperLeg,
        LeftLowerLeg,
        RightLowerLeg,
        LeftFoot,
        RightFoot,
        Neck,
        Count
    };

    // Joint for skeletal animation
    struct Joint {
        std::string name;
        float position[3];      // Local position
        float rotation[3];      // Local rotation (Euler XYZ)
        float scale[3];         // Local scale
        int parentIndex;        // Parent joint index (-1 for root)
        int childCount;
        int childIndices[8];    // Max 8 children per joint
    };

    // Mesh for a body part
    struct PartMesh {
        R15Part part;
        std::string name;
        std::vector<float> vertices;      // Position data (x, y, z)
        std::vector<float> normals;       // Normal data (nx, ny, nz)
        std::vector<float> uv;           // Texture coordinates (u, v)
        std::vector<u16> indices;        // Index buffer
        std::string texturePath;         // Texture file path
        bool isSkin;                     // Whether this is skin or clothing
    };

    // R15 Character instance
    struct R15Character {
        std::vector<Joint> joints;       // Skeleton
        std::vector<PartMesh> meshes;    // Body meshes
        std::string playerName;          // Owner name
        
        // Animation state
        float jointRotation[(int)R15Part::Count][3];  // Current joint rotations
        float jointPosition[(int)R15Part::Count][3];  // Current joint positions
        
        // Pose state
        bool isWalking;
        bool isRunning;
        bool isJumping;
        bool isDead;
        float walkCycle;  // Animation cycle (0-1)
        
        // Visual settings
        u32 bodyColors[(int)R15Part::Count];  // BrickColor for each part
        u32 skinColor;                         // Default skin color
        std::string shirtTexture;             // Shirt texture path
        std::string pantsTexture;             // Pants texture path
        std::string faceTexture;              // Face texture path
        std::string hatPath;                  // Hat RBXM path
        
        // Bounding box for collision
        float minX, maxX, minY, maxY, minZ, maxZ;
        
        // Transform
        float worldPosition[3];
        float worldRotation[3];
        float worldScale[3];
        
        // Rendering
        bool rendered;
    bool customMesh = false;
        C3D_Tex* cachedTextures[8];
    };

    // Load R15 character from RBXM file
    R15Character* loadFromRBXM(const char* rbxmPath);

    // Create default R15 character (Roblox default)
    R15Character* createDefault();

    // Set character body colors
    void setBodyColor(R15Character* character, R15Part part, u32 color);
    void setSkinColor(R15Character* character, u32 color);
    
    // Set clothing
    void setShirtTexture(R15Character* character, const char* path);
    void setPantsTexture(R15Character* character, const char* path);
    void setFaceTexture(R15Character* character, const char* path);
    void setHatPath(R15Character* character, const char* path);

    // Set character position and rotation
    void setPosition(R15Character* character, float x, float y, float z);
    void setRotation(R15Character* character, float x, float y, float z);
    
    // Update animation (call every frame)
    void updateAnimation(R15Character* character, float deltaTime, bool isMoving);
    
    // Render character using Citro3D
    void render(R15Character* character);
    
    // Set animation pose
    void setPose(R15Character* character, const float jointRotations[(int)R15Part::Count][3]);
    
    // Get joint world transform
    void getJointWorldTransform(R15Character* character, R15Part part, float outPos[3], float outRot[3]);
    
    // Get bounding box
    void getBoundingBox(R15Character* character, float out[6]);
    
    // Free character memory
    void freeCharacter(R15Character* character);
    
    // Initialize R15 character system
    void init();
    
    // Shutdown
    void shutdown();
}