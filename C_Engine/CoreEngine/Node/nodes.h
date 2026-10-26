/*
    OpenBLOX nodes
*/

#ifndef _NODES_H_
#define _NODES_H_

#include "../../ThirdParty/TinyPhysicsEngine/tinyphysicsengine.h"
#include <unordered_map>
#include <string>
void render_cube_transform(float x, float y, float z, float rx, float ry, float rz, float sx, float sy, float sz, int color); 

class vec3 {
    public:
        float x, y, z;
}; typedef vec3 Vector3;
inline void set_vec3(vec3* vector3, float x, float y, float z) {
    vector3->x = x;
    vector3->y = y;
    vector3->z = z;
}

#include <cmath>

class CFrame {
    public:
        float X,Y,Z,R00,R01,R02,R10,R11,R12,R20,R21,R22;

    vec3 toEulerAngles()
    {
        vec3 ret;
       
        // Extract angles assuming YXZ rotation order
        float sy = -R20;
    
        if (fabs(sy) < 1.0f) // Standard case
        {
            ret.x = atan2(R21, R22) * (180.0 / M_PI);   // Roll
            ret.y = asin(sy)        * (180.0 / M_PI);   // Pitch
            ret.z = atan2(R10, R00) * (180.0 / M_PI);   // Yaw
        }
        else // Gimbal lock case
        {
            ret.x = 0; // Roll is undefined, set to zero
            ret.y = ((sy > 0) ? M_PI / 2 : -M_PI / 2) * (180.0 / M_PI); // ±90 degrees
            ret.z = atan2(-R01, R11)                  * (180.0 / M_PI); // Yaw
        }

        return ret; 
    }

    vec3 position()
    {
        vec3 ret;

        ret.x = X;
        ret.y = Y;
        ret.z = Z;

        return ret;
    }

};
typedef CFrame CoordinateFrame;

// Color3 - RGB with 0-1 float range (2007+)
class Color3Value {
    public:
        float r, g, b;
        Color3Value() : r(0), g(0), b(0) {}
        Color3Value(float red, float green, float blue) : r(red), g(green), b(blue) {}
        
        // Clamp values to 0-1
        void clamp() {
            r = r < 0 ? 0 : (r > 1 ? 1 : r);
            g = g < 0 ? 0 : (g > 1 ? 1 : g);
            b = b < 0 ? 0 : (b > 1 ? 1 : b);
        }
        
        // Convert to integer color (0x00RRGGBB)
        int toInt() {
            return (int(r * 255) << 16) | (int(g * 255) << 8) | int(b * 255);
        }
};

// BrickColor - Classic Roblox color palette (255 colors + Color3)
// Each BrickColor has a number and display name, maps to RGBA
struct BrickColorEntry {
    const char* name;
    int r, g, b; // 0-255
};

// Full BrickColor palette - returns entry by number (1-255 classic, 0 = use Color3)
const BrickColorEntry* getBrickColor(int number);

// UDim - for GUI (2012+)
struct UDim {
    float Scale;
    int Offset;
    UDim() : Scale(0), Offset(0) {}
};

// UDim2 - 2D size/position for GUI
struct UDim2 {
    UDim X, Y;
};

// Vector3int16 - integer vector for GUI sizes
struct Vector3int16 {
    short X, Y, Z;
    Vector3int16() : X(0), Y(0), Z(0) {}
};

void render_cf(CFrame cf, vec3 size, int color, float transparency, int shape);

#include <vector>

// Base Node
class Node {
public:
    virtual ~Node() {}
    char name[64];
    char type[32];
    Node* parent;
    std::vector<Node*> children;

    bool locked = false;
    bool anchored = true;
    bool raycastAble = true; // Blockland Specific, ROBLOX Levels will ignore this
    bool canCollide = true;
    bool visible = true;
    
    // Script properties
    char* source = nullptr;           // Lua source code
    int moduleResultRef = -1;         // Cached module result reference
    
    // Sound properties
    bool playing = false;
    float soundPitch = 1.0f;
    float soundTimePosition = 0.0f;
    bool soundPaused = false;
    float soundVolume = 1.0f;
    char* soundId = nullptr;
    
    // Physics/animation
    float health = 100.0f;
    float maxHealth = 100.0f;
    float walkSpeed = 16.0f;
    float jumpPower = 50.0f;
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float size[3] = {1, 1, 1};
    float transparency = 0.0f;
    std::unordered_map<std::string, double> attributes;

    void SetParent(Node *newParent)
    {
        parent = newParent;
        newParent->children.push_back(this);
    }
};

// Part
enum Material {
    Material_Plastic = 0,
    Material_Wood = 1,
    Material_Slate = 2,
    Material_CoronaMarble = 3,
    Material_Cement = 4,
    Material_AsgardMarble = 5,
    Material_Water = 6,
    Material_Grass = 7,
    Material_Rubber = 8,
    Material_Glacier = 9,
    Material_WolfMarble = 10,
    Material_Sand = 11,
    Material_ForestMarble = 12,
    Material_Snow = 13,
    Material_LegacyPlastic = 14,
    Material_GraniteMarble = 16,
    Material_Sandstone = 18,
    Material_AzurePride = 19,
    Material_CrackedLava = 20,
    Material_CorrugatedPlastic = 21,
    Material_Neon = 22,
    Material_Gravel = 23,
    Material_Essence = 24,
    Material_Pebble = 25,
    Material_Tile = 26,
    Material_Airborne = 27,
    Material_Fire = 29,
    Material_LavaGlow = 30,
    Material_Magma = 31,
    Material_Limestone = 32,
    Material_Fabrics = 33,
    Material_Silver = 34,
    Material_Foil = 35,
    Material_GumDrop = 36,
    Material_Glass = 37,
    Material_Marshmallow = 38,
    Material_DiamondPlate = 39,
};

enum SurfaceType {
    Surface_Smooth = 0,
    Surface_Weld = 1,
    Surface_Studs = 2,
    Surface_SmoothNoOutlines = 3,
    Surface_None = 4,
    Surface_Halogen = 5,
    Surface_Corners = 6,
    Surface_NoRingCorner = 7,
    Surface_NoRingSmooth = 8,
    Surface_NoRingStuds = 9,
    Surface_VectorXY = 10,
    Surface_VectorZX = 11,
    Surface_VectorYZ = 12,
    Surface_Hinge = 13,
    Surface_Motor = 14,
    Surface_SteppingMotor = 15,
    Surface_Inlet = 16,
    Surface_Universal = 24,
    Surface_Bumps = 25,
    Surface_Grooves = 26,
    Surface_Unjoinable = 27,
    Surface_Spawn = 28,
};

class Part : public Node {
public:
    int color = 2;
    Color3Value color3;              // Color3 override (2012+)
    int brickColor;                  // BrickColor number (0 = use Color3)
    float transparency = 0;
    int shape; // Maybe make -1 force the engine to find a SpecialMesh?
    int material;                    // Material enum (2012+)
    int reflectance;                 // 0-100 (2012+)
    int lightEmission;               // 0-100 (2012+)
    int textureID;                   // Texture ID from catalog (2012+)
    int textureID2;                  // Secondary texture (2015+)
    SurfaceType surfaceTypes[6];     // Front, Back, Left, Right, Top, Bottom
    char *texture;                   // Texture filename
    char *meshId;                    // MeshId for MeshPart
    int partMode = 0; // 0 = ROBLOX, 1 = BlockLand
    vec3 scale;
    CFrame cf;

    TPE_Joint joints[16];
    TPE_Connection connections[32];
    TPE_Body body;
    bool hasBeenPositioned = false;
    
    void render() {
        render_cf(cf, scale, color, transparency, shape);
        
        if (anchored) {
            TPE_bodyMoveTo(&body,TPE_vec3(cf.X*512,cf.Y*512,cf.Z*512));
        } else {
            if (!hasBeenPositioned) {
                TPE_bodyMoveTo(&body,TPE_vec3(cf.X*512,cf.Y*512,cf.Z*512));
                hasBeenPositioned = true;
            } else {
                TPE_bodyApplyGravity(&body,TPE_F / 100);
            }
        }
        TPE_Vec3 position = TPE_bodyGetCenterOfMass(&body);
        cf.X = position.x/512.0;
        cf.Y = position.y/512.0;
        cf.Z = position.z/512.0;
    }
    void makePhysicsPart() {
        TPE_makeBox(joints, connections, scale.x*512, scale.y*512, scale.z*512, 0.1);
        vec3 rotation = cf.toEulerAngles();
        TPE_bodyInit(&body, joints, 16, connections, 32, 1);
        TPE_bodyRotateByAxis(&body, TPE_vec3(rotation.x*512, rotation.y*512, rotation.z*512));
        TPE_bodyMoveTo(&body,TPE_vec3(cf.X*512,cf.Y*512,cf.Z*512));
    }
};

