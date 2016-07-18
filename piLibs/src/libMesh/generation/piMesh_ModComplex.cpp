///--------------------------------------------------------------------------//
// pi . 2005 . code for the TheIntro 64 kb intro by RGBA                    //
//--------------------------------------------------------------------------//

#include <math.h>
#include <string.h>
#include <malloc.h>
#include "../../libDataUtils/piArray.h"
#include "../../libDataUtils/piTArray.h"
#include "../../libMath/piVecTypes.h"

#include "../piMesh.h"
#include "piMesh_Generate.h"
#include "piMesh_ModComplex.h"
#include "piMesh_Adjacency.h"


namespace piLibs {


static void piMeshVert_Scale( piMesh *dst, int dstID, float scale )
{
    float *dstdata = (float*)dst->GetVertexData( STID, dstID, POSID );
    for( int j=0; j<3; j++ ) dstdata[j] *= scale;
}

static void piMeshVert_SInc( piMesh *dst, int dstID, float s, piMesh *src, int srcID )
{
    float *srcdata = (float*)src->GetVertexData( STID, srcID, POSID );
    float *dstdata = (float*)dst->GetVertexData( STID, dstID, POSID );
    for( int j=0; j<3; j++ ) dstdata[j] += s*srcdata[j];
}

const static float vpw_cc[4] = { 9.0f, 3.0f, 1.0f, 3.0f };
const static float epw_cc[4] = { 3.0f, 3.0f, 1.0f, 1.0f };
const static float vpw_te[4] = { 16.0f, 0.0f, 0.0f, 0.0f };
const static float epw_te[4] = { 4.0f, 4.0f, 0.0f, 0.0f };

bool piMesh_Subdivide( piMesh *mesh, int times, int mode )
{
    const float *vpw = (mode==1) ? vpw_te : vpw_cc;
    const float *epw = (mode==1) ? epw_te : epw_cc;

    while( times-- )
    {
        piMesh   aux;
        piMeshAdjacency adj;

        if( !piMeshAdjacency_Init( &adj, mesh ) )
            return false;

        const int numf = mesh->mFaceData.mIndexArray[0].mNum;
        const int numv = mesh->mVertexData.mVertexArray[0].mNum;
        const int nume = piMeshAdjacency_GetNumEdges( &adj );

        if( !aux.Init(  1, numv+numf+nume, &mesh->mVertexData.mVertexArray[0].mFormat, piRMVEDT_Polys, 1, numf*4 ) )
            return false;

        //////////////////////////////////////////////////////////////////////
        const int foff = 0;
        const int voff = numf;
        const int eoff = numf + numv;

        int numOutFaces = 0;

        for( int i=0; i<numf; i++ )
        {
            piMeshFace *fo = mesh->mFaceData.mIndexArray[0].mBuffer + i;
            const int nv = fo->mNum;

            int eid[4]; 
            for( int j=0; j<nv; j++ )
            {
                int abcd[4]; for( int k=0; k<nv; k++ ) abcd[k] = fo->mIndex[ (j+k)%nv ];

                adj.mVertexValence[ abcd[0] ] ++;

                eid[j] = piMeshAdjacency_GetEdgeIDContainingVerts( &adj, abcd[0], abcd[1] );

                piMeshVert_SInc( &aux, foff + i, 0.25f, mesh, abcd[0] );   // face vertex
    
                for( int k=0; k<nv; k++ )
                {
                    piMeshVert_SInc( &aux, voff + abcd[0], vpw[k], mesh, abcd[k] );   // vertex vertex
                    piMeshVert_SInc( &aux, eoff + eid[j],  epw[k], mesh, abcd[k] );   // edge vertex
                }
            }

            for( int j=0; j<nv; j++ )                                            // make child faces
            {
                piMeshFace *df = aux.mFaceData.mIndexArray[0].mBuffer + numOutFaces++;
                df->mNum = 4;
                df->mIndex[0] = foff + i;
                df->mIndex[1] = eoff + eid[(j+nv-1)%nv]; 
                df->mIndex[2] = voff + fo->mIndex[j];
                df->mIndex[3] = eoff + eid[ j       ]; 
            }
        }

        for( int i=0; i<numv; i++ ) piMeshVert_Scale( &aux, voff+i, 0.0625f/(float)adj.mVertexValence[i] );
        for( int i=0; i<nume; i++ ) piMeshVert_Scale( &aux, eoff+i, 0.1250f/(float)adj.arisaux[i].va );


        //////////////////////////////////////////////////////////////////////

        mesh->DeInit();
        piMeshAdjacency_DeInit( &adj );
        
        aux.mFaceData.mIndexArray[0].mNum = numOutFaces;
        *mesh = aux;
         
        }

    return true;
}

bool piMesh_Relax( piMesh *mesh, int times, float r )
{
    const int numf = mesh->mFaceData.mIndexArray[0].mNum;

    for( int j=0; j<times; j++ )
    {
        for( int i=0; i<numf; i++ )
        {
            piMeshFace *q = mesh->mFaceData.mIndexArray[0].mBuffer + i;

            const int num = q->mNum;
            for( int k=0; k<num; k++ )
            {
                const int ia = q->mIndex[k];
                const int ib = q->mIndex[(k+1)%num];

                float *va = (float*)mesh->GetVertexData( STID, ia, POSID );
                float *vb = (float*)mesh->GetVertexData( STID, ib, POSID );

                float dif[3] = { vb[0]-va[0], vb[1]-va[1], vb[2]-va[2] };

                va[0] = va[0] + dif[0]*r;
                va[1] = va[1] + dif[1]*r;
                va[2] = va[2] + dif[2]*r;
    
                vb[0] = vb[0] - dif[0]*r;
                vb[1] = vb[1] - dif[1]*r;
                vb[2] = vb[2] - dif[2]*r;

            }
        }
    }



    return true;
}



#if 0
// (base,top) = (fp[0], fp[1])
int mod_extrude( QMESH *mesh, float *fp, unsigned char *bp, intptr *vp )
{
    int           i, j, n, nv, nq;
    int           va, vb;
    int           newnumquads;
    QUAD          *qori;
    QUAD          *qdst;
    QMESH         aux;

    if( !qmesh_alloc( &aux, mesh->nv*2, mesh->nq*6 ) )
        return( 0 );

    // duplicate verts
    n = mesh->nv;
    for( i=0; i<n; i++ )
        {
        aux.verts[  i] = mesh->verts[i];
        aux.verts[  i].vert.y = fp[0];
        aux.verts[n+i] = mesh->verts[i];
        aux.verts[n+i].vert.y = fp[1];
        }


    // duplicate faces
    nq   = mesh->nq;
    qori = mesh->quads;
    qdst = aux.quads;
    newnumquads = 0;
    for( i=0; i<nq; i++, qori++ )
        {
        nv = qori->n;

        // top cap
        *qdst++ = *qori;

        // botton cap
        for( j=0; j<nv; j++ )
            qdst->v[j] = qori->v[nv-1-j] + n;
        qdst->n = nv;
        qdst++;

        // edge quad
        for( j=0; j<nv; j++ )
            {
            va = qori->v[j];
            vb = qori->v[(j+1)%nv];
            //get_ab( qori, j, &va, &vb );
            if( isedge( mesh, va, vb ) )
                {
                qdst->v[0] = vb;
                qdst->v[1] = va;
                qdst->v[2] = va + n;
                qdst->v[3] = vb + n;
                qdst->n = nv;
                qdst++;
                newnumquads++;
                }
            }
        }

    qmesh_free( mesh );
    mesh->quads = aux.quads;
    mesh->verts = aux.verts;
    mesh->nv    = mesh->nv*2;
    mesh->nq    = mesh->nq*2 + newnumquads;

    return( 1 );
}



//----------------------------------------------------------------------------------------


static int init_aristas_qmesh( AMANAG *am, QMESH *m )
{
    int     i, a;
    QUAD    *q;

    if( !init_aristas( am, m->nq<<2 ) )
        return( 0 );

    for( i=0; i<m->nq; i++ )
        {
        q = m->quads + i;
        for( a=0; a<q->n; a++ )
            mete( am, q->v[a], q->v[(a+1)%q->n], i );
        }

    return( 1 );
}





static void VERT_Add( QVERT *v1, QVERT *dst )
{
    int i;
    float *sf = (float*)v1;
    float *df = (float*)dst;

    for( i=0; i<5; i++ )
        *df++ += *sf++;
}

static void VERT_Scale( QVERT *v, float s )
{
    int i;
    float *df = (float*)v;

    for( i=0; i<5; i++ )
        *df++ *= s;
}


static void crea_vertices_catmulclark( QMESH *aux, QMESH *mesh, AMANAG *am )
{
    int     j, k, n, l;
    QVERT    v1, v2;
    QVERT    *dst;
    int     a, b;
    QUAD    *fo;

    dst = aux->verts;

    // puntos de cara [mesh->nfaces]
    fo = mesh->quads;
    for( j=0; j<mesh->nq; j++ )
        {
        for( k=0; k<fo->n; k++ )
            VERT_Add( mesh->verts + fo->v[k], dst );
        VERT_Scale( dst++, 1.0f/(float)fo->n );
        fo++;
        }

    // puntos de arista [naristas]
    for( j=0; j<am->numaristas; j++ )
        {
        a = am->arisaux[j].v0;
        b = am->arisaux[j].v1;

#ifdef DEBUG
/*
if( a==-1 || b==-1 )
{
mdebug_printstr( "qmesh_subdivide() :: error 0!!!!!\r\n" );
halt_program();
}
*/
#endif

        VERT_Add( mesh->verts+a, dst );
        VERT_Add( mesh->verts+b, dst );

        a = am->arisaux[j].c0;
        b = am->arisaux[j].c1;

//pi!!!!!!!!!!!! bordes!
if( b==-1 ) b=a;

        VERT_Add( aux->verts+a, dst );
        VERT_Add( aux->verts+b, dst );
        VERT_Scale( dst++, 0.25f );
        }


    // los vertices viejos [mesh->nverts]
    for( j=0; j<mesh->nv; j++ )
        {
        n = aristas_num4v( am, j );

        if( n<3 ) n=3;

  

//        if( n<3 ) n=3;
        
        //--- Q ---
        memset( &v1, 0, sizeof(QVERT) );
        l = 0;
        fo = mesh->quads;
        for( k=0; k<mesh->nq; k++ )
            {
            
            if( fo->v[0]==j || fo->v[1]==j || fo->v[2]==j || ((fo->v[3]==j)&&(fo->n==4)) )
                {
                VERT_Add( aux->verts+k, &v1 );
                l++;
                }
            fo++;
            }
        VERT_Scale( &v1, 1.0f/(float)l );
        //--- R ---
        memset( &v2, 0, sizeof(QVERT) );
        l = 0;
        for( k=0; k<am->numaristas; k++ )
            {
            a = am->arisaux[k].v0;
            b = am->arisaux[k].v1;
#ifdef DEBUG
//if( a==-1 || b==-1 )
//{
//mdebug_printstr( "qmesh_subdivide() :: error 3!!!!!\r\n" );
//return( 0 );
//}
#endif
            if( a==j || b==j )
                {
                VERT_Add( mesh->verts+a, &v2 );
                VERT_Add( mesh->verts+b, &v2 );
                l++;
                }
            }
        VERT_Scale( &v2, 1.0f/(float)l );
        //--- V ---
        *dst = mesh->verts[j];
        VERT_Scale( dst, (float)(n-3) );
        VERT_Add( &v1, dst );
        VERT_Add( &v2, dst );
        VERT_Scale( dst++, 1.0f/(float)n );
        }

}







*/


/*
// (dir, tolerance) = (bp[0], fp[0])
int mod_symmetrice(   QMESH *mesh, float *fp, unsigned char *bp, intptr *vp )
{
    int           i, j, nv, nq;
    QUAD          *qori;
    QUAD          *qdst;
    QMESH         aux;

    nv = mesh->nv;
    nq = mesh->nq;

    if( !qmesh_alloc( &aux, nv*2, nq*2 ) )
        return( 0 );


    for( i=0; i<nv; i++ )
        {
        aux.verts[   i] = mesh->verts[i];
        aux.verts[nv+i] = mesh->verts[i];

        ((float*)&aux.verts[nv+i].vert.x)[bp[0]] *= -1.0f;
        }

    for( i=0; i<nq; i++ )
        {
        qori = mesh->quads + i;
        qdst = aux.quads + i;

        qdst[0]  = qori[0];
        qdst[nq] = qori[0];

        for( j=0; j<qori->n; j++ )
            {
            int k = qori->v[j];

            float x = ((float*)&aux.verts[k].vert.x)[bp[0]];
            if( x>fp[0] )
                k += nv;
            qdst[nq].v[qori->n-1-j] = k;
            }   
        
        }

      
    qmesh_free( mesh );
/*
    mesh->quads = aux.quads;
    mesh->verts = aux.verts;
    mesh->nv    = aux.nv;
    mesh->nq    = aux.nq;
*/
    *mesh = aux;

