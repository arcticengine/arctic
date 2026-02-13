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

#define _USE_MATH_DEFINES
#include <cmath>
#include "engine/mesh.h"
#include "engine/mesh_gen.h"

namespace arctic {

bool Mesh_GenerateTorus(Mesh *me, const MeshVertexFormat *vertexFormat, int gy, int gx, float ry, float rx) {
  const int numVerts = (gx + 1) * (gy + 1);
  const int numFaces = 2 * gx * gy;
  if (!me->Init(1, numVerts, vertexFormat, kRMVEDT_Polys, 1, numFaces)) {
    return false;
  }

  int k = 0;
  for (int j = 0; j <= gy; j++) {
    for (int i = 0; i <= gx; i++) {
      const float x = -1.0f + 2.0f*(float)(i) / (float)gx;
      const float y = -1.0f + 2.0f*(float)(j) / (float)gy;

      const float a = 6.2831f*x;
      const float b = 6.2831f*y;

      float ww = ry + rx*sinf(a);

      float *v = (float*)me->GetVertexData(STREAMID, k++, POSID);
      v[0] = rx*cosf(a);
      v[1] = ww*sinf(b);
      v[2] = ww*cosf(b);
    }
  }
  MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer;
  for (int j = 0; j<gy; j++) {
    for (int i = 0; i<gx; i++) {
      face->mIndex[0] = (gx + 1)*((j + 1) % gy) + ((i + 0) % gx);
      face->mIndex[1] = (gx + 1)*((j + 1) % gy) + ((i + 1) % gx);
      face->mIndex[2] = (gx + 1)*((j + 0) % gy) + ((i + 1) % gx);
      face++;
      face->mIndex[0] = (gx + 1)*((j + 0) % gy) + ((i + 1) % gx);
      face->mIndex[1] = (gx + 1)*((j + 0) % gy) + ((i + 0) % gx);
      face->mIndex[2] = (gx + 1)*((j + 1) % gy) + ((i + 0) % gx);
      face++;
    }
  }
  me->mVertexData.mVertexArray[0].mNum = numVerts;
  me->mFaceData.mIndexArray[0].mNum = numFaces;
  return true;
}

bool Mesh_GeneratePlane(Mesh *me, const MeshVertexFormat *vertexFormat, int gx, int gy) {
  const int numVerts = (gx + 1) * (gy + 1);
  const int numFaces = 2 * gx * gy;
  if (!me->Init(1, numVerts, vertexFormat, kRMVEDT_Polys, 1, numFaces)) {
    return false;
  }

  int k = 0;
  for (int j=0; j<=gy; j++) {
    for (int i=0; i<=gx; i++) {
      const float x = -1.0f + 2.0f*(float)(i)/(float)gx;
      const float y = -1.0f + 2.0f*(float)(j)/(float)gy;

      float *v = (float*)me->GetVertexData(STREAMID, k++, POSID);
      v[0] = x;
      v[1] = 0.0f;
      v[2] = y;
    }
  }

  MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer;
  for (int j=0; j<gy; j++) {
    for (int i=0; i<gx; i++) {
#if 0
      face->mIndex[0] = (gx+1)*(j+0) + (i+0);
      face->mIndex[1] = (gx+1)*(j+1) + (i+0);
      face->mIndex[2] = (gx+1)*(j+1) + (i+1);
      face->mIndex[3] = (gx+1)*(j+0) + (i+1);
      face++;
#else
      face->mIndex[0] = (gx+1)*(j+1) + (i+0);
      face->mIndex[1] = (gx+1)*(j+1) + (i+1);
      face->mIndex[2] = (gx+1)*(j+0) + (i+1);
      face++;
      face->mIndex[0] = (gx+1)*(j+0) + (i+1);
      face->mIndex[1] = (gx+1)*(j+0) + (i+0);
      face->mIndex[2] = (gx+1)*(j+1) + (i+0);
      face++;
#endif
    }
  }
  me->mVertexData.mVertexArray[0].mNum = numVerts;
  me->mFaceData.mIndexArray[0].mNum = numFaces;
  return true;
}

//     2------ 3
//    /|      /|
//   6-------7 |
//   | |     | |
//   | 0-----|-1
//   |/      |/
//   4-------5

static char cvd[] = { 
  0, 1, 5, 4,
  2, 6, 7, 3,
  1, 3, 7, 5,
  0, 4, 6, 2, 
  4, 5, 7, 6,
  0, 2, 3, 1 };

bool Mesh_GenerateCube(Mesh *me, const MeshVertexFormat *vertexFormat) {
  const int numVerts = 8;
  const int numFaces = 12;
  if (!me->Init(1, numVerts, vertexFormat, kRMVEDT_Polys, 1, numFaces)) {
    return false;
  }

  for (int i=0; i<8; i++) {
    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);
    v[0] = -1.0f+2.0f*((i>>0)&1);
    v[1] = -1.0f+2.0f*((i>>1)&1);
    v[2] = -1.0f+2.0f*((i>>2)&1);
  }

  MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer;
  for (int i=0; i<6; i++) {
    face->mIndex[0] = cvd[4*i+0];
    face->mIndex[1] = cvd[4*i+1];
    face->mIndex[2] = cvd[4*i+2];
    face++;
    face->mIndex[0] = cvd[4*i+2];
    face->mIndex[1] = cvd[4*i+3];
    face->mIndex[2] = cvd[4*i+0];
    face++;
  }
  me->mVertexData.mVertexArray[0].mNum = numVerts;
  me->mFaceData.mIndexArray[0].mNum = numFaces;
  return true;
}


bool Mesh_GenerateCubePlanes(Mesh *me, const MeshVertexFormat *vertexFormat) {
  const int numVerts = 24;
  const int numFaces = 12;
  if (!me->Init(1, numVerts, vertexFormat, kRMVEDT_Polys, 1, numFaces)) {
    return false;
  }

  for (int i = 0; i<24; i++) {
    int f = i >> 2;
    int j = i & 3;

    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);

    if (f==0) {
      v[0] = -1.0f;
      v[2] = -1.0f + 2.0f*((j >> 0) & 1);
      v[1] = -1.0f + 2.0f*((j >> 1) & 1);
    } else if (f == 1) {
      v[0] = 1.0f;
      v[1] = -1.0f + 2.0f*((j >> 0) & 1);
      v[2] = -1.0f + 2.0f*((j >> 1) & 1);
    } else if (f == 2) {
      v[0] = -1.0f + 2.0f*((j >> 0) & 1);
      v[1] = -1.0f;
      v[2] = -1.0f + 2.0f*((j >> 1) & 1);
    } else if (f == 3) {
      v[2] = -1.0f + 2.0f*((j >> 0) & 1);
      v[1] =  1.0f;
      v[0] = -1.0f + 2.0f*((j >> 1) & 1);
    } else if (f == 4) {
      v[1] = -1.0f + 2.0f*((j >> 0) & 1);
      v[0] = -1.0f + 2.0f*((j >> 1) & 1);
      v[2] = -1.0f;
    }
    else { v[0] = -1.0f + 2.0f*((j >> 0) & 1);
      v[1] = -1.0f + 2.0f*((j >> 1) & 1);
      v[2] = 1.0f;
    }
  }


  MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer;
  for (int i = 0; i<6; i++) {
    face->mIndex[0] = 4*i + 0;
    face->mIndex[1] = 4*i + 1;
    face->mIndex[2] = 4*i + 3;
    face++;
    face->mIndex[0] = 4*i + 3;
    face->mIndex[1] = 4*i + 2;
    face->mIndex[2] = 4*i + 0;
    face++;
  }

  me->mVertexData.mVertexArray[0].mNum = numVerts;
  me->mFaceData.mIndexArray[0].mNum = numFaces;
  return true;
}

bool Mesh_PatchedSphere(Mesh *me, const MeshVertexFormat *vertexFormat, int n) {
  const int nv = (n-1)*(n-1)*6 + (n-1)*12 + 1*8;
  const int nf = n*n*6;
  if (!me->Init(1, nv, vertexFormat, kRMVEDT_Polys, 1, nf)) {
    return false;
  }

  for (int i=0; i<6; i++) {

  }
  return true;
}

bool Mesh_GenerateRevolution(Mesh *me, const MeshVertexFormat *vertexFormat) {
  /*
     float           x, y, a;
     QVERT            *v;
     QUAD            *q;
     int				notapa;
     signed char *ptr;


     notapa = bp[1];//(int)*(ptr++);

     int nv = bp[0];//(int)*(ptr++);
     int nu = (int)*(ptr++);

     if (!qmesh_alloc(m, 2+(nu-2)*nv, nv*(nu-1)))
     return 0;
     if (!Mesh_Init(me, 8, stride, nc, layout, 6))
     return false;


     v = m->verts;

     v->vert.x = (*ptr++)*(1.0f/127.0f);
     v->vert.y = (*ptr++)*(1.0f/127.0f);
  //v->t[0].y = 1.0f;
  v++;
  for (int j=1; j<(nu-1); j++)
  {
  x = (*ptr++)*(1.0f/127.0f);
  y = (*ptr++)*(1.0f/127.0f);
  for (int i=0; i<nv; i++)
  {
  v->t.x = (float)i/(float)nv;
  v->t.y = (float)j/(float)nu;
  a = 6.2831f*v->t.x;
  v->vert.x = x*sinf(a);
  v->vert.y = y;
  v->vert.z = x*cosf(a);
  v++;
  }
  }

  v->vert.x = (*ptr++)*(1.0f/127.0f);
  v->vert.y = (*ptr++)*(1.0f/127.0f);
  v->t.y = 1.0f;

  q = m->quads;
  for (int i=0; i<nv; i++)
  {
  q->n = 3;
  q->v[0] = 0;
  q->v[1] = 1+i;
  q->v[2] = 1+((i+1)%nv);
  q++;
  }

  for (int j=0; j<(nu-3); j++)
  for (int i=0; i<nv; i++)
  {
  q->n = 4;
  q->v[0] = 1 + ((j))*nv +   i;
  q->v[1] = 1 + ((j+1))*nv +   i;
  q->v[2] = 1 + ((j+1))*nv + ((i+1)%nv);
  q->v[3] = 1 + ((j))*nv + ((i+1)%nv);
  q++;
  }

  for (int i=0; i<nv; i++)
  {
  q->n = 3;
  q->v[0] = m->nv-1;
  q->v[1] = m->nv-2 - i;
  q->v[2] = m->nv-2 - ((i+1)%nv);
  q++;
}

*/
return true;
}


}
