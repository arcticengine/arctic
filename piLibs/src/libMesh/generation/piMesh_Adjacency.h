#pragma once

#include "../piMesh.h"

namespace piLibs {


typedef struct
{
    int    v0;
    int    v1;
    int    va;  // valence: 1 or 2
}piMeshEdge;

typedef struct
{
    int         nmax;
    int         numaristas;
    piMeshEdge  *arisaux;
    //-----------------
    char        *mVertexValence;
    
}piMeshAdjacency;

bool piMeshAdjacency_Init( piMeshAdjacency *me, const piMesh *mesh );
void piMeshAdjacency_DeInit( piMeshAdjacency *me );

int piMeshAdjacency_GetNumEdges( piMeshAdjacency *am );
// devuelve el index de la arista que contiene a "a" y "b"
int piMeshAdjacency_GetEdgeIDContainingVerts( piMeshAdjacency *am, int a, int b );



} // namespace piLibs