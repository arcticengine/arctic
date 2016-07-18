#pragma once

#include "../piMesh.h"

namespace piLibs {

#define SUBD_CC 0
#define SUBD_TE 1


bool piMesh_Subdivide( piMesh *me, int times, int mode ); // 0=cc   1=tesselate
bool piMesh_ExtrudeFace( piMesh *me, int faceID );
bool piMesh_Relax( piMesh *mesh, int times, float r );

bool piMesh_Clip( piMesh *mesh, float y );

bool piMesh_Split( piMesh *outMesh, piMesh *mesh, int nx, int ny, int nz, int *resNum );

bool piMesh_Separate(piMesh *mesh);


/*
// (base,top) = (fp[0], fp[1])
int  mod_extrude(   QMESH *mesh, float *fp, unsigned char *bp, intptr *vp );

// (times), (mode) = (bp[0]), (bp[1])
int  mod_subdivide( QMESH *mesh, float *fp, unsigned char *bp, intptr *vp );

// (amount) = (fp[0])
int  mod_relax(     QMESH *mesh, float *fp, unsigned char *bp, intptr *vp );

// (dir,plane,tolerance) = (bp[0], fp[0], fp[1])
int  mod_symmetrice(   QMESH *mesh, float *fp, unsigned char *bp, intptr *vp );
*/
} // namespace piLibs
