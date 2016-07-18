#pragma once

// falta que se pueda hacer LOD con numero de polys pintados
// falta que se pueda hacer sortable IA...

#include "../../libRender/piRenderer.h"
#include "../../libMesh/piMesh.h"

namespace piLibs {


#define IQRENDERMESH_MAXELEMS    8
#define IQRENDERMESH_MAXSTREAMS  8
#define IQRENDERMESH_MAXELEMENTARRAYS 48

typedef struct
{
    void                       *mBuffer;
    piRArrayLayout              mLayout;
    piBuffer                    mVBO;
    unsigned int	            mLength;
}piRenderMeshVertexStream;

typedef struct
{
    unsigned int	          mNumStreams;
    piRenderMeshVertexStream  mStream[IQRENDERMESH_MAXSTREAMS];
}piRenderMeshVertexData;

typedef struct
{
//    unsigned int *mTmpRadix;
//    unsigned int *mSortedBuffer;

    unsigned int    mNum;
    unsigned int   *mBuffer;
    piBuffer        mIBO;
    piVertexArray   mVAO;
}piRenderMeshElementArray;

typedef struct
{
    piPrimitiveType mType;
    unsigned int    mNumElementArrays;
    piRenderMeshElementArray mElementArray[IQRENDERMESH_MAXELEMENTARRAYS];
    piVertexArray   mVAO;       // in case there are no index arrays (point cloud)
}piRenderMeshIndexData;

class piRenderMesh
{
public:
    piRenderMesh();
    ~piRenderMesh();

    bool InitFromMesh( piRenderer *renderer, const piMesh *mesh, piPrimitiveType patchNum );

    void End( piRenderer *renderer );

    void Render( piRenderer *renderer, int elementArrayID, int numInstances ) const;

    const bound3 & GetBBox( void ) const;

    const int GetNumVertices( void ) const;
    const piBuffer GetVertexBuffer( int stream ) const;

private:
    bound3                  mBBox;
    piRenderMeshVertexData  mVertexData;
    piRenderMeshIndexData   mIndexData;
};


} // namespace piLibs