#pragma once

#include "../piMesh.h"
#include "../../libMath/piVecTypes.h"

namespace piLibs {


void piMesh_Scale( piMesh *me, const vec3 & xyz );
void piMesh_Translate( piMesh *me, const vec3 & xyz );
void piMesh_Orientate( piMesh *me, const float *au, const float *av, const float *aw );
void piMesh_Sit( piMesh *me );
void piMesh_Tap( piMesh *me, float factor );
void piMesh_SwapYZ(piMesh *me);

} // namespace piLibs
/*
typedef void (*D_FUNC)( QVERT *vert, unsigned char param );//, float *params );
//typedef int  (*C_FUNC)( VERT *vert, int num, int clone, int nclones ); 

// (scale) = (fp0, fp1, fp2)
int  mod_scale(            QMESH *m, float *fp, unsigned char *bp, intptr *vp );

// (translate) = (bp0, bp1, bp2, fp0)
int  mod_translate(        QMESH *m, float *fp, unsigned char *bp, intptr *vp );

// (axis),(angle) = (bp0, bp1, bp2), (fp3)
int  mod_rotate(           QMESH *m, float *fp, unsigned char *bp, intptr *vp );

// (params) = (fp0..fp13)
int  mod_texturize(        QMESH *m, float *fp, unsigned char *bp, intptr *vp );

// (func) = (vp[0])
int  mod_vertex_retouch(   QMESH *m, float *fp, unsigned char *bp, intptr *vp );

// (num), (func) = (bp[0]), (vp[0])
int  mod_clone(            QMESH *m, float *fp, unsigned char *bp, intptr *vp );
*/
