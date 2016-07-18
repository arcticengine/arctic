///--------------------------------------------------------------------------//
// iq . 2005 . code for the TheIntro 64 kb intro by RGBA                    //
//--------------------------------------------------------------------------//

#include <string.h>
#include "qmesh.h"
#include "genutils.h"


//----------------------------------------------------------------------------------------
#include "e:/2_doing/cs/cg/l_shared/src/kernel/log/log.h"

static int findClosest( VEC3D *v, QMESH *m, float md )
{
    int     i, c;
    VEC3D   del;
    float   dis;

    c = -1;
    for( i=0; i<m->nv; i++ )
        {
        VecSub( &m->verts[i].vert, v, &del );
        dis = VecLength( &del );
        if( dis<md )
            {
            md = dis;
            c = i;
            }
        }

    return c;
}


static int getID( int vid, int *mappings, int nummappings, int offset )
{
    int i;

    for( i=0; i<nummappings; i++ )
        if( mappings[2*i+0]==vid )
            return mappings[2*i+1];

    return 0;
}


// m[0] += m[1]
int mod_connect( QMESH **mp, float *fp, unsigned char *bp, intptr *vp )
{
    int i, j;
    int mappings[2048];
    int nummappings = 0;

    int nv = 0;

    QMESH aux;

    if( !qmesh_alloc( &aux, mp[0]->nv+mp[1]->nv, mp[0]->nq+mp[1]->nq ) )
        return( 0 );
    
    memcpy( aux.verts, mp[0]->verts, mp[0]->nv*sizeof(QVERT) );
    memcpy( aux.quads, mp[0]->quads, mp[0]->nq*sizeof(QUAD) );

    QVERT *dv = aux.verts + mp[0]->nv;
    QUAD  *df = aux.quads + mp[0]->nq;
    QVERT *sv = mp[1]->verts;
    QUAD  *sf = mp[1]->quads;

    // verts
    int nr = 0;
    for( j=0; j<mp[1]->nv; j++ )
        {
        int vid = findClosest( &sv[j].vert, mp[0], fp[0] );
        if( vid==-1 )
            {
            // normal append
            dv[nv++] = sv[j];
            vid = mp[0]->nv + nr++;
            }

        mappings[2*nummappings+0] = j;
        mappings[2*nummappings+1] = vid;
        nummappings++;
        }

    aux.nv = mp[0]->nv + nv;

    // faces
    for( j=0; j<mp[1]->nq; j++ )
        {
        df->n = sf->n;
        for( i=0; i<sf->n; i++ )
            df->v[i] = getID( sf->v[i], mappings, nummappings, mp[0]->nv );
        sf++;
        df++;
        }

    qmesh_free( mp[0] );
    *mp[0] = aux;
    
    
    return( 1 );
}


//=====================================
// fp: dt, friction, gravity, envelope
int mod_cloth( QMESH **mp, float *fp, unsigned char *bp, intptr *vp )
{

    PHYSIC physics;

    if( !PHYSICS_SetUp( &physics, mp[0], mp[1], 1, fp+1, bp[2] ) )
        return( 0 );

    int num = 10*bp[0];
    int numwog = 5*bp[0];
    for( int i=0; i<num; i++ )
        {
        PHYSICS_Do( &physics, fp[0], i>=numwog );
        }

 //   PHYSICS_ToMesh( &physics );

    PHYSICS_End( &physics );
    
    return( 1 );
}