    return( 1 );
}

#endif


bool piMesh_ExtrudeFace( piMesh *me, int faceID )
{
    {
    const piMeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;
    if( !me->Expand( num, num ) ) return false;
    }

    piMeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = face->mNum;


    const int newIndices = me->mVertexData.mVertexArray[0].mNum;
    const int newFace = me->mFaceData.mIndexArray[0].mNum;
    for( int i=0; i<num; i++ )
    {
        piMeshFace *nface = me->mFaceData.mIndexArray[0].mBuffer + newFace + i;
        nface->mNum = 4;
        nface->mIndex[0] =  face->mIndex[i];
        nface->mIndex[1] =  face->mIndex[(i+1)%num];
        nface->mIndex[2] =  newIndices + ((i+1)%num);
        nface->mIndex[3] =  newIndices +   i;
    }
    //--------

    for( int i=0; i<num; i++ )
    {
        vec3 *w = (vec3 *)me->GetVertexData( STID, face->mIndex[i], POSID );
        vec3 *v = (vec3 *)me->GetVertexData( STID, newIndices+i, POSID );

        *v = *w;
    }
    //--------
    for( int i=0; i<num; i++ )
        face->mIndex[i] = newIndices + i;

    me->mVertexData.mVertexArray[0].mNum += num;
    me->mFaceData.mIndexArray[0].mNum += num;

    return true;
}



