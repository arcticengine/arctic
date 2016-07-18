#pragma once

#include "../libMath/piVecTypes.h"

namespace piLibs {


#define piMesh_MAXELEMS    8
#define piMesh_MAXINDEXARRAYS 48
#define piMesh_MAXVERTEXARRAYS 8


typedef enum
{
    piRMVEDT_UByte    = 0,
    piRMVEDT_Float    = 1,
    piRMVEDT_Int      = 2,
    piRMVEDT_Double   = 3,
}piMeshVertexElemDataType;

typedef enum
{
    piRMVEDT_Polys    = 0,
    piRMVEDT_Points   = 1
}piMeshType;

typedef struct
{
    unsigned int              mNumComponents;
	piMeshVertexElemDataType  mType;
	bool                      mNormalize;
    unsigned int              mOffset;
}piMeshVertexElemInfo;

typedef struct
{
    int                       mStride;    // in bytes
    int                       mNumElems;
    int                       mDivisor;
    piMeshVertexElemInfo      mElems[piMesh_MAXELEMS];
}piMeshVertexFormat;


typedef struct
{
    unsigned int	          mMax;
    unsigned int	          mNum;
    void                     *mBuffer;
    piMeshVertexFormat        mFormat;
}piMeshVertexArray;

typedef struct
{
    int               mNumVertexArrays;
    piMeshVertexArray mVertexArray[piMesh_MAXVERTEXARRAYS];
}piMeshVertexData;

typedef struct
{
    int mNum;
    int mIndex[4];  // 3 or 4...
}piMeshFace;

typedef struct
{
    unsigned int  mMax;
    unsigned int  mNum;
    piMeshFace   *mBuffer;
}piMeshIndexArray;

typedef struct
{
    piMeshType       mType; // 0=polys, 1=points
    int              mNumIndexArrays;
    piMeshIndexArray mIndexArray[piMesh_MAXINDEXARRAYS];
}piMeshFaceData;

class piMesh
{
public:
    piMesh();
    ~piMesh();

    bool Init( int numVertexStreams, int nv, const piMeshVertexFormat *vf, piMeshType type, int numElementsArrays, int numElements );
    void DeInit( void );
    bool Expand( int nv, int nf );
    bool Clone( piMesh *dst );

    void *GetVertexData( int streamID, int vertexID, int elementID ) const;
    bool AddVertexStream( const int nv, const piMeshVertexFormat *vf );
    void Normalize( int stream, int pPos, int npos );
    void CalcBBox( int stream, int pPos );
    //void RandomizeVerts( void );

    bool Load( const wchar_t *name );
    int Save( const wchar_t *name );
    int Compact( void );

    //-----------------------
    // Dynamic Construction
    int  GetVertexSize(int streamID);
    void GetVertex(int streamID, int vertexID, void *data);
    void SetVertex(int streamID, int vertexID, void *data);
    bool SetTriangle(int streamID, int triangleID, int a, int b, int c);

public:
    bound3           mBBox;
    piMeshVertexData mVertexData;
    piMeshFaceData   mFaceData;
};



} // namespace piLibs