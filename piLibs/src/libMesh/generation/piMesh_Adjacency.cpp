#include <malloc.h>
#include <string.h>

#include "piMesh_Adjacency.h"

namespace piLibs {

// devuelve el index de la arista que contiene a "a" y "b"
int piMeshAdjacency_GetEdgeIDContainingVerts( piMeshAdjacency *am, int a, int b )
{
    const int ma = (a>b)?a:b;
    const int mi = (a<b)?a:b;

    for( int i=0; i<am->numaristas; i++ )
		if( am->arisaux[i].v0 == ma && am->arisaux[i].v1 == mi )
            return i;
    return -1;
}

bool piMeshAdjacency_Init( piMeshAdjacency *me, const piMesh *mesh )
{
    const int numf = mesh->mFaceData.mIndexArray[0].mNum;
    me->nmax = numf*4;
    me->numaristas = 0;
    me->arisaux = (piMeshEdge*)malloc( me->nmax*sizeof(piMeshEdge) );
    if( !me->arisaux )
        return( 0 );
    memset( me->arisaux, 0, me->nmax*sizeof(piMeshEdge) );

    for( int i=0; i<numf; i++ )
    {
        piMeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + i;
        const int fv = face->mNum;
        for( int j=0; j<fv; j++ )
        {
            const int v0 = face->mIndex[j];
            const int v1 = face->mIndex[(j+1)%fv];
            const int ed = piMeshAdjacency_GetEdgeIDContainingVerts( me, v0, v1 );

            if( ed==-1 )
            {
                const int id = me->numaristas++;
                me->arisaux[id].v0 = (v0>v1)?v0:v1;
                me->arisaux[id].v1 = (v0<v1)?v0:v1;
                me->arisaux[id].va = 1;
            }
            else
            {
                me->arisaux[ed].va = 2;
            }
        }
    }

    //----------------
    const int num = mesh->mVertexData.mVertexArray[0].mNum;
    me->mVertexValence = (char*)malloc( num );
    if( !me->mVertexValence )
        return false;

    memset( me->mVertexValence, 0, num );

    return true;
}

void piMeshAdjacency_DeInit( piMeshAdjacency *me )
{
    free( me->mVertexValence );
    free( me->arisaux );
}

int piMeshAdjacency_GetNumEdges( piMeshAdjacency *me )
{
    return me->numaristas;
}


}
