#pragma once

#include "piVecTypes.h"

namespace piLibs {

inline float sdBox(const vec3 & p, const vec3 & b)
{
    const vec3 d = abs(p) - b;
    return fminf(maxcomp(d), 0.0f) + length(vmax(d, 0.0f));
}

}