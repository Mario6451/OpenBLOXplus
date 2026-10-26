#include "mesh_loader.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static inline std::string trim(const std::string &s){ size_t a=s.find_first_not_of(" \t\r\n"); if(a==std::string::npos) return ""; size_t b=s.find_last_not_of(" \t\r\n"); return s.substr(a,b-a+1); }

MeshData MeshLoader::loadMeshFromFile(const std::string &path) {
	MeshData mesh;
	FILE *f = fopen(path.c_str(), "r");
	if (!f) return mesh;
	// Try naive OBJ parsing
	char line[1024];
	std::vector<float> temp_v;
	std::vector<float> temp_vn;
	std::vector<float> temp_vt;
	while (fgets(line, sizeof(line), f)) {
		std::string s = trim(line);
		if (s.size() == 0) continue;
		if (s[0]=='#') continue;
		if (s.rfind("v ",0)==0) {
			float x,y,z; if (sscanf(s.c_str()+2, "%f %f %f", &x,&y,&z)==3) { temp_v.push_back(x); temp_v.push_back(y); temp_v.push_back(z); }
		} else if (s.rfind("vn ",0)==0) {
			float x,y,z; if (sscanf(s.c_str()+3, "%f %f %f", &x,&y,&z)==3) { temp_vn.push_back(x); temp_vn.push_back(y); temp_vn.push_back(z); }
		} else if (s.rfind("vt ",0)==0) {
			float u,v; if (sscanf(s.c_str()+3, "%f %f", &u,&v)==2) { temp_vt.push_back(u); temp_vt.push_back(v); }
		} else if (s.rfind("f ",0)==0) {
			// parse faces like f v/vt/vn ...
			int vi[4], vti[4], vni[4]; int cnt=0;
			const char *p = s.c_str()+2; char buf[128];
			while (*p && cnt<4) {
				int read=0;
				int a=0,b=0,c=0;
				if (sscanf(p, "%d/%d/%d %n", &a,&b,&c,&read) >= 1) {
					vi[cnt]=a; vti[cnt]=b; vni[cnt]=c; cnt++; p+=read; continue;
				}
				if (sscanf(p, "%d//%d %n", &a,&c,&read) >=1) { vi[cnt]=a; vni[cnt]=c; vti[cnt]=0; cnt++; p+=read; continue; }
				if (sscanf(p, "%d/%d %n", &a,&b,&read) >=1) { vi[cnt]=a; vti[cnt]=b; vni[cnt]=0; cnt++; p+=read; continue; }
				if (sscanf(p, "%d %n", &a,&read) >=1) { vi[cnt]=a; vti[cnt]=0; vni[cnt]=0; cnt++; p+=read; continue; }
				break;
			}
			if (cnt>=3) {
				int vertCount = (int)(temp_v.size() / 3);
				for (int i=0;i<cnt-2;i++) {
					int idx0=vi[0]-1, idx1=vi[i+1]-1, idx2=vi[i+2]-1;
					if (idx0 < 0 || idx1 < 0 || idx2 < 0 || idx0 >= vertCount || idx1 >= vertCount || idx2 >= vertCount) continue;
					mesh.vertices.push_back(temp_v[idx0*3+0]); mesh.vertices.push_back(temp_v[idx0*3+1]); mesh.vertices.push_back(temp_v[idx0*3+2]);
					mesh.vertices.push_back(temp_v[idx1*3+0]); mesh.vertices.push_back(temp_v[idx1*3+1]); mesh.vertices.push_back(temp_v[idx1*3+2]);
					mesh.vertices.push_back(temp_v[idx2*3+0]); mesh.vertices.push_back(temp_v[idx2*3+1]); mesh.vertices.push_back(temp_v[idx2*3+2]);
					uint32_t base = (uint32_t)(mesh.vertices.size()/3 - 3);
					mesh.indices.push_back(base); mesh.indices.push_back(base+1); mesh.indices.push_back(base+2);
				}
			}
		}
	}
	fclose(f);
	// If no vertices found, return empty mesh
	return mesh;
}

// Upload mesh to GPU. Vertex format: xyz(3f) + uv(2f) + rgba(4f) = 9 floats per vertex
GPU_Mesh* MeshLoader::uploadMeshToGPU(const MeshData &m) {
	if (m.vertices.empty() || m.indices.empty()) return nullptr;

	int num_verts = (int)(m.vertices.size() / 3);
	int vertex_size = 9 * sizeof(float); // xyz(3) + uv(2) + rgba(4)

	// Pack vertex data
	float *packed = (float*)malloc(num_verts * vertex_size);
	if (!packed) return nullptr;

	for (int i = 0; i < num_verts; i++) {
		int dst = i * 9;
		// XYZ
		packed[dst + 0] = m.vertices[i * 3 + 0];
		packed[dst + 1] = m.vertices[i * 3 + 1];
		packed[dst + 2] = m.vertices[i * 3 + 2];
		// UV (use stored UVs or defaults)
		if (i * 2 + 1 < (int)m.uvs.size()) {
			packed[dst + 3] = m.uvs[i * 2 + 0];
			packed[dst + 4] = m.uvs[i * 2 + 1];
		} else {
			packed[dst + 3] = 0.0f;
			packed[dst + 4] = 0.0f;
		}
		// RGBA (white, opaque)
		packed[dst + 5] = 1.0f;
		packed[dst + 6] = 1.0f;
		packed[dst + 7] = 1.0f;
		packed[dst + 8] = 1.0f;
	}

	GPU_Mesh *gpu = (GPU_Mesh*)malloc(sizeof(GPU_Mesh));
	if (!gpu) { free(packed); return nullptr; }
	gpu->num_indices = (int)m.indices.size();
	gpu->use_indices = true;

	// Allocate raw FRCDRAM for vertex buffer
	gpu->vbo = linearAlloc(num_verts * vertex_size);
	if (!gpu->vbo) { free(packed); free(gpu); return nullptr; }
	memcpy(gpu->vbo, packed, num_verts * vertex_size);
	free(packed);

	// Allocate raw FRCDRAM for index buffer
	gpu->ibo = linearAlloc(m.indices.size() * sizeof(uint32_t));
	if (!gpu->ibo) { linearFree(gpu->vbo); free(gpu); return nullptr; }
	memcpy(gpu->ibo, m.indices.data(), m.indices.size() * sizeof(uint32_t));

	return gpu;
}

void MeshLoader::freeGPUMesh(GPU_Mesh *mesh) {
	if (!mesh) return;
	linearFree(mesh->vbo);
	linearFree(mesh->ibo);
	free(mesh);
}

