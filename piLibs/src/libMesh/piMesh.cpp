#include <string.h>
#include <malloc.h>

#include "../libSystem/piFile.h"
#include "../libDataUtils/piArray.h"
#include "../libMath/piVecTypes.h"
#include "../libMath/piRandom.h"
#include "piMesh.h"

namespace piLibs {

static const unsigned int typeSizeof[] = {
    1, //   piRMVEDT_UByte    = 0,
    4, //   piRMVEDT_Float    = 1,
    4, //   piRMVEDT_Int      = 2,
    8, //   piRMVEDT_Double   = 3,
};

piMesh::piMesh()
{
}

piMesh::~piMesh()
{
}

bool piMesh::Init( int numVertexStreams, int nv, const piMeshVertexFormat *vfs, 
                  piMeshType type, int numElementsArrays, int numElements )
{
    if( (numElements<0) || (vfs[0].mNumElems>piMesh_MAXELEMS) ) return false;
    if( (type==piRMVEDT_Polys) && (nv<3) ) return false;

    memset( this, 0, sizeof(piMesh) );


    mVertexData.mNumVertexArrays = 0;//numVertexStreams;
    for( int j=0; j<numVertexStreams; j++ )
    {
        if( !this->AddVertexStream( nv, vfs+j ) )
            return false;
    }

    //--------------------
    mFaceData.mNumIndexArrays = numElementsArrays;
    mFaceData.mType = type;
    for( int i=0; i<numElementsArrays; i++ )
    {
		mFaceData.mIndexArray[i].mMax  = numElements;
		mFaceData.mIndexArray[i].mNum  = numElements;
		if( numElements>0 )
		{
            const int bufferSize = numElements * ( (mFaceData.mType==piRMVEDT_Polys) ? sizeof(piMeshFace) : sizeof(unsigned int) );
			mFaceData.mIndexArray[i].mBuffer = (piMeshFace*)malloc( bufferSize );
			if( !mFaceData.mIndexArray[i].mBuffer )
			{
				return false;
			}
			memset( mFaceData.mIndexArray[i].mBuffer, 0, bufferSize );
		}
    }
    return true;
}

bool piMesh::AddVertexStream( const int nv, const piMeshVertexFormat *vf )
{
    const int id = mVertexData.mNumVertexArrays;

    mVertexData.mVertexArray[id].mMax = nv;
    mVertexData.mVertexArray[id].mNum = nv;
    mVertexData.mVertexArray[id].mFormat = *vf;

    mVertexData.mVertexArray[id].mBuffer = malloc( nv*vf->mStride );
    if( !mVertexData.mVertexArray[id].mBuffer )
        return false;
    memset( mVertexData.mVertexArray[id].mBuffer, 0, nv*vf->mStride );

    int off = 0;
    for( int i=0; i<vf->mNumElems; i++ )
    {
        mVertexData.mVertexArray[id].mFormat.mElems[i].mOffset = off;
        off  += vf->mElems[i].mNumComponents*typeSizeof[ vf->mElems[i].mType ];
    }

    mVertexData.mNumVertexArrays = id + 1;

    return true;
}

bool piMesh::Clone(piMesh *dst )
{
    const int nv = mVertexData.mVertexArray[0].mNum;
    const int numElements = mFaceData.mIndexArray[0].mNum;

    const piMeshVertexFormat *vf = &mVertexData.mVertexArray[0].mFormat;

    if( !dst->Init( 1, nv, vf, mFaceData.mType, mFaceData.mNumIndexArrays, numElements ) )
        return false;

    const int bufferSize = numElements * ((mFaceData.mType == piRMVEDT_Polys) ? sizeof(piMeshFace) : sizeof(unsigned int));

    memcpy(dst->mVertexData.mVertexArray[0].mBuffer, mVertexData.mVertexArray[0].mBuffer, nv*vf->mStride);
    memcpy(dst->mFaceData.mIndexArray[0].mBuffer, mFaceData.mIndexArray[0].mBuffer, bufferSize );
    return true;
}

void piMesh::DeInit( void )
{
    for( int i=0; i<mVertexData.mNumVertexArrays; i++ )
    {
        if( mVertexData.mVertexArray[i].mMax ) 
            free( mVertexData.mVertexArray[i].mBuffer );
    }
    for( int i=0; i<mFaceData.mNumIndexArrays; i++ )
    {
        if( mFaceData.mIndexArray[i].mMax )   
           free( mFaceData.mIndexArray[i].mBuffer );
    }
}

int piMeshVertexElemDataType2Num( piMeshVertexElemDataType x )
{
    if( x==piRMVEDT_UByte )  return 0;
    if( x==piRMVEDT_Float )  return 1;
    if( x==piRMVEDT_Int )    return 2;
    if( x==piRMVEDT_Double ) return 3;
    return 0;
}

int piMesh::Save( const wchar_t *name )
{
    piFile fp;
    if( !fp.Open(name, L"wb") )
		return( 0 );

    fp.WriteFloatarray2( (float*)&mBBox, 6 );

    fp.WriteUInt32(  mVertexData.mNumVertexArrays);

    for (int j = 0; j<mVertexData.mNumVertexArrays; j++)
    {
        piMeshVertexArray *va = mVertexData.mVertexArray + j;
        fp.WriteUInt32( va->mNum );
        fp.WriteUInt32( va->mFormat.mStride);
        fp.WriteUInt32( va->mFormat.mDivisor);
        fp.WriteUInt32( va->mFormat.mNumElems);

        for( int i=0; i<va->mFormat.mNumElems; i++ )
	    {
            fp.WriteUInt32( (int)va->mFormat.mElems[i].mType);
            fp.WriteUInt32( va->mFormat.mElems[i].mNumComponents);
            fp.WriteUInt32( va->mFormat.mElems[i].mNormalize ? 1 : 0);
            fp.WriteUInt32( va->mFormat.mElems[i].mOffset);
	    }

        if (va->mNum>0)
	    {
            fp.WriteFloatarray2( (float*)va->mBuffer, va->mNum*va->mFormat.mStride / 4);
	    }
    }

    const int esize = (mFaceData.mType==piRMVEDT_Polys) ? sizeof(piMeshFace) : sizeof(unsigned int);
    fp.WriteUInt32(  (mFaceData.mType==piRMVEDT_Polys)?0:1 );
    fp.WriteUInt32(  mFaceData.mNumIndexArrays );
    for( int i=0; i<mFaceData.mNumIndexArrays; i++ )
    {
        fp.WriteUInt32(  mFaceData.mIndexArray[i].mNum );

		if( mFaceData.mIndexArray[i].mNum>0 )
		{
			fp.WriteUInt32array( (unsigned int*)mFaceData.mIndexArray[i].mBuffer, mFaceData.mIndexArray[i].mNum*esize/4 );
		}
	}

	fp.Close();

	return( 1 );
}

bool piMesh::Load( const wchar_t *name )
{
    piFile fp;
    if( !fp.Open(name, L"rb") )
		return false;

    fp.ReadFloatarray2( (float*)&mBBox, 6 );

    mVertexData.mNumVertexArrays = fp.ReadUInt32();

    for (int j = 0; j<mVertexData.mNumVertexArrays; j++)
    {
        piMeshVertexArray *va = mVertexData.mVertexArray + j;

	    va->mNum = fp.ReadUInt32();
        va->mFormat.mStride = fp.ReadUInt32();
        va->mFormat.mDivisor = fp.ReadUInt32();
        va->mFormat.mNumElems = fp.ReadUInt32();
        va->mMax = va->mNum;

        for( int i=0; i<va->mFormat.mNumElems; i++ )
	    {
            va->mFormat.mElems[i].mType = (piMeshVertexElemDataType)fp.ReadUInt32();
            va->mFormat.mElems[i].mNumComponents = fp.ReadUInt32();
            va->mFormat.mElems[i].mNormalize = (fp.ReadUInt32() == 1);
            va->mFormat.mElems[i].mOffset = fp.ReadUInt32();
	    }

        if (va->mNum>0)
        {
            va->mBuffer = malloc(va->mNum*va->mFormat.mStride);
            if (!va->mBuffer)
                return false;

            fp.ReadFloatarray2( (float*)va->mBuffer, va->mNum*va->mFormat.mStride / 4);
        }

    }

  
    mFaceData.mType = (fp.ReadUInt32( )==0)?piRMVEDT_Polys:piRMVEDT_Points;
    mFaceData.mNumIndexArrays = fp.ReadUInt32();
    const int esize = (mFaceData.mType==piRMVEDT_Polys) ? sizeof(piMeshFace) : sizeof(unsigned int);
    for( int i=0; i<mFaceData.mNumIndexArrays; i++ )
    {
		mFaceData.mIndexArray[i].mNum = fp.ReadUInt32();
		mFaceData.mIndexArray[i].mMax = mFaceData.mIndexArray[i].mNum;
		if( mFaceData.mIndexArray[i].mNum>0 )
		{
			mFaceData.mIndexArray[i].mBuffer = (piMeshFace*)malloc( mFaceData.mIndexArray[i].mNum*esize );
			if( !mFaceData.mIndexArray[i].mBuffer )
				return false;

            fp.ReadUInt32array(  (unsigned int*)mFaceData.mIndexArray[i].mBuffer, mFaceData.mIndexArray[i].mNum*esize/4 );
		}
    }
  
	fp.Close();

	return true;
}


#if 0
int piMesh_MoveStream(piMesh *me, int dst, int ori )
{
    pStream[dst] = pStream[ori];
    memset( pStream+ori, 0, sizeof(piMeshStream) );
    return( 1 );
}
 
int piMesh_NewStream( piMesh *me, int num, const int *chunktypes, int *pos )
{
    for( int i=0; i<MAX_STREAMS; i++ )
	{
		piMeshStream *st = pStream + i;
        if( st->stride==0 )
        {
            st->num_chunks = 0;
            st->stride = 0;
            for( int j=0; chunktypes[j]!=0; j++ )
            {
                st->num_chunks++;
                st->chunktype[j] = chunktypes[j];
                st->stride += piMesh_getchunksize( chunktypes[j] );
            }

            if( num )
            {
                size_t amount = (size_t)num*(size_t)st->stride*(size_t)sizeof(float);
                st->buffer = (float*)malloc( amount );
                if( !st->buffer )
                {
//                    LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "Could not allocate %d megabytes\n", amount>>20 );
				    return( 0 );
                }
            }
            *pos = i;
            return( 1 );
        }
	}

    return( 0 );

}




/*
int  piMesh_Expand( piMesh *me, int nnv, int nnf, int *vbase, int *fbase )
{
    piMeshFace  *nf;
    int     i;
    float   *ns[MAX_STREAMS];
    long amount;

    nf = (piMeshFace*)malloc( (pNumFaces+nnf)*sizeof(piMeshFace) );
    if( !nf )
        {
        //LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", ((pNumFaces+nnf)*sizeof(piMeshFace))>>20 );
		return( 0 );
        }
	
    if( pNumFaces )
        memcpy( nf, pFace, pNumFaces*sizeof(piMeshFace) );
    

    for( i=0; i<MAX_STREAMS; i++ )
        if( pStream[i].stride )
            {
	    amount = ((size_t)pNumVertices+(size_t)nnv)*(size_t)pStream[i].stride*(size_t)sizeof(float);
            ns[i] = (float*)malloc( amount );
            if( !ns[i] )
                {
                //LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", amount>>20 );
                free( nf );
                return( 0 );
                }
            }

    if( pNumVertices )
        for( i=0; i<MAX_STREAMS; i++ )
            if( pStream[i].stride )
	        {
		amount = (long)pNumVertices*(long)pStream[i].stride*(long)sizeof(float);
                memcpy( ns[i], pStream[i].buffer, amount );
		}
    
    if( pNumFaces )    
        free( pFace );
    pFace = nf;

    
    for( i=0; i<MAX_STREAMS; i++ )
        if( pStream[i].stride )
            {
            if( pNumVertices )
                free( pStream[i].buffer );
            pStream[i].buffer = ns[i];
            }

    *vbase = pNumVertices;
    *fbase = pNumFaces;

    pNumVertices += nnv;
    pNumFaces += nnf;

    return( 1 );
}
*/
#include <stdio.h>
int piMesh::Load( const wchar_t *name )
{
    int		i, j;
    piMeshStream *st;
    size_t amount;


    FILE *fp = piFile_Open( name, L"rb" );
    if( !fp )
        return( 0 );

    memset( me, 0, sizeof(piMesh) );

    //LOG_Printf( "sizeof(DMATTABLE) = %d\n", sizeof(DMATTABLE) );
    
	pNumVertices = piFile_readUInt32( fp );
    pNumFaces = piFile_readUInt32( fp );
//   pMatTable.numMats = 0;

    for( i=0; i<MAX_STREAMS; i++ )
        {
        st = pStream+i;

        st->num_chunks = piFile_readUInt32( fp );
        st->stride = 0;
        if( st->num_chunks )
            {
            for( j=0; j<st->num_chunks; j++ )
                {
                st->chunktype[j] = piFile_readUInt32( fp );
                st->stride += piMesh_getchunksize( st->chunktype[j] );
                }
            }
        }



    amount = (size_t)pNumFaces*(size_t)sizeof(piMeshFace);

    pFace = (piMeshFace*)malloc( amount );
    if( !pFace )
        {
        //LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", amount>>20 );
		return( 0 );
        }

    piFile_readUInt32array( fp, (unsigned int*)pFace, pNumFaces, 3, 4 );
    //fread( (unsigned int*)pFace, pNumFaces, 3*4, fp );

    for( i=0; i<MAX_STREAMS; i++ )
        {
        st = pStream+i;
        if( st->num_chunks )
            {
            amount = (size_t)pNumVertices*(size_t)st->stride*(size_t)sizeof(float);
            st->buffer = (float*)malloc( amount );
            if( !st->buffer )
                {
                //LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", amount>>20 );
			    return( 0 );
                }
            piFile_readfloatarray2( fp, st->buffer, (size_t)(pNumVertices*st->stride) );
            }
        }


    fp.Close();

    return( 1 );
}

*/


/*

void piMesh_FreeStream( piMesh *me, int i )
{
    if( pStream[i].stride )
		{
        pStream[i].stride = 0;

        if( pStream[i].buffer )
		    free( pStream[i].buffer );
        
        pStream[i].buffer = 0;
		}
}




void piMesh_Free( piMesh *me )
{
    int	i;

    if( pFace ) 
        {
        free( pFace );
        pFace = 0;
        }

	for( i=0; i<MAX_STREAMS; i++ )
        piMesh_FreeStream( me, i );

//   DMATTABLE_Free( &pMatTable );
}

void piMesh_Scale( piMesh *me, float x, float y, float z )
{
	piVec3F *v = (piVec3F*)pStream[0].buffer;
	for( unsigned i=0; i<pNumVertices; i++, v++ )
	{
		v->x *= x;
		v->y *= y;
		v->z *= z;
	}
}


void piMesh_CalcBBOx( piMesh *me, float *bbox )
{
    piVec3F *v = (piVec3F*)pStream[0].buffer;

	bbox[0] = v[0].x;
	bbox[1] = v[0].y;
	bbox[2] = v[0].z;
	bbox[3] = v[0].x;
	bbox[4] = v[0].y;
	bbox[5] = v[0].z;

    for( unsigned int i=0; i<pNumVertices; i++, v++ )
	{
		const float x = v[0].x;
		const float y = v[0].y;
		const float z = v[0].z;

		if( x<bbox[0] ) bbox[0]=x;
		else if( x>bbox[3] ) bbox[3]=x;

		if( y<bbox[1] ) bbox[1]=y;
		else if( y>bbox[4] ) bbox[4]=y;

		if( z<bbox[2] ) bbox[2]=z;
		else if( z>bbox[5] ) bbox[5]=z;
	}
}

//--------------------------------------

static void polynorm( float *pos, int pos_inc, float *nor, int nor_inc, int ia, int ib, int ic )
{
    piVec3F  ab, cb, no;

    piVec3F_Sub( &ab, (piVec3F*)(pos+ia*pos_inc), (piVec3F*)(pos+ib*pos_inc) );
    piVec3F_Sub( &cb, (piVec3F*)(pos+ic*pos_inc), (piVec3F*)(pos+ib*pos_inc) );

    piVec3F_Cross( &ab, &cb, &no );
    //Vec3Norm( &no );

    piVec3F_Inc( (piVec3F*)(nor+ia*nor_inc), &no );
    piVec3F_Inc( (piVec3F*)(nor+ib*nor_inc), &no );
    piVec3F_Inc( (piVec3F*)(nor+ic*nor_inc), &no );
}

void piMesh_CalcNormals( piMesh *me, int pos_stream, int pos_stride, int pos_offset,
								   int nor_stream, int nor_stride, int nor_offset,
								   float ang_th )
{
	unsigned int	    i;
	float	*nor_ptr;
	float	*pos_ptr;
	int		nor_inc, pos_inc;
	float	*n;
    piMeshFace   *f;

	pos_ptr = pStream[pos_stream].buffer + pos_offset;
	pos_inc = pos_stride / sizeof(float);
	nor_ptr = pStream[nor_stream].buffer + nor_offset;
	nor_inc = nor_stride / sizeof(float);

	ang_th = ang_th*3.1415927f/180.0f;

	n = nor_ptr;
	for( i=0; i<pNumVertices; i++, n += nor_inc )
		{
		n[0] = 0.0f;
		n[1] = 0.0f;
		n[2] = 0.0f;
		}

    f = pFace;
    for( i=0; i<pNumFaces; i++, f++ )
		{
        polynorm( pos_ptr, pos_inc,
				  nor_ptr, nor_inc,
				  f->i[0], f->i[1], f->i[2] );
		}

	n = nor_ptr;
	for( i=0; i<pNumVertices; i++, n += nor_inc )
        piVec3F_Norm( (piVec3F*)n );
}


//-------------------------------------------

int piMesh_Merge( piMesh *me )
{
	float *buffer;
	unsigned int		i;
	float	*fa, *fb, *fc;

	buffer = (float*)malloc( pNumVertices*8*sizeof(float) );
	if( !buffer )
    {
        //LOG_Printf( LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", pNumVertices*8*sizeof(float) );
		return( 0 );
    }

	fa = (float*)pStream[0].buffer;
	fb = (float*)pStream[1].buffer;

	fc = buffer;
	for( i=0; i<pNumVertices; i++ )
		{
		fc[0] = fa[0];
		fc[1] = fa[1];
		fc[2] = fa[2];
		fc[3] = fb[0];
		fc[4] = fb[1];
		fc[5] = fb[2];
		fc[6] = fb[3];
		fc[7] = fb[4];
		fc += 8;
		fa += 3;
		fb += 5;
		}

	piMesh_FreeStream( me, 0 );
	piMesh_FreeStream( me, 1 );

	pStream[0].num_chunks = 3;
	pStream[0].stride = 3+3+2;
	pStream[0].buffer = buffer;

	return( 1 );
}









//============================================================
//include "f_mesh.h"

int piMesh_SerialSave_Open( SESA_DATA *sd, const wchar_t *name )
{
	FILE *fp = piFile_Open( name, L"wb" );
	if( !fp )
		return( 0 );

	piFile_writeUInt32( fp, 0 );   // nv
	piFile_writeUInt32( fp, 0 );   // nf

    sd->me.pNumFaces = 0;
    sd->me.pNumVertices = 0;
    sd->actual_stream = 0;
    sd->fp = fp;

    return( 1 );
}


void piMesh_SerialSave_AddStride_Add( SESA_DATA *sd, int *chunktypes )
{
    int i;
    int num_chunks = 0;

    for( i=0; chunktypes[i]!=0; i++ )
        num_chunks++;
 
    piFile_writeUInt32( sd->fp, num_chunks );

    sd->me.pStream[sd->actual_stream].stride = 0;
    if( num_chunks )
        {
        for( i=0; chunktypes[i]!=0; i++ )
            {
            piFile_writeUInt32( sd->fp, chunktypes[i] );
            sd->me.pStream[sd->actual_stream].stride += piMesh_getchunksize( chunktypes[i] );
            }
        }

    sd->actual_stream++;
}

void piMesh_SerialSave_AddStride_End( SESA_DATA *sd )
{
    int i;

	for( i=sd->actual_stream; i<MAX_STREAMS; i++ )
        piFile_writeUInt32( sd->fp, 0 );

    sd->actual_stream = 0;
}

void piMesh_SerialSave_AdpiMeshFace( SESA_DATA *sd, piMeshFace *face )
{
	piFile_writeUInt32array( sd->fp, (unsigned int*)face, sizeof(piMeshFace)/4 );
    sd->me.pNumFaces++;
}

void piMesh_SerialSave_AddVertex_StartStream( SESA_DATA *sd )
{
    sd->actual_stream = 0;
}


void piMesh_SerialSave_AddVertex_Add( SESA_DATA *sd, float *data )
{
	piFile_writeUInt32array( sd->fp, (unsigned int*)data, sd->me.pStream[sd->actual_stream].stride );

    if( sd->actual_stream==0 )
        sd->me.pNumVertices++;
}

void piMesh_SerialSave_AddVertex_EnpiMeshStream( SESA_DATA *sd )
{
    sd->actual_stream++;
}


void piMesh_SerialSave_Close( SESA_DATA *sd )
{
    fseek( sd->fp, 0, SEEK_SET );

	piFile_writeUInt32( sd->fp, sd->me.pNumVertices );
	piFile_writeUInt32( sd->fp, sd->me.pNumFaces );

	piFile_Close( sd->fp );
}

int piMesh_Check( piMesh *me )
{
    unsigned int i, j;

    unsigned int numf = pNumFaces;
    unsigned int numv = pNumVertices;

    // check VA
    for( j=0; j<MAX_STREAMS; j++ )
        {
        if( pStream[j].num_chunks )
            {
            float *fptr = pStream[j].buffer;
            int   *iptr = (int*)pStream[j].buffer;
            int   s = pStream[j].stride;
            for( i=0; i<s*numv; i++ )
                if( _isnan((double)fptr[i]) || !_finite((double)fptr[i]) || iptr[i]==0xffffffaa )
                    return( 0 );
            }
        }

    // check IA
    for( i=0; i<numf; i++ )
        {
        if( pFace[i].i[0]>=numv )
            return( 0 );
        if(  pFace[i].i[1]>=numv )
            return( 0 );
        if( pFace[i].i[2]>=numv )
            return( 0 );
        }

    return 1;
}

//------------------------------------------
/*
typedef struct
{
    int a;
    int b;
}TmpEdge;

void insertEdge( ARRAY *edges, int a, int b )
{
    TmpEdge      *e;
    unsigned int i;

    if( a>b ) { int t=a; a=b; b=t; } 

    unsigned int num = ARRAY_GetLength( edges );
    e = (TmpEdge*)ARRAY_GetPointer( edges, 0 );
    for( i=0; i<num; i++ )
        if( e[i].a==a && e[i].b==b )
            return;

    e = (TmpEdge *)ARRAY_Alloc( edges, 1 );
    if( !e )
        return;
    e->a = a;
    e->b = b;
    
}

int piMesh_CalcEdges( const piMesh *me, ARRAY *edges )
{
    unsigned int i;
    unsigned int numF = pNumFaces;

    if( !ARRAY_Init( edges, numF*3, sizeof(TmpEdge) ) )
        return( 0 );

    for( i=0; i<numF; i++ )
        {
        int a = pFace[i].i[0];
        int b = pFace[i].i[1];
        int c = pFace[i].i[2];

        insertEdge( edges, a, b );
        insertEdge( edges, b, c );
        insertEdge( edges, c, a );
        }

    
    return( 1 );
}*/
#endif

void piMesh::Normalize( int stream, int ppos, int npos )
{
    const int numv = mVertexData.mVertexArray[stream].mNum;
    const int numt = mFaceData.mIndexArray[0].mNum;

    for( int i=0; i<numv; i++ )
    {
        float *v = (float*)GetVertexData( stream, i, npos );
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
    }

    for( int i=0; i<numt; i++ )
    {
        piMeshFace *face = mFaceData.mIndexArray[0].mBuffer + i;

        const int ft = face->mNum;
        
        vec3 nor = vec3( 0.0f, 0.0f, 0.0f );
        for( int j=0; j<ft; j++ )
        {
            const vec3 va = *((vec3*)GetVertexData( stream, face->mIndex[ j      ], ppos ));
            const vec3 vb = *((vec3*)GetVertexData( stream, face->mIndex[(j+1)%ft], ppos ));

            nor += cross( va, vb );
        }

        for( int j=0; j<ft; j++ )
        {
            vec3 *n = (vec3*)GetVertexData( stream, face->mIndex[j], npos );
            n->x += nor.x;
            n->y += nor.y;
            n->z += nor.z;
        }
    }

    for( int i=0; i<numv; i++ )
    {
        vec3 *v = (vec3*)GetVertexData( stream, i, npos );
        *v = normalize( *v );
    }
}

void piMesh::CalcBBox( int stream, int pPos )
{
    mBBox = bound3(1e20f, -1e20f, 1e20f, -1e20f, 1e20f, -1e20f );

    const int num = mVertexData.mVertexArray[stream].mNum;
    for( int i=0; i<num; i++ )
    {
        float *v = (float*)GetVertexData( stream, i, pPos );
        if (v[0]<mBBox.mMinX) mBBox.mMinX = v[0];
        if (v[1]<mBBox.mMinY) mBBox.mMinY = v[1];
        if (v[2]<mBBox.mMinZ) mBBox.mMinZ = v[2];
        if (v[0]>mBBox.mMaxX) mBBox.mMaxX = v[0];
        if (v[1]>mBBox.mMaxY) mBBox.mMaxY = v[1];
        if (v[2]>mBBox.mMaxZ) mBBox.mMaxZ = v[2];
    }
}

bool piMesh::Expand( int nv, int nf )
{
    for( int j=0; j<mVertexData.mNumVertexArrays; j++ )
    {
        piMeshVertexArray *va = mVertexData.mVertexArray + j;

        if( (va->mNum + nv) >= va->mMax )
        {
            const unsigned int newNV = va->mMax + ((nv<64)?64:nv);
            va->mBuffer = realloc( va->mBuffer, newNV*va->mFormat.mStride );
            if( !va->mBuffer ) 
                return false;
            va->mMax = newNV;
        }

        if( (mFaceData.mIndexArray[0].mNum + nf) >= mFaceData.mIndexArray[0].mMax )
        {
            const unsigned int newNF = mFaceData.mIndexArray[0].mMax + ((nf<64)?64:nf);
            mFaceData.mIndexArray[0].mBuffer = (piMeshFace*)realloc( mFaceData.mIndexArray[0].mBuffer, newNF*sizeof(piMeshFace) );
            if( !mFaceData.mIndexArray[0].mBuffer )   
                return false;
            mFaceData.mIndexArray[0].mMax = newNF;
        }
    }

    return true;
}

/*
void piMesh_RandomizeVerts( piMesh *me )
{
    const int num = mVertexData.mNum;
    const int stride = mVertexData.mFormat.mStride;
    char tmp[32*4];
    int seed = 123456;
    for( int i=0; i<num; i++ )
    {
        int j = pirand_int15b( &seed ) % num;

        char *ptrA = (char*)mVertexData.mBuffer + stride*i;
        char *ptrB = (char*)mVertexData.mBuffer + stride*j;

        memcpy( tmp, ptrA, stride );
        memcpy( ptrA, ptrB, stride );
        memcpy( ptrB, tmp, stride );
    }
}*/

int piMesh::Compact( void )
{

    for( int j=0; j<mVertexData.mNumVertexArrays; j++ )
    {
        piMeshVertexArray *va = mVertexData.mVertexArray + j;

	    const int numSrcVerts = va->mNum;

	    const int stride = va->mFormat.mStride;

	    piArray tmp;
	    if( !tmp.Init( numSrcVerts, stride ) )
		    return 0;

	    int *tmpHis = (int*)malloc( numSrcVerts * sizeof(int) );
	    if( !tmpHis )
		    return 0;
	    memset( tmpHis, 0, numSrcVerts * sizeof(int) );

	    const int numf = mFaceData.mIndexArray[0].mNum;
	    for( int i=0; i<numf; i++ )
	    {
		    const piMeshFace *fa = mFaceData.mIndexArray[0].mBuffer + i;
		    const int nv = fa->mNum;
		    for( int j=0; j<nv; j++ )
		    {
			    tmpHis[ fa->mIndex[j] ] += 1;
		    }
	    }

	    int num = 0;
	    for( int i=0; i<numSrcVerts; i++ )
	    {
		    if( tmpHis[i] > 0 )
		    {
                if (!tmp.Append((char*)va->mBuffer + stride*i))
				    return 0;

			    // change all references
			    for( int j=0; j<numf; j++ )
			    {
				    piMeshFace *fa = mFaceData.mIndexArray[0].mBuffer + j;
				    const int nv = fa->mNum;
				    for( int k=0; k<nv; k++ )
				    {
					    if( fa->mIndex[k] == i )
                            fa->mIndex[k] = num;
				    }
			    }
			    num++;
		    }
	    }

	    free( va->mBuffer );
	    va->mBuffer = malloc( num*stride );
        if( !va->mBuffer )
            return false;
        memcpy(va->mBuffer, tmp.GetAddress(0), num*stride);
	    va->mNum = num;
	    va->mMax = num;

	    free( tmpHis );
        tmp.End();
    }

	return 1;
}

/*
void piMesh_CopyChannel(piMesh *mesh, int stream, int src, int dst)
{
    const int nv = mesh->mVertexData.mNum;
    for (int i = 0; i<nv; i++)
    {
        vec3 *s = (vec3*)piMesh_GetVertexData(mesh, i, src);
        vec3 *d = (vec3*)piMesh_GetVertexData(mesh, i, dst);
        d[0] = s[0];
    }
}
*/

int piMesh::GetVertexSize( int streamID)
{
    return mVertexData.mVertexArray[streamID].mFormat.mStride;
}

void piMesh::GetVertex( int streamID, int vertexID, void *data)
{
    memcpy(data, (char*)mVertexData.mVertexArray[streamID].mBuffer + mVertexData.mVertexArray[streamID].mFormat.mStride*vertexID, mVertexData.mVertexArray[streamID].mFormat.mStride);
}

void piMesh::SetVertex( int streamID, int vertexID, void *data)
{
    memcpy((char*)mVertexData.mVertexArray[streamID].mBuffer + mVertexData.mVertexArray[streamID].mFormat.mStride*vertexID, data, mVertexData.mVertexArray[streamID].mFormat.mStride );
}

bool piMesh::SetTriangle( int streamID, int triangleID, int a, int b, int c )
{
    piMeshFace *face = mFaceData.mIndexArray[streamID].mBuffer + triangleID;

    face->mNum = 3;
    face->mIndex[0] = a;
    face->mIndex[1] = b;
    face->mIndex[2] = c;

    return true;
}

void *piMesh::GetVertexData( int streamID, int vertexID, int elementID ) const
{
    return (void*)((char*)mVertexData.mVertexArray[streamID].mBuffer + vertexID*mVertexData.mVertexArray[streamID].mFormat.mStride + mVertexData.mVertexArray[streamID].mFormat.mElems[elementID].mOffset );
}


}