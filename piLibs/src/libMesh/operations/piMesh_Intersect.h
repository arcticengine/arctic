#pragma once

#include "../../libMath/piVecTypes.h"

namespace piLibs {

bool piMesh_Intersect( const piMesh *me, const vec3 & ro, vec3 &rd, float tmin, float tmax, float *res );

} // namespace piLibs