///--------------------------------------------------------------------------//
// pi . 2005 . code for the TheIntro 64 kb intro by RGBA                    //
//--------------------------------------------------------------------------//

#include <string.h>
#include "../../libMath/piVecTypes.h"
#include "../piMesh.h"
#include "piMesh_Generate.h"

namespace piLibs {


vec3 piMeshFace_GetNormal( const piMesh *me, int faceID, float  *nlen )
{
    vec3 nor = vec3( 0.0f, 0.0f, 0.0f );

    const piMeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    for( int i=0; i<num; i++ )
    {
        const vec3 a = *((vec3 *)me->GetVertexData( STID, face->mIndex[(i+1)%num], POSID ));
        const vec3 b = *((vec3 *)me->GetVertexData( STID, face->mIndex[ i       ], POSID ));
        nor += cross( b, a );
    }
    float l = length( nor );
    nor /= l;
    if (nlen) nlen[0] = 0.5f*l;
    return nor;
}

vec3 piMeshFace_GetCenter( const piMesh *me, int faceID )
{
    const piMeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    vec3 center = vec3( 0.0f, 0.0f, 0.0f ); 
    for( int i=0; i<num; i++ )
    {
        vec3 *v = (vec3 *)me->GetVertexData( STID, face->mIndex[i], POSID );
        center += *v;
    }
    return center / (float)num;
}


mat4x4 piMeshFace_GetFrame( const piMesh *me, int faceID )
{
    const vec3 cen = piMeshFace_GetCenter( me, faceID );

    const piMeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    const vec3 *v0 = (const vec3 *)me->GetVertexData( STID, face->mIndex[0], POSID );
    const vec3 *v1 = (const vec3 *)me->GetVertexData( STID, face->mIndex[1], POSID );
    const vec3 *v2 = (const vec3 *)me->GetVertexData( STID, face->mIndex[num-1], POSID );

    const vec3 uu = normalize( *v1 - *v0 );
    const vec3 vv = normalize( *v2 - *v0 );
    const vec3 ww = normalize( cross(uu,vv) );

    return mat4x4( uu.x, uu.y, uu.z, -dot(uu,cen),
                   vv.x, vv.y, vv.z, -dot(vv,cen),
                   ww.x, ww.y, ww.z, -dot(ww,cen),
                   0.0f, 0.0f, 0.0f, 1.0f );
}

void piMeshFace_GetFrame2( const piMesh *me, int faceID, mat4x4 & ltw, mat4x4 & w2l )
{
    const vec3 cen = piMeshFace_GetCenter( me, faceID );

    const piMeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    const vec3 *v0 = (const vec3 *)me->GetVertexData( STID, face->mIndex[0], POSID );
    const vec3 *v1 = (const vec3 *)me->GetVertexData( STID, face->mIndex[1], POSID );
    const vec3 *v2 = (const vec3 *)me->GetVertexData( STID, face->mIndex[(num-1)], POSID );

    const vec3 uu = normalize( *v1 - *v0 );
    const vec3 vv = normalize( *v2 - *v0 );
    const vec3 ww = normalize( cross(uu,vv) );

    w2l = mat4x4( uu.x, uu.y, uu.z, -dot(uu,cen),
                  vv.x, vv.y, vv.z, -dot(vv,cen),
                  ww.x, ww.y, ww.z, -dot(ww,cen),
                  0.0f, 0.0f, 0.0f, 1.0f );

    ltw = mat4x4( uu.x, vv.x, ww.x, cen.x,
                  uu.y, vv.y, ww.y, cen.y,
                  uu.z, vv.z, ww.z, cen.z,
                  0.0f, 0.0f, 0.0f, 1.0f );
}

void piMeshFace_Shrink( piMesh *me, int faceID, float amount )
{
    const vec3 cen = piMeshFace_GetCenter( me, faceID );

    const piMeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    for( int i=0; i<num; i++ )
    {
        vec3 *v = (vec3 *)me->GetVertexData( STID, face->mIndex[i], POSID );
        *v = mix( *v, cen, amount );
    }
}

void piMeshFace_ExtrudeDir( piMesh *mesh, int faceID, const vec3 & dir, float amount )
{
    const piMeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    for( int i=0; i<num; i++ )
    {
        vec3 *v = (vec3 *)mesh->GetVertexData( STID, face->mIndex[i], POSID );
        *v += dir*amount;
    }
}

void piMeshFace_Extrude( piMesh *mesh, int faceID, float amount )
{
    const vec3 nor = piMeshFace_GetNormal( mesh, faceID, nullptr );
    piMeshFace_ExtrudeDir( mesh, faceID, nor, amount );
}


void piMeshFace_TransformGlobal( piMesh *mesh, int faceID, const mat4x4 & mat )
{
    const piMeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    for( int i=0; i<num; i++ )
    {
        vec3 *v = (vec3 *)mesh->GetVertexData( STID, face->mIndex[i], 0 );

        vec3 p = vtransform( mat, vec3(v->x,v->y,v->z) );
        v->x = p.x;
        v->y = p.y;
        v->z = p.z;
    }
}

void piMeshFace_TransformLocal(  piMesh *mesh, int faceID, const mat4x4 & mat )
{
    const mat4x4 wtl = piMeshFace_GetFrame( mesh, faceID );
    const mat4x4 ltw = invert( wtl );
    const mat4x4 tot = ltw * mat * wtl;

    const piMeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    for( int i=0; i<num; i++ )
    {
        vec3 *v = (vec3 *)mesh->GetVertexData( STID, face->mIndex[i], 0 );

        const vec3 p = vtransform( tot, vec3((const float*)v) );
        v->x = p.x;
        v->y = p.y;
        v->z = p.z;
    }
}

void piMeshFace_ExtrudeTo( piMesh *mesh, int faceID, const vec3 & xyz )
{
    const piMeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;

    const vec3 cen = piMeshFace_GetCenter( (const piMesh*)mesh, faceID );
    const vec3 nor = piMeshFace_GetNormal( (const piMesh*)mesh, faceID, NULL );

    vec3 pn = xyz - cen;

    //piVec3F_Norm( &pn );
    const float pk = -dot( pn, xyz );
    const float ins = 1.0f / dot( pn, pn );
    for( int i=0; i<num; i++ )
    {
        vec3 * v = (vec3 *)mesh->GetVertexData( STID, face->mIndex[i], 0 );

        float dist = distance( *v, cen );

        // project into the plane
        const float t = - (pk + dot(pn,*v)) * ins;
        *v = *v + t*pn;

        vec3 di = normalize( *v - xyz );
        v->x = xyz[0] + di.x*dist; 
        v->y = xyz[1] + di.y*dist; 
        v->z = xyz[2] + di.z*dist; 
    }
}

}