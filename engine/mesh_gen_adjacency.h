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

#include "engine/mesh.h"

namespace arctic {


typedef struct {
  int    v0;
  int    v1;
  int    va;  // valence: 1 or 2
}MeshEdge;

typedef struct {
  int         nmax;
  int         numaristas;
  MeshEdge  *arisaux;
  //-----------------
  char        *mVertexValence;

}MeshAdjacency;

bool MeshAdjacency_Init(MeshAdjacency *me, const Mesh *mesh);
void MeshAdjacency_DeInit(MeshAdjacency *me);

int MeshAdjacency_GetNumEdges(MeshAdjacency *am);
// devuelve el index de la arista que contiene a "a" y "b"
int MeshAdjacency_GetEdgeIDContainingVerts(MeshAdjacency *am, int a, int b);



} // namespace piLibs
