#include <math.h>
#include "piMesh_IteSimple.h"
#include "../../libMath/piVecTypes.h"
#include "../../libMath/piRandom.h"

namespace piLibs {


bool piMesh_Vertex( piMesh *me, IQMESH_VERTEX_FUNC callback, void *opaque  )
{
    const int num = me->mVertexData.mVertexArray[STID].mNum;
    for( int i=0; i<num; i++ )
    {
        float *v = (float*)me->GetVertexData( STID, i, POSID );
        float *n = (float*)me->GetVertexData( STID, i, NORID );

        if( !callback( v, n, i, opaque ) )
            return false;
    }
    return true;
}

/*
float area1( const float *pa, const float *pb, const float *pc )
{
    const float e1[3] = { pb[0]-pa[0], pb[1]-pa[1], pb[2]-pa[2] };
    const float e2[3] = { pc[0]-pa[0], pc[1]-pa[1], pc[2]-pa[2] };

    const float no[3] = { e1[1]*e2[2] - e1[2]*e2[1],
                          e1[2]*e2[0] - e1[0]*e2[2],
                          e1[0]*e2[1] - e1[1]*e2[0] };

    return 0.5f * sqrtf( no[0]*no[0] + no[1]*no[1] + no[2]*no[2] );
}
*/
static float calcArea( const vec3 **v, int num )
{
    float f = 0.0f;

    for( int j=0; j<(num-2); j++ )
    {
        const vec3 e1 = *(v[j+1]) - *(v[0]);
        const vec3 e2 = *(v[j+2]) - *(v[0]);
        f += length( cross( e1, e2 ) );
    }

    return f * 0.5f;
}

static void interpolateBarycentric3( vec3 *pos, vec3 *nor, float u, float v, const vec3 **po, const vec3 **no )
{
    pos->x = po[0]->x + u*(po[1]->x-po[0]->x) + v*(po[3]->x-po[0]->x) + u*v*(po[0]->x-po[1]->x+po[2]->x-po[3]->x);
    pos->y = po[0]->y + u*(po[1]->y-po[0]->y) + v*(po[3]->y-po[0]->y) + u*v*(po[0]->y-po[1]->y+po[2]->y-po[3]->y);
    pos->z = po[0]->z + u*(po[1]->z-po[0]->z) + v*(po[3]->z-po[0]->z) + u*v*(po[0]->z-po[1]->z+po[2]->z-po[3]->z);

    nor->x = no[0]->x + u*(no[1]->x-no[0]->x) + v*(no[3]->x-no[0]->x) + u*v*(no[0]->x-no[1]->x+no[2]->x-no[3]->x);
    nor->y = no[0]->y + u*(no[1]->y-no[0]->y) + v*(no[3]->y-no[0]->y) + u*v*(no[0]->y-no[1]->y+no[2]->y-no[3]->y);
    nor->z = no[0]->z + u*(no[1]->z-no[0]->z) + v*(no[3]->z-no[0]->z) + u*v*(no[0]->z-no[1]->z+no[2]->z-no[3]->z);

    *nor = normalize( *nor );
}


static void interpolateBarycentricA( float *res, float u, float v, const float **da, int numchannels )
{
    for( int i=0; i<numchannels; i++ )
    {
        res[i] = da[0][i] + u*(da[1][i]-da[0][i]) + v*(da[3][i]-da[0][i]) + u*v*(da[0][i]-da[1][i]+da[2][i]-da[3][i]);
    }
}


static bool isOutside( const vec3 **p, int num, const float *bbox )
{
    int n;
 
    n=1; for( int i=0; i<num; i++ ) if( p[i]->x > bbox[0] ) { n=0; break; } if( n ) return true;
    n=1; for( int i=0; i<num; i++ ) if( p[i]->x < bbox[3] ) { n=0; break; } if( n ) return true;
    n=1; for( int i=0; i<num; i++ ) if( p[i]->y > bbox[1] ) { n=0; break; } if( n ) return true;
    n=1; for( int i=0; i<num; i++ ) if( p[i]->y < bbox[4] ) { n=0; break; } if( n ) return true;
    n=1; for( int i=0; i<num; i++ ) if( p[i]->z > bbox[2] ) { n=0; break; } if( n ) return true;
    n=1; for( int i=0; i<num; i++ ) if( p[i]->z < bbox[5] ) { n=0; break; } if( n ) return true;

    return false;
} 


bool piMesh_Surface( piMesh *me, const float density, int supersample, int mseed, IQMESH_SURFACE_FUNC callback, void *opaque, const float *bbox )
{
    const int num = me->mFaceData.mIndexArray[STID].mNum;
    int id = 0;
    int seed = 123456789 + mseed;
    for( int i=0; i<num; i++ )
    {
        const int ni = me->mFaceData.mIndexArray[0].mBuffer[i].mNum;
        const int *ind = me->mFaceData.mIndexArray[0].mBuffer[i].mIndex;

        const vec3 *po[4] = { (vec3 *)me->GetVertexData( STID, ind[0], POSID ),
                              (vec3 *)me->GetVertexData( STID, ind[1], POSID ),
                              (vec3 *)me->GetVertexData( STID, ind[2], POSID ),
                    (ni==3)?0:(vec3 *)me->GetVertexData( STID, ind[3], POSID ) };

       if( bbox )
       if( isOutside( po, ni, bbox ) )
           continue;


        const float area = calcArea( po, ni );
/*
        const piVec3F *no[4] = { (piVec3F *)piMesh_GetVertexData( me, ind[0], nPos ),
                                 (piVec3F *)piMesh_GetVertexData( me, ind[1], nPos ),
                                 (piVec3F *)piMesh_GetVertexData( me, ind[2], nPos ),
                       (ni==3)?0:(piVec3F *)piMesh_GetVertexData( me, ind[3], nPos ) };

*/


        float isupersample = 1.0f/(float)supersample;

        const int numpoints = (int)((float)supersample*area*density);

        for( int j=0; j<numpoints; j++ )
        {
            if( supersample>1 )
            {
			    if( pirand_flo23b(&seed) > isupersample ) continue;
            }

            const float u = pirand_flo23b( &seed );
            const float v = pirand_flo23b( &seed );

            //piVec3F rpos, rnor;
            //interpolateBarycentric3( &rpos, &rnor, u, v, po, no );
            //if( !callback( (float*)&rpos, (float*)&rnor, id++, opaque ) )
            //    return false;

            float data[16];
            int off = 0;
            for( int k=0; k<me->mVertexData.mVertexArray[STID].mFormat.mNumElems; k++ )
            {
                piMeshVertexElemInfo *vei = me->mVertexData.mVertexArray[STID].mFormat.mElems + k; 
                if( vei->mType !=piRMVEDT_Float ) continue;

                const float *da[4] = { (float *)me->GetVertexData( STID, ind[0], k ),
                                       (float *)me->GetVertexData( STID, ind[1], k ),
                                       (float *)me->GetVertexData( STID, ind[2], k ),
                             (ni==3)?0:(float *)me->GetVertexData( STID, ind[3], k ) };
                
                interpolateBarycentricA( data+off, u, v, da, vei->mNumComponents );
                off += vei->mNumComponents;
            }

            if( !callback( data, data+3, id++, opaque ) )
                return false;
        }
    }

    return true;
}


}