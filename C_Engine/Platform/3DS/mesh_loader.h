#pragma once

#include <string>
#include <vector>
#include <3ds.h>
#include <citro3d.h>

struct MeshData {
	std::vector<float> vertices; // x,y,z
	std::vector<float> normals;  // nx,ny,nz
	std::vector<float> uvs;      // u,v
	std::vector<uint32_t> indices;
};

struct GPU_Mesh {
	void* vbo;      // Vertex buffer: xyz(3f) + uv(2f) + rgba(4f)
	void* ibo;      // Index buffer
	int num_indices;     // Number of indices
	bool use_indices;    // True if indexed
};

namespace MeshLoader {
	// Try to load legacy Roblox .mesh or fallback formats. Best-effort OBJ support included.
	MeshData loadMeshFromFile(const std::string &path);

	// Upload mesh to GPU. Returns nullptr on failure.
	GPU_Mesh* uploadMeshToGPU(const MeshData &m);

	// Free GPU mesh resources
	void freeGPUMesh(GPU_Mesh *mesh);
}
