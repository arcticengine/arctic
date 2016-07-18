#pragma once

#include "../piMesh.h"

namespace piLibs {


int piMeshPly_Read( piMesh *mesh, const char *name, bool calcNormals );

} // namespace piLibs