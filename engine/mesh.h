// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2022 Huldra
// Copyright (c) 2015 - 2016 Inigo Quilez
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#pragma once

#include "engine/arctic_types.h"
#include "engine/bound3f.h"

namespace arctic {

class GlBuffer;
class GlProgram;

#define Mesh_MAXELEMS    8
#define Mesh_MAXINDEXARRAYS 48
#define Mesh_MAXVERTEXARRAYS 8

enum MeshVertexElemDataType {
  kRMVEDT_UByte = 0,
  kRMVEDT_Float = 1,
  kRMVEDT_Int = 2,
  kRMVEDT_Double = 3,
};

enum MeshType {
  kRMVEDT_Polys = 0,
  kRMVEDT_Points = 1
};

struct MeshVertexElemInfo {
  unsigned int mNumComponents = 0;
  MeshVertexElemDataType mType = kRMVEDT_UByte;
  bool mNormalize = false;
  unsigned int mOffset = 0;
  // The vertex shader attribute this element feeds, used by Mesh::Draw. The
  // string is not copied, so it has to outlive the mesh: a literal will do.
  const char *mName = nullptr;

  // Calculate the size of an element in bytes
  unsigned int GetElementSize() const;
};

struct MeshVertexFormat {
  int mStride = 0;    // in bytes
  int mNumElems = 0;
  int mDivisor = 0;
  MeshVertexElemInfo mElems[Mesh_MAXELEMS];

  // Adds a new element to the vertex format
  // Returns the index of the added element or -1 if the format is full
  int AddElement(unsigned int numComponents, MeshVertexElemDataType type, bool normalize = false);

  // Adds a new element named after the vertex shader attribute it feeds, so
  // that Mesh::Draw can bind it by name
  // Returns the index of the added element or -1 if the format is full
  int AddElement(const char *name, unsigned int numComponents,
      MeshVertexElemDataType type, bool normalize = false);
};

struct MeshVertexArray {
  unsigned int mMax = 0;
  unsigned int mNum = 0;
  void *mBuffer = nullptr;
  MeshVertexFormat mFormat;
};

struct MeshVertexData {
  int mNumVertexArrays = 0;
  MeshVertexArray mVertexArray[Mesh_MAXVERTEXARRAYS];
};

#pragma pack(1)
struct MeshFace {
  int mIndex[3];  // 3
};
#pragma pack()

struct MeshIndexArray {
  unsigned int mMax = 0;
  unsigned int mNum = 0;
  MeshFace *mBuffer = nullptr;
};

struct MeshFaceData {
  MeshType mType = kRMVEDT_Polys; // 0=polys, 1=points
  int mNumIndexArrays = 0;
  MeshIndexArray mIndexArray[Mesh_MAXINDEXARRAYS];
};

class Mesh {
  public:
    Mesh();
    ~Mesh();

    // Allocates the mesh. The capacity is fixed here: AddVertex and AddFace
    // never grow it, they refuse to write past nv/numElements. Call Expand to
    // make room for more.
    bool Init(int numVertexStreams, int nv, const MeshVertexFormat *vf,
              MeshType type,
              int numElementsArrays, int numElements);
    void DeInit();
    void ClearGeometry();
    // Makes room for nv more vertices in every vertex stream and nf more faces
    // in every index array. Invalidates every pointer previously returned by
    // GetVertexData, and mBuffer pointers as well.
    bool Expand(int nv, int nf);
    bool Clone(Mesh *dst);

    // The returned pointer is valid until the next Expand or DeInit call.
    void *GetVertexData(int streamID, int vertexID, int elementID) const;
    bool AddVertexStream(const int nv, const MeshVertexFormat *vf);
    void Normalize(int stream, int pPos, int npos);
    void CalcBBox(int stream, int pPos);
    //void RandomizeVerts();

    bool Load(const char *name);
    int Save(const char *name);
    int Compact();

    // Dynamic Construction
    int  GetVertexSize(int streamID);
    void GetVertex(int streamID, int vertexID, void *data);
    void SetVertex(int streamID, int vertexID, void *data);
    bool SetTriangle(int streamID, int triangleID, int a, int b, int c);

    // New methods for direct vertex and face manipulation.
    // Both return -1 and write a message to the log when the capacity given to
    // Init is exhausted, so an underestimated Init is visible instead of
    // silently losing geometry.
    int AddVertex(int streamID, ...);
    int AddFace(int streamID, int v1, int v2, int v3);
    int GetCurrentVertexCount(int streamID) const;
    int GetCurrentFaceCount(int streamID) const;

    // Draws the mesh with the given program. Every vertex element that has a
    // name goes to the shader attribute of that name, so the caller needs
    // neither a vertex buffer of its own nor a single glVertexAttribPointer
    // call. An element whose name the shader does not declare is skipped. A
    // format without any names falls back to binding element i to slot i.
    // The vertex and index buffers of the GPU are created here on the first
    // call and refreshed whenever the geometry changes.
    void Draw(GlProgram &program, int streamID = 0, int indexArrayID = 0);

    // Tells Draw that the geometry has changed and has to be sent to the GPU
    // again. The methods of this class do it themselves; call it after writing
    // to mVertexData or mFaceData buffers directly.
    void InvalidateGpuGeometry();

  private:
    bool GrowVertexArray(int streamID, int extra);
    bool GrowIndexArray(int arrayID, int extra);
    void ReleaseGpuBuffers();

  public:
    Bound3F mBBox;
    MeshVertexData mVertexData;
    MeshFaceData mFaceData;

  private:
    // Plain scalars and pointers only, as Init memsets the whole object.
    GlBuffer *mGpuVertexBuffer = nullptr;
    GlBuffer *mGpuIndexBuffer = nullptr;
    Ui64 mGeometryRevision = 0;
    Ui64 mGpuRevision = 0;
    int mGpuStreamID = -1;
    int mGpuIndexArrayID = -1;
};



} // namespace arctic
