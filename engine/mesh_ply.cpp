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
#include <stdlib.h>
#include <stdio.h>
#include "engine/mesh_ply.h"

namespace arctic {

static int separa_palabras(char *str, char *ptrs[], const char *del, int maxpalabras) {
  if (str[0]==';') return 0;
  if (str[0]==0) return 0;
  if (str[0]==13) return 0;
  if (str[0]==10) return 0;

  int i = 0;
  char *p = strtok(str, del);
  while (p && i<maxpalabras) {
    ptrs[i++] = p;
    p = strtok(0, del);
  }
  return i;
}

static int readline(char *str, int max, FILE *fp) {
  if (!fgets(str, max, fp)) {
    return 0;
  }

  size_t l = strlen(str);
  while (l > 0 && (str[l - 1] == 10 || str[l - 1] == 13)) {
    str[l - 1] = 0;
    l--;
  }

  return 1;
}


int MeshPly_Read(Mesh *me, const char *name, bool calcNormals) {
  int n, nv, nf;
  char str[256];
  char *ptrs[16];

  FILE *fp = fopen(name, "rt");
  if (!fp) {
    return 0;
  }

  readline(str, 255, fp); if (strcmp(str,"ply")) { fclose(fp); return 0; }
  fgets(str,255,fp);
  fgets(str,255,fp);

  readline(str,255,fp); 
  n = separa_palabras(str, ptrs, " \t", 16);
  if (strcmp(ptrs[0], "element")) { fclose(fp); return 0; }
  if (strcmp(ptrs[1], "vertex")) { fclose(fp); return 0; }
  nv = atoi(ptrs[2]);

  do {
    readline(str,255,fp); 
    n = separa_palabras(str, ptrs, " \t", 16);
  } while (strcmp(ptrs[0],"property")==0);

  n = separa_palabras(str, ptrs, " \t", 16);
  if (strcmp(ptrs[0], "element")) { fclose(fp); return 0; }
  if (strcmp(ptrs[1], "face")) { fclose(fp); return 0; }
  nf = atoi(ptrs[2]);

  do {
    readline(str,255,fp); 
    n = separa_palabras(str, ptrs, " \t", 16);
  } while (strcmp(ptrs[0],"property")==0);

  if (strcmp(str,"end_header")) {
    fclose(fp);
    return 0;
  }

  const MeshVertexFormat vf = {6*sizeof(float), 2, 0,
    {{3, kRMVEDT_Float, false}, {3, kRMVEDT_Float, false}}};
  if (!me->Init(1, nv, &vf, kRMVEDT_Polys, 1, nf)) {
    fclose(fp);
    return 0;
  }

  for (int i=0; i<nv; i++) {
    readline(str,255,fp);

    float *v  = (float*)me->GetVertexData(0, i, 0);
    float *nt = (float*)me->GetVertexData(0, i, 1);

    n = separa_palabras(str, ptrs, " \t", 16);

    v[0] = (float)atof(ptrs[0]);
    v[1] = (float)atof(ptrs[1]);
    v[2] = (float)atof(ptrs[2]);

    nt[0] = (float)atof(ptrs[3]);
    nt[1] = (float)atof(ptrs[4]);
    nt[2] = (float)atof(ptrs[5]);
  }

  //-------------------

  for (int i=0; i<nf; i++) {
    readline(str,255,fp);

    n = separa_palabras(str, ptrs, " \t", 16);

    me->mFaceData.mIndexArray[0].mBuffer[i].mIndex[0] = atoi(ptrs[1]);
    me->mFaceData.mIndexArray[0].mBuffer[i].mIndex[1] = atoi(ptrs[2]);
    me->mFaceData.mIndexArray[0].mBuffer[i].mIndex[2] = atoi(ptrs[3]);
  }

  fclose(fp);


  //-------------------
  /*
     DMESH_CalcNormals(me, 0, 3*sizeof(float), 0,
     1, 5*sizeof(float), 0, 
     20.0f);
     */
  return 1;
}


}