bool piMesh_Clip( piMesh *mesh, float y )
{
    const int numf = mesh->mFaceData.mIndexArray[0].mNum;

    piMeshFace *faces = (piMeshFace*)malloc( numf*sizeof(piMeshFace) );
    if( !faces )
        return false;
    int nnumf = 0;
    for( int i=0; i<numf; i++ )
    {
        piMeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;
        //bool canRemove = true;
        int above = 0;
        for( int j=0; j<mf->mNum; j++ )
        {
            const int vid = mf->mIndex[j];
            vec3 *vert = (vec3 *)mesh->GetVertexData( STID, vid, POSID );
            //if( vert->y>y ) { canRemove=false; break; }
            if( vert->y>y ) above++;
        }

        //if( above==mf->mNum ) faces[nnumf++] = *mf;
        if( above>0 ) faces[nnumf++] = *mf;

        //if( !canRemove ) faces[nnumf++] = *mf;
    }

    free( mesh->mFaceData.mIndexArray[0].mBuffer );
    mesh->mFaceData.mIndexArray[0].mBuffer  = faces;
    mesh->mFaceData.mIndexArray[0].mNum = nnumf;
    return true;
}


bool piMesh_Split( piMesh *outMesh, piMesh *mesh, int nx, int ny, int nz, int *resNum )
{
	const int numf = mesh->mFaceData.mIndexArray[0].mNum;

	mesh->CalcBBox( STID, POSID );

	const bound3 ob = mesh->mBBox;

    const int numOutMeshs = nx*ny*nz;
	if( numOutMeshs>256 )
		return false;
	piTArray<piMeshFace> faces[256];
	for( int i=0; i<numOutMeshs; i++ )
	{
		if( !faces[i].Init( numf, false ) )
			return false;
	}

    int nnumf = 0;
    for( int i=0; i<numf; i++ )
    {
        const piMeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;
		const float *vert = (float*)mesh->GetVertexData( STID, mf->mIndex[0], POSID );

		const int ix = (int)floorf( (vert[0] - ob.mMinX)/(ob.mMaxX - ob.mMinX) * (float)nx * 0.99999f );
		const int iy = (int)floorf( (vert[1] - ob.mMinY)/(ob.mMaxY - ob.mMinY) * (float)ny * 0.99999f );
		const int iz = (int)floorf( (vert[2] - ob.mMinZ)/(ob.mMaxZ - ob.mMinZ) * (float)nz * 0.99999f );
		const int id = ix + iy*nx + iz*nx*ny;

		if( id<0 || id>=numOutMeshs )
			return false;

		if( !faces[id].Append( mf ) ) 
			return false;
    }

	int remap[256];
	int num = 0;
	for( int i=0; i<numOutMeshs; i++ )
	{
		const int numDstFaces = faces[i].GetLength();
		if( numDstFaces!=0 )
		{
			remap[num++] = i;
		}
	}

	*resNum = num;

	for( int i=0; i<num; i++ )
	{
		piMesh *dst = outMesh + i;
		piTArray<piMeshFace> *src = faces + remap[i];
		const int numDstFaces = src->GetLength();
		const int numDstVerts = mesh->mVertexData.mVertexArray[0].mNum;
		if( !dst->Init( 1, numDstVerts, &mesh->mVertexData.mVertexArray[0].mFormat, mesh->mFaceData.mType, 1, numDstFaces ) )
			return false;
		memcpy( dst->mFaceData.mIndexArray[0].mBuffer, src->GetAddress(0), numDstFaces*sizeof(piMeshFace) );
		memcpy( dst->mVertexData.mVertexArray[0].mBuffer, mesh->mVertexData.mVertexArray[0].mBuffer, numDstVerts*mesh->mVertexData.mVertexArray[0].mFormat.mStride );

		if( !dst->Compact() )
			return false;
	}

	for( int i=0; i<numOutMeshs; i++ )
	{
		faces[i].End();
	}


	return true;
}


