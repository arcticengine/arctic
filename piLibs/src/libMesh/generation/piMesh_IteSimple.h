#pragma once

#include "../piMesh.h"
#include "piMesh_Generate.h"

namespace piLibs {


typedef bool (*IQMESH_VERTEX_FUNC)(        float *pos,       float *nor, int callID, void *opaque );
typedef bool (*IQMESH_SURFACE_FUNC)( const float *pos, const float *nor, int callID, void *opaque );

bool piMesh_Vertex( piMesh *me, IQMESH_VERTEX_FUNC callback, void *opaque );

bool piMesh_Surface( piMesh *me,
                     const float density, int supersample,  int mseed, 
                     IQMESH_SURFACE_FUNC callback, void *opaque, const float *bbox );

} // namspace piLibs