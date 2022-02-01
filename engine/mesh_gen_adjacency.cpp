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

#include <cstring>
#include "engine/mesh_gen_adjacency.h"

namespace arctic {

// devuelve el index de la arista que contiene a "a" y "b"
int MeshAdjacency_GetEdgeIDContainingVerts(MeshAdjacency *am, int a, int b) {
  const int ma = (a>b)?a:b;
  const int mi = (a<b)?a:b;

  for (int i=0; i<am->numaristas; i++) {
    if (am->arisaux[i].v0 == ma && am->arisaux[i].v1 == mi) {
      return i;
    }
  }
  return -1;
}

bool MeshAdjacency_Init(MeshAdjacency *me, const Mesh *mesh) {
  const int numf = mesh->mFaceData.mIndexArray[0].mNum;
  me->nmax = numf*4;
  me->numaristas = 0;
  me->arisaux = (MeshEdge*)malloc(me->nmax*sizeof(MeshEdge));
  if (!me->arisaux) {
    return 0;
  }
  memset(me->arisaux, 0, me->nmax*sizeof(MeshEdge));

  for (int i=0; i<numf; i++) {
    MeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + i;
    const int fv = 3;
    for (int j=0; j<fv; j++) {
      const int v0 = face->mIndex[j];
      const int v1 = face->mIndex[(j+1)%fv];
      const int ed = MeshAdjacency_GetEdgeIDContainingVerts(me, v0, v1);

      if (ed==-1) {
        const int id = me->numaristas++;
        me->arisaux[id].v0 = (v0>v1)?v0:v1;
        me->arisaux[id].v1 = (v0<v1)?v0:v1;
        me->arisaux[id].va = 1;
      } else {
        me->arisaux[ed].va = 2;
      }
    }
  }

  //----------------
  const int num = mesh->mVertexData.mVertexArray[0].mNum;
  me->mVertexValence = (char*)malloc(num);
  if (!me->mVertexValence) {
    return false;
  }
  memset(me->mVertexValence, 0, num);

  return true;
}

void MeshAdjacency_DeInit(MeshAdjacency *me) {
  free(me->mVertexValence);
  free(me->arisaux);
}

int MeshAdjacency_GetNumEdges(MeshAdjacency *me) {
  return me->numaristas;
}


}