bool piMesh_Separate(piMesh *mesh)
{
    const int nums = mesh->mVertexData.mNumVertexArrays;
    const int nume = mesh->mFaceData.mNumIndexArrays;

    if (nums>1 || nume>1 ) return false;

    const int numf = mesh->mFaceData.mIndexArray[0].mNum;

    int numv = 0;
    for (int i = 0; i<numf; i++)
    {
        piMeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;
        numv += mf->mNum;
    }

    piMesh tmp;

    if (!tmp.Init(1, numv, &mesh->mVertexData.mVertexArray[0].mFormat, mesh->mFaceData.mType, 1, numf) )
        return false;
    
    tmp.mFaceData.mIndexArray[0].mMax = numf;
    tmp.mFaceData.mIndexArray[0].mNum = numf;

    numv = 0;
    for (int i = 0; i<numf; i++)
    {
        piMeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;

        tmp.mFaceData.mIndexArray[0].mBuffer[i].mNum = mf->mNum;

        for ( int j=0; j<mf->mNum; j++ )
        {
            const int vid = mf->mIndex[j];
            char data[256];
            mesh->GetVertex( 0, vid, data);
            tmp.SetVertex( 0, numv, data);
            tmp.mFaceData.mIndexArray[0].mBuffer[i].mIndex[j] = numv;
            numv++;
        }
    }

    mesh->DeInit();
    memcpy( mesh, &tmp, sizeof(piMesh) );

    return true;
}



}