// SpawnLocation
class SpawnLocation : public Part {
public:
    int teamColor = 2;
};

// MeshPart - Part with custom mesh (2011+)
class MeshPart : public Part {
public:
    char *meshId;            // Mesh asset ID
    vec3 meshScale;         // Mesh scale
    vec3 meshOffset;        // Mesh offset
    int meshType;           // 0=FileMesh, 1=Sphere, 2=Cylinder, 3=Block
};

// Decal - Image on surface (2007+)
class Decal : public Node {
public:
    char *texture;          // Texture asset ID
    CFrame cf;
    float transparency;
    int face;               // Which face (0=Front, 1=Back, 2=Left, 3=Right, 4=Top, 5=Bottom)
};

// Script - Lua code (2007+)
class Script : public Node {
public:
    char *source;           // Lua source code
    bool runContext;        // 0=Client, 1=Server
};

// Model - Container for multiple parts
class Model : public Node {
public:
    // Models don't have additional properties, just container
};

// Weld - Joins two parts (2007+)
class Weld : public Node {
public:
    char *part0;            // First part name
    char *part1;            // Second part name
    CFrame cframe0;         // Offset for first part
    CFrame cframe1;         // Offset for second part
};

// Sound - Audio playback (2007+)
class Sound : public Node {
public:
    char *sound;            // Sound asset ID
    float volume;           // 0-10
    float pitch;            // 0.25-4
    bool playing;           // Is playing
    float timePosition;     // Current playback time
};

// ParticleEmitter - Visual effects (2009+)
class ParticleEmitter : public Node {
public:
    char *texture;          // Particle texture
    float speed;            // Particle speed
    float lifetime;         // Particle lifetime
    int count;              // Number of particles
    float spreadAngle;      // Spread angle in degrees
    float rotation;         // Particle rotation
    float drift;            // Drift force
    float wind;             // Wind force
    float gravity;          // Gravity force
    float emissorShape;     // 0=Sphere, 1=Box
};

// PointLight - Omnidirectional light (2011+)
class PointLight : public Node {
public:
    float brightness;       // Light brightness (0-3)
    float range;            // Light range
    Color3Value color;      // Light color
    bool shade;             // Cast shadows
};

// SpotLight - Directional light (2011+)
class SpotLight : public Node {
public:
    float brightness;       // Light brightness (0-3)
    float range;            // Light range
    float focus;            // Spotlight focus (0-100)
    Color3Value color;      // Light color
    bool shade;             // Cast shadows
    int face;               // Which face it points to
};

// BillboardGui - 3D UI (2015+)
class BillboardGui : public Node {
public:
    UDim2 size;             // GUI size
    UDim2 stroke;           // Stroke offset
    float sizeOffset;       // Size offset
    float extentsOffset;    // Extents offset
    bool alwaysOnTop;       // Always visible
    Node *adornee;          // Adornee (what it follows)
};

// ForceField - Energy effects (2007+)
class ForceField : public Node {
public:
    int face;               // Which face to apply to
    bool showOnTop;         // Show on top of parts
};

// Attachment - For constraints (2015+)
class Attachment : public Node {
public:
    CFrame cf;              // Attachment position
    vec3 position;          // Local position
};

// AlignPosition - Constraint (2015+)
class AlignPosition : public Node {
public:
    float rigidity;         // Constraint rigidity
    float maxForce;         // Maximum force
    float maxVelocity;      // Maximum velocity
};

// AlignOrientation - Constraint (2015+)
class AlignOrientation : public Node {
public:
    float rigidity;         // Constraint rigidity
    float maxTorque;        // Maximum torque
    float maxAngularVelocity; // Maximum angular velocity
};

// Region3 - Damage/healing zone (2011+)
class RegionPart : public Part {
public:
    int regionType;         // 0=Damage, 1=Heal, 2=ForceField
    float damagePerSecond;  // Damage rate
    float healPerSecond;    // Heal rate
};

// Shitty world render function, will be slow, but will hopefully work if shit doesn't break.
inline void renderWorld(Node *world) {
    for (int i = 0; i < world->children.size(); i++) {
        Node* node = world->children[i];
        
        // Attempt Cast.
        Part* part = dynamic_cast<Part*>(node);
        if (part != nullptr) {
            //printf("Render part\n");
            part->render(); // Part exists, render
        }

        renderWorld(node);
    }
}

#endif // _NODES_H_

