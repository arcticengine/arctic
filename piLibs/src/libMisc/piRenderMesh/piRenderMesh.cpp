#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <float.h>

#include "../../libSystem/piFile.h"
#include "../../libMath/piVecTypes.h"
#include "../../libDataUtils/piRadixSort.h"
#include "piRenderMesh.h"

namespace piLibs {


static piRArrayDataType dt2al( piMeshVertexElemDataType dt )
{
    if( dt==piRMVEDT_UByte  ) return piRArrayType_UByte;
    if( dt==piRMVEDT_Float  ) return piRArrayType_Float;
    if( dt==piRMVEDT_Int    ) return piRArrayType_Int;
    if( dt==piRMVEDT_Double ) return piRArrayType_Double;
    return piRArrayType_Float;
}


piRenderMesh::piRenderMesh()
{
}

piRenderMesh::~piRenderMesh()
{
}

const bound3 & piRenderMesh::GetBBox( void ) const
{
    return mBBox;
}

const int piRenderMesh::GetNumVertices( void ) const
{
    return mVertexData.mStream[0].mLength;
}

const piBuffer piRenderMesh::GetVertexBuffer( int stream ) const
{
    return mVertexData.mStream[stream].mVBO;
}


void piRenderMesh::End( piRenderer *renderer )
{
    for( unsigned int i=0; i<mVertexData.mNumStreams; i++ )
    {
        piRenderMeshVertexStream *stream = mVertexData.mStream + i;
        renderer->DestroyBuffer(stream->mVBO);
    }

    if( mIndexData.mNumElementArrays == 0)
    {
        renderer->DestroyVertexArray( mIndexData.mVAO );
    }
    else
    {
        for (unsigned int i = 0; i<mIndexData.mNumElementArrays; i++)
        {
            if( mIndexData.mElementArray[i].mNum != 0 )
            {
	            renderer->DestroyBuffer( mIndexData.mElementArray[i].mIBO );
                renderer->DestroyVertexArray( mIndexData.mElementArray[i].mVAO);
            }
        }
    }
}
/*
int piRenderMesh_AddStream(piRenderMesh *me, int length, const piRArrayLayout *layout)
{
    const int sid = me->mVertexData.mNumStreams;

    piRenderMeshVertexStream *stream = me->mVertexData.mStream + sid;

    stream->mLength = length;
    stream->mLayout.mStride = layout->mStride;
    stream->mLayout.mNumElements = layout->mNumElements;
    stream->mLayout.mDivisor = layout->mDivisor;
    stream->mVBO = 0;

    for (int j = 0; j<layout->mNumElements; j++)
    {
        stream->mLayout.mEntry[j].mNumComponents = layout->mEntry[j].mNumComponents;
        stream->mLayout.mEntry[j].mType = layout->mEntry[j].mType;
        stream->mLayout.mEntry[j].mNormalize = layout->mEntry[j].mNormalize;
    }

    stream->mBuffer = malloc(stream->mLength*stream->mLayout.mStride);
    if (!stream->mBuffer)
        return -1;

    me->mVertexData.mNumStreams += 1;

    return sid;
}

void piRenderMesh_SetVertex(piRenderMesh *me, int streamID, int vertexID, void *data)
{
    piRenderMeshVertexStream *stream = me->mVertexData.mStream + streamID;

    memcpy((char*)stream->mBuffer + stream->mLayout.mStride*vertexID, data, stream->mLayout.mStride );
}

static int getIndicesPerElement(piPrimitiveType type)
{
    int indicesPerElement = 1;
    if (type == piPT_Triangle)  indicesPerElement = 3;
    if (type == piPT_QuadPatch) indicesPerElement = 4;
    return indicesPerElement;
}


int piRenderMesh_AddElements(piRenderMesh *me, piPrimitiveType type, int length)
{
    const int indicesPerElement = getIndicesPerElement( type );

    int sid = 0;

    me->mIndexData.mType = type;
    me->mIndexData.mNumElementArrays = 1;
    me->mIndexData.mElementArray[sid].mNum = length * indicesPerElement;
    me->mIndexData.mElementArray[sid].mBuffer = (unsigned int*)malloc(me->mIndexData.mElementArray[sid].mNum*sizeof(unsigned int));
    if (!me->mIndexData.mElementArray[sid].mBuffer)
        return -1;

//    me->mIndexData.mElementArray[sid].mTmpRadix = nullptr;
    //me->mIndexData.mElementArray[sid].mSortedBuffer = nullptr;

    return 0;
}

bool piRenderMesh_AddTriangle(piRenderMesh *me, int streamID, int triangleID, int a, int b, int c)
{
    piRenderMeshElementArray *stream = me->mIndexData.mElementArray + streamID;

    const int indicesPerElement = getIndicesPerElement( me->mIndexData.mType );

    stream->mBuffer[ indicesPerElement*triangleID + 0] = a;
    stream->mBuffer[ indicesPerElement*triangleID + 1] = b;
    stream->mBuffer[ indicesPerElement*triangleID + 2] = c;

    return true;
}

bool piRenderMesh_MakeSortable(piRenderMesh *me )
{
    int sid = 0;

    int numI = me->mIndexData.mElementArray[sid].mNum;
    int numE = numI / 3;

    me->mIndexData.mElementArray[sid].mSortedBuffer = (unsigned int*)malloc(numI*sizeof(unsigned int));
    if (!me->mIndexData.mElementArray[sid].mSortedBuffer)
        return false;

    me->mIndexData.mElementArray[sid].mTmpRadix = (unsigned int*)malloc(2 * 2 * numE*sizeof(unsigned int));
    if (!me->mIndexData.mElementArray[sid].mTmpRadix)
        return false;

    return true;
}
*/



bool piRenderMesh::InitFromMesh( piRenderer *renderer, const piMesh *mesh, piPrimitiveType patchNum)
{
    memset( this, 0, sizeof(piRenderMesh) );
    mVertexData.mNumStreams = 0;
    mIndexData.mNumElementArrays = 0;

    //------------------------------------------------------------

    if (mesh->mFaceData.mType == piRMVEDT_Points && patchNum != piRMVEDT_Points ) return false;
    //if (mesh->mFaceData.mType == piRMVEDT_Polys  && patchNum != piPT_Triangle) return false;


    mBBox = mesh->mBBox;

    mVertexData.mNumStreams = mesh->mVertexData.mNumVertexArrays;

    for( unsigned int k=0; k<mVertexData.mNumStreams; k++ )
    {
        const piMeshVertexArray *mva = mesh->mVertexData.mVertexArray + k;
        piRenderMeshVertexStream *stream = mVertexData.mStream + k;

        stream->mLength = mva->mNum;
        stream->mLayout.mStride = mva->mFormat.mStride;
        stream->mLayout.mNumElements = mva->mFormat.mNumElems;
        stream->mLayout.mDivisor = mva->mFormat.mDivisor;
        stream->mVBO = 0;

        for( int j=0; j<mva->mFormat.mNumElems; j++ )
        {
            stream->mLayout.mEntry[j].mNumComponents = mva->mFormat.mElems[j].mNumComponents;
            stream->mLayout.mEntry[j].mType = dt2al( mva->mFormat.mElems[j].mType );
            stream->mLayout.mEntry[j].mNormalize = mva->mFormat.mElems[j].mNormalize;
        }

        stream->mBuffer = malloc(stream->mLength*stream->mLayout.mStride);
        if( !stream->mBuffer )
            return false;

        memcpy(stream->mBuffer, mesh->mVertexData.mVertexArray[k].mBuffer, stream->mLength*stream->mLayout.mStride);
    }

    //------------------------------------------------------------

    if (mesh->mFaceData.mType == piPT_Point)
    {
        mIndexData.mType = piPT_Point;
		mIndexData.mNumElementArrays = mesh->mFaceData.mNumIndexArrays;
		for( int i=0; i<mesh->mFaceData.mNumIndexArrays; i++ )
		{
            const int num = mesh->mFaceData.mIndexArray[i].mNum;
			mIndexData.mElementArray[i].mNum = num;
			if( num>0 )
			{
				mIndexData.mElementArray[i].mBuffer = (unsigned int*)malloc( num*sizeof(unsigned int) );
				if( !mIndexData.mElementArray[i].mBuffer )
					return false;
                memcpy( mIndexData.mElementArray[i].mBuffer, mesh->mFaceData.mIndexArray[i].mBuffer, num*sizeof(unsigned int) );
            }
        }
        return true;
    }

    //------------------------------------------------------------
    mIndexData.mType = patchNum;
    mIndexData.mNumElementArrays = mesh->mFaceData.mNumIndexArrays;
    for( int i=0; i<mesh->mFaceData.mNumIndexArrays; i++ )
    {
		mIndexData.mElementArray[i].mNum = 0;
		if( mesh->mFaceData.mIndexArray[i].mNum>0 )
		{
            if (patchNum == piPT_QuadPatch)
			{
				const unsigned int nf = mesh->mFaceData.mIndexArray[i].mNum;
				mIndexData.mElementArray[i].mNum = nf * 4;
				mIndexData.mElementArray[i].mBuffer = (unsigned int*)malloc( mIndexData.mElementArray[i].mNum*sizeof(unsigned int) );
				if( !mIndexData.mElementArray[i].mBuffer )
					return false;
				unsigned int *ind = mIndexData.mElementArray[i].mBuffer;
				for( unsigned int j=0; j<nf; j++ )
				{
					ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
					ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[1];
					ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
					ind[3] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[3];
					ind += 4;
				}
			}
			else
			{
				const unsigned int nf = mesh->mFaceData.mIndexArray[i].mNum;
				int  nt = 0;
				for( unsigned int j=0; j<nf; j++ ) 
				{ 
					const int fn = mesh->mFaceData.mIndexArray[i].mBuffer[j].mNum;
					nt += (fn-2);
				}

				mIndexData.mElementArray[i].mNum = nt * 3;
				mIndexData.mElementArray[i].mBuffer = (unsigned int*)malloc( mIndexData.mElementArray[i].mNum*sizeof(unsigned int) );
				if( !mIndexData.mElementArray[i].mBuffer )
					return false;

				unsigned int *ind = mIndexData.mElementArray[i].mBuffer;

				for( unsigned int j=0; j<nf; j++ )
				{
					const int fn = mesh->mFaceData.mIndexArray[i].mBuffer[j].mNum;
					if( fn==3 )
					{
						ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
						ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[1];
						ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
						ind += 3;
					}
					else
					{
						ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
						ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[1];
						ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
						ind += 3;
						ind[0] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[0];
						ind[1] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[2];
						ind[2] = mesh->mFaceData.mIndexArray[i].mBuffer[j].mIndex[3];
						ind += 3;
					}
				}
			}
		}
	}


    //-----------------------------------------------------------------------------------------
    for( unsigned int k=0; k<mVertexData.mNumStreams; k++ )
    {
        piRenderMeshVertexStream * stream = mVertexData.mStream + k;

        stream->mVBO = renderer->CreateBuffer(stream->mBuffer, stream->mLayout.mStride*stream->mLength, piBufferType_Static);
	    if( !stream->mVBO )
		    return false;
    }

    if(mIndexData.mNumElementArrays==0 )
    {
        mIndexData.mVAO = renderer->CreateVertexArray( mVertexData.mNumStreams, mVertexData.mStream[0].mVBO, &mVertexData.mStream[0].mLayout,
                                                                                mVertexData.mStream[1].mVBO, &mVertexData.mStream[1].mLayout,
                                                                                nullptr );
        if (!mIndexData.mVAO)
            return false;
    }
    else
    {
        for( unsigned int i=0; i<mIndexData.mNumElementArrays; i++ )
        {
		    if( mIndexData.mElementArray[i].mNum==0 )
		    {
			    mIndexData.mElementArray[i].mIBO = 0;
		    }
		    else
		    {
                mIndexData.mElementArray[i].mIBO = renderer->CreateBuffer( mIndexData.mElementArray[i].mBuffer, mIndexData.mElementArray[i].mNum*sizeof(unsigned int), piBufferType_Static);
			    if( !mIndexData.mElementArray[i].mIBO )
				    return false;

                mIndexData.mElementArray[i].mVAO = renderer->CreateVertexArray( mVertexData.mNumStreams, 
                                                                                mVertexData.mStream[0].mVBO, &mVertexData.mStream[0].mLayout,
                                                                                mVertexData.mStream[1].mVBO, &mVertexData.mStream[1].mLayout,
                                                                                mIndexData.mElementArray[i].mIBO );
                if( !mIndexData.mElementArray[i].mVAO )
                    return false;
		    }
        }
    }

    return true;
}



void piRenderMesh::Render( piRenderer *renderer, int elementArrayID, int numInstances ) const
{
    if( mIndexData.mNumElementArrays==0 )
    {
        renderer->AttachVertexArray( mIndexData.mVAO );
        renderer->DrawPrimitiveNotIndexed(mIndexData.mType, 0, mVertexData.mStream[0].mLength, numInstances);
        renderer->DettachVertexArray();
    }
    else
    {
        renderer->AttachVertexArray( mIndexData.mElementArray[elementArrayID].mVAO );
        renderer->DrawPrimitiveIndexed(mIndexData.mType, mIndexData.mElementArray[elementArrayID].mNum, numInstances, 0, 0);
        renderer->DettachVertexArray();
    }
}


//---------------
/*
bool piRenderMesh_UploadDynamicIA(piRenderMesh *me, piRenderer *renderer, int streamID)
{
    piRenderMeshVertexStream *stream = me->mVertexData.mStream + streamID;

    stream->mVBO = renderer->CreateBuffer(stream->mBuffer, stream->mLayout.mStride*stream->mLength, piBufferType_Static);
    if (!stream->mVBO)
        return false;

    me->mIndexData.mElementArray[0].mIBO = renderer->CreateBuffer(me->mIndexData.mElementArray[0].mBuffer, me->mIndexData.mElementArray[0].mNum*sizeof(unsigned int), piBufferType_Dynamic);
    if (!me->mIndexData.mElementArray[0].mIBO)
        return false;

    me->mIndexData.mElementArray[0].mVAO = renderer->CreateVertexArray(stream->mVBO, me->mIndexData.mElementArray[0].mIBO, &stream->mLayout);
    if (!me->mIndexData.mElementArray[0].mVAO)
        return false;

    return true;
}*/
/*
void piRenderMesh_Sort( piRenderMesh *me, piRenderer *renderer, const vec3 & ro, const vec3 & rd )
{
    unsigned int *lTmpRadix = me->mIndexData.mElementArray[0].mTmpRadix;
    unsigned int *lSortedBuffer = me->mIndexData.mElementArray[0].mSortedBuffer;
    unsigned int *lSrcIndexBuffer = me->mIndexData.mElementArray[0].mBuffer;

    unsigned int *data = me->mIndexData.mElementArray[0].mBuffer;
    unsigned int numE = me->mIndexData.mElementArray[0].mNum / 3;

    const int vstride = me->mVertexData.mStream[0].mLayout.mStride;

    for( unsigned int i=0; i<numE; i++ )
    {
        const int i0 = lSrcIndexBuffer[3 * i + 0];
        const int i1 = lSrcIndexBuffer[3 * i + 1];
        const int i2 = lSrcIndexBuffer[3 * i + 2];

        const float *v0 = (float*)((char*)me->mVertexData.mStream[0].mBuffer + vstride*i0);
        const float *v1 = (float*)((char*)me->mVertexData.mStream[0].mBuffer + vstride*i1);
        const float *v2 = (float*)((char*)me->mVertexData.mStream[0].mBuffer + vstride*i2);
#if 1
        const vec3 m = vec3(v0[0] + v1[0] + v2[0], 
                            v0[1] + v1[1] + v2[1], 
                            v0[2] + v1[2] + v2[2] )/3.0f;
        //const float d = dot(m-ro,rd);
        const float d = length( m - ro );
#else

        const float d = fminf(fminf(length(vec3(v0)-ro), 
                                    length(vec3(v1)-ro)),
                                    length(vec3(v2)-ro));
        //d = 1000.0f - v0[1];
#endif

        lTmpRadix[2*i+0] = i;
        //lTmpRadix[2*i+1] = 0x3fffff - (unsigned int)(10000.0f*d);
        lTmpRadix[2*i+1] = ((unsigned int*)(&d))[0];
    }
    
    piRadixSort4(lTmpRadix, lTmpRadix + numE*2, numE);

    for( unsigned int i=0; i<numE; i++ )
    {
        const unsigned int id = lTmpRadix[2 * (numE-1-i) + 0];
        lSortedBuffer[3 * i + 0] = lSrcIndexBuffer[3 * id + 0];
        lSortedBuffer[3 * i + 1] = lSrcIndexBuffer[3 * id + 1];
        lSortedBuffer[3 * i + 2] = lSrcIndexBuffer[3 * id + 2];
    }

    renderer->UpdateBuffer(me->mIndexData.mElementArray[0].mIBO, lSortedBuffer, numE * 3 * sizeof(unsigned int));
}*/


}