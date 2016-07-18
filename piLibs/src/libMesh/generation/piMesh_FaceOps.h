#pragma once

#include "../../libMath/piVecTypes.h"
#include "../../libMesh/piMesh.h"

namespace piLibs {

vec3   piMeshFace_GetNormal( const piMesh *me, int faceID, float * nlen );
vec3   piMeshFace_GetCenter( const piMesh *me, int faceID );
mat4x4 piMeshFace_GetFrame(  const piMesh *me, int faceID );
void piMeshFace_GetFrame2( const piMesh *me, int faceID, mat4x4 & ltw, mat4x4 & w2l );

void piMeshFace_TransformGlobal( piMesh *mesh, int faceID, const mat4x4 & mat );
void piMeshFace_TransformLocal(  piMesh *mesh, int faceID, const mat4x4 & mat );

// obsolete?
void piMeshFace_Shrink( piMesh *mesh, int faceID, float amount );
void piMeshFace_Extrude( piMesh *mesh, int faceID, float amount );
void piMeshFace_ExtrudeDir( piMesh *mesh, int faceID, const vec3 & dir, float amount );
void piMeshFace_ExtrudeTo( piMesh *mesh, int faceID, const vec3 & xyz );

}