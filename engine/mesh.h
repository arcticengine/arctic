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

#include "engine/bound3f.h"

namespace arctic {

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
};

struct MeshVertexFormat {
  int mStride = 0;    // in bytes
  int mNumElems = 0;
  int mDivisor = 0;
  MeshVertexElemInfo mElems[Mesh_MAXELEMS];
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

    bool Init(int numVertexStreams, int nv, const MeshVertexFormat *vf,
              MeshType type,
              int numElementsArrays, int numElements);
    void DeInit();
    void ClearGeometry();
    bool Expand(int nv, int nf);
    bool Clone(Mesh *dst);

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

  public:
    Bound3F mBBox;
    MeshVertexData mVertexData;
    MeshFaceData mFaceData;
};



} // namespace piLibs
