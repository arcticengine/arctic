#include <math.h>
#include "../../libMath/piVecTypes.h"
#include "piCameraF.h"

namespace piLibs {

piCameraF::piCameraF()
{
    mMatrix    = setIdentity4();
    mMatrixInv = setIdentity4();
}

piCameraF::~piCameraF()
{
}

void piCameraF::Set(const vec3 & pos, const vec3 & dir, const float roll)
{
    mMatrix    = setLookat( pos, pos + dir, vec3(sinf(roll),cosf(roll),sinf(roll) ));
    mMatrixInv = invert(mMatrix);
}

void piCameraF::SetPos(const vec3 & pos)
{
    mMatrix[ 3] = -(mMatrix[0] * pos[0] + mMatrix[1] * pos[1] + mMatrix[ 2] * pos[2]);
    mMatrix[ 7] = -(mMatrix[4] * pos[0] + mMatrix[5] * pos[1] + mMatrix[ 6] * pos[2]);
    mMatrix[11] = -(mMatrix[8] * pos[0] + mMatrix[9] * pos[1] + mMatrix[10] * pos[2]);
}

vec3 piCameraF::GetPos( void ) const
{
    return (mMatrixInv * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
}

vec3 piCameraF::GetDir(void) const
{
    return normalize((mMatrixInv * vec4(0.0f, 0.0f, -1.0f, 0.0f)).xyz());
}

const mat4x4 & piCameraF::GetMatrix(void) const
{
    return mMatrix;
}

const mat4x4 & piCameraF::GetMatrixInverse(void) const
{
    return mMatrixInv;
}

void piCameraF::SetMatrix(const mat4x4 & mat)
{   
    mMatrix = mat;
    mMatrixInv = invert(mMatrix);
}

void piCameraF::LocalMove( const vec3 & pos )
{
    mMatrix[ 3] -= pos.x;
    mMatrix[ 7] -= pos.y;
    mMatrix[11] -= pos.z;
    mMatrixInv = invert(mMatrix);
}

void piCameraF::GlobalMove(const vec3 & pos)
{
    mMatrix[ 3] -= (mMatrix[0] * pos[0] + mMatrix[1] * pos[1] + mMatrix[ 2] * pos[2]);
    mMatrix[ 7] -= (mMatrix[4] * pos[0] + mMatrix[5] * pos[1] + mMatrix[ 6] * pos[2]);
    mMatrix[11] -= (mMatrix[8] * pos[0] + mMatrix[9] * pos[1] + mMatrix[10] * pos[2]);
    mMatrixInv = invert(mMatrix);
}

void piCameraF::RotateXY(const float x, const float y )
{
    mMatrix = setRotationY(x) * mMatrix;
    mMatrixInv = invert(mMatrix);
}

}