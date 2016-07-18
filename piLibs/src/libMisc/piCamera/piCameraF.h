#pragma once

#include "../../libMath/piVecTypes.h"

namespace piLibs {

class piCameraF
{
public:
    piCameraF();
    ~piCameraF();

    void Set(const vec3 & pos, const vec3 & dir, const float roll);
    void SetPos(const vec3 & pos);

    void   GlobalMove(const vec3 &);
    void   LocalMove(const vec3 &);
    void   RotateXY(const float x, const float y);

    const mat4x4 & GetMatrix( void ) const;
    const mat4x4 & GetMatrixInverse( void ) const;
    void SetMatrix(const mat4x4 & mat );

    vec3 GetPos(void) const;
    vec3 GetDir(void) const;


public:
    mat4x4 mMatrix;
    mat4x4 mMatrixInv;
};



} // namespace piLibs