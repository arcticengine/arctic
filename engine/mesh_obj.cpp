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
//#define USETRICAT

#include <cstring>
#include <deque>
#include <fstream>
#include <vector>
#include "engine/mesh.h"
#include "engine/vec3f.h"

namespace arctic {

static int splitInWords(char *str, char **ptrs, const char *sep, int num) {
  int i;
  for (i=0; i<num; i++) {
    ptrs[i] = strtok(i==0?str:0, sep);
    if (ptrs[i]==0) {
      break;
    }
  }
  return i;
}

int MeshObj_Read(Mesh *me, const char *name, bool calcNormals) {
  char str[512];
  char *ptrs[128];
  std::vector<Vec3F> vertices;
  std::vector<MeshFace> faces;

  std::ifstream ifile(name);
  if (!ifile.is_open()) {
    return 0;
  }
  std::deque<std::string> original_file;
  std::string line;
  while (ifile.good()) {
    getline(ifile, line);
    if (!line.empty()) {
      original_file.push_back(line);
    }
  }
  ifile.close();

  vertices.reserve(1024 * 1024);
  faces.reserve(1024 * 1024);

  // get all the vertices
  for (size_t i = 0; i < original_file.size(); ++i) {
    memcpy(str, original_file[i].c_str(), std::min((size_t)512, original_file[i].size() + 1));
    str[511] = 0;
    int n = splitInWords(str, ptrs, ", \t", 128);
    if (n>0) {
      if (ptrs[0][0]=='v' && ptrs[0][1]=='\0') {
        Vec3F vert;
        vert.x = (float)atof(ptrs[1]);
        vert.y = (float)atof(ptrs[2]);
        vert.z = (float)atof(ptrs[3]);
        vertices.push_back(vert);
      }            
    }
  }

  // get all the faces

  //int group    = -1;
  //int newgrouprequest = 0;
  //int matid = -1;
  for (size_t i = 0; i < original_file.size(); ++i) {
    MeshFace face;
    memcpy(str, original_file[i].c_str(), std::min((size_t)512, original_file[i].size() + 1));
    str[511] = 0;
    int n = splitInWords(str, ptrs, ", \t", 128);
    if (n>0) {
      if (ptrs[0][0]=='v' && ptrs[0][1]=='\0') {
      // } else if (ptrs[0][0]=='g' && ptrs[0][1]=='\0')
      // {
      //    newgrouprequest=1;
      //}
      //else if (strcmp(ptrs[0],"usemtl")==0)
      // {
      //     newgrouprequest=1;
      // }
      } else if (ptrs[0][0]=='f' && ptrs[0][1]=='\0' && n>=(3+1)) {
        if (n==(1+3)) {
          face.mIndex[0] = atoi(ptrs[1])-1;
          face.mIndex[1] = atoi(ptrs[2])-1;
          face.mIndex[2] = atoi(ptrs[3])-1;
          //face.mat = matid;
          faces.push_back(face);
        } else if (n==(1+4)) {
          face.mIndex[0] = atoi(ptrs[1])-1;
          face.mIndex[1] = atoi(ptrs[2])-1;
          face.mIndex[2] = atoi(ptrs[3])-1;


          faces.push_back(face);
          face.mIndex[0] = atoi(ptrs[3])-1;
          face.mIndex[1] = atoi(ptrs[4])-1;
          face.mIndex[2] = atoi(ptrs[1])-1;
          //face.mat = matid;
          faces.push_back(face);

        } else {
          // TODO: tesselate N-polygons into triangle!
          const int num = n-1;
          if (num<32) {	
            unsigned int indices[32];
            for (int i=0; i<num; i++) {
              indices[i] = atoi(ptrs[1+i])-1;
            }

            for (int i=0; i<num-2; i++) {
              face.mIndex[0] = indices[0];
              face.mIndex[1] = indices[i+1];
              face.mIndex[2] = indices[i+2];
              //face.mat = matid;
              faces.push_back(face);
            }
          }
        }
      }     
    }
  }


  if (calcNormals) {
    const MeshVertexFormat vf = { 6*sizeof(float), 2, 0, {{3, kRMVEDT_Float, false},
      {3, kRMVEDT_Float, false} } };
    if (!me->Init(1, vertices.size(), &vf, kRMVEDT_Polys, 1, faces.size())) {
      return 0;
    }
  } else {
    const MeshVertexFormat vf = { 3*sizeof(float), 1, 0, { {3, kRMVEDT_Float, false} } };
    if (!me->Init(1, vertices.size(), &vf, kRMVEDT_Polys, 1, faces.size())) {
      return 0;
    }
  }

  // copy vertices
  //memcpy(me->mVertexData.mBuffer, vertices.mBuffer, vertices.mNum*sizeof(Vec3F));
  float *vptr = (float*)&vertices[0];
  for (unsigned int i = 0; i<vertices.size(); i++) {
    float *v = (float*)me->GetVertexData(0, i, 0);
    v[0] = vptr[0];
    v[1] = vptr[1];
    v[2] = vptr[2];
    vptr += 3;
  }
  // copy faces
  memcpy(me->mFaceData.mIndexArray[0].mBuffer, &faces[0], faces.size()*sizeof(MeshFace));

  if (calcNormals) {
    me->Normalize(0, 0, 1);
  }
  return 1;
}

}

