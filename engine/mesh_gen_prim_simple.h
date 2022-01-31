#pragma once

#include "engine/mesh.h"

namespace arctic {

bool Mesh_GeneratePlane(Mesh *me, const MeshVertexFormat *vertexFormat, int gx, int gy);
bool Mesh_GenerateCube(Mesh *me, const MeshVertexFormat *vertexFormat);
bool Mesh_GenerateCubePlanes(Mesh *me, const MeshVertexFormat *vertexFormat);
bool Mesh_GenerateTorus(Mesh *me, const MeshVertexFormat *vertexFormat, int gy, int gx, float ry, float rx);
bool Mesh_PatchedSphere(Mesh *me, const MeshVertexFormat *vertexFormat, int n);

} // namespace arctic
