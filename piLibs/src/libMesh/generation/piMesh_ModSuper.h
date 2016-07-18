#pragma once

#include "qmesh.h"

namespace piLibs {

int  mod_connect( QMESH **mp, float *fp, unsigned char *bp, intptr *vp );
int  mod_cloth( QMESH **mp, float *fp, unsigned char *bp, intptr *vp );

} // namespace piLibs