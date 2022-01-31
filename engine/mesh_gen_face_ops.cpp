// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2022 Huldra
// Copyright (c) 2005 - 2016 Inigo Quilez
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

#include <string.h>
#include "engine/vec3f.h"
#include "engine/mesh.h"
#include "engine/mesh_gen.h"
#include "engine/mesh_gen_face_ops.h"

namespace arctic {


Vec3F MeshFace_GetNormal(const Mesh *me, int faceID, float  *nlen) {
  Vec3F nor = Vec3F(0.0f, 0.0f, 0.0f);

  const MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  for(int i=0; i<num; i++) {
    const Vec3F a = *((Vec3F *)me->GetVertexData(STREAMID, face->mIndex[(i+1)%num], POSID));
    const Vec3F b = *((Vec3F *)me->GetVertexData(STREAMID, face->mIndex[ i       ], POSID));
    nor += Cross(b, a);
  }
  float l = Length(nor);
  nor /= l;
  if (nlen) {
    nlen[0] = 0.5f*l;
  }
  return nor;
}

Vec3F MeshFace_GetCenter(const Mesh *me, int faceID) {
  const MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  Vec3F center = Vec3F(0.0f, 0.0f, 0.0f); 
  for(int i=0; i<num; i++) {
    Vec3F *v = (Vec3F *)me->GetVertexData(STREAMID, face->mIndex[i], POSID);
    center += *v;
  }
  return center / (float)num;
}


Mat44F MeshFace_GetFrame(const Mesh *me, int faceID) {
  const Vec3F cen = MeshFace_GetCenter(me, faceID);

  const MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  const Vec3F *v0 = (const Vec3F *)me->GetVertexData(STREAMID, face->mIndex[0], POSID);
  const Vec3F *v1 = (const Vec3F *)me->GetVertexData(STREAMID, face->mIndex[1], POSID);
  const Vec3F *v2 = (const Vec3F *)me->GetVertexData(STREAMID, face->mIndex[num-1], POSID);

  const Vec3F uu = Normalize(*v1 - *v0);
  const Vec3F vv = Normalize(*v2 - *v0);
  const Vec3F ww = Normalize(Cross(uu,vv));

  return Mat44F(uu.x, uu.y, uu.z, -Dot(uu,cen),
      vv.x, vv.y, vv.z, -Dot(vv,cen),
      ww.x, ww.y, ww.z, -Dot(ww,cen),
      0.0f, 0.0f, 0.0f, 1.0f);
}

void MeshFace_GetFrame2(const Mesh *me, int faceID, Mat44F & ltw, Mat44F & w2l) {
  const Vec3F cen = MeshFace_GetCenter(me, faceID);

  const MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  const Vec3F *v0 = (const Vec3F *)me->GetVertexData(STREAMID, face->mIndex[0], POSID);
  const Vec3F *v1 = (const Vec3F *)me->GetVertexData(STREAMID, face->mIndex[1], POSID);
  const Vec3F *v2 = (const Vec3F *)me->GetVertexData(STREAMID, face->mIndex[(num-1)], POSID);

  const Vec3F uu = Normalize(*v1 - *v0);
  const Vec3F vv = Normalize(*v2 - *v0);
  const Vec3F ww = Normalize(Cross(uu,vv));

  w2l = Mat44F(uu.x, uu.y, uu.z, -Dot(uu,cen),
      vv.x, vv.y, vv.z, -Dot(vv,cen),
      ww.x, ww.y, ww.z, -Dot(ww,cen),
      0.0f, 0.0f, 0.0f, 1.0f);

  ltw = Mat44F(uu.x, vv.x, ww.x, cen.x,
      uu.y, vv.y, ww.y, cen.y,
      uu.z, vv.z, ww.z, cen.z,
      0.0f, 0.0f, 0.0f, 1.0f);
}

void MeshFace_Shrink(Mesh *me, int faceID, float amount) {
  const Vec3F cen = MeshFace_GetCenter(me, faceID);

  const MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  for(int i=0; i<num; i++) {
    Vec3F *v = (Vec3F *)me->GetVertexData(STREAMID, face->mIndex[i], POSID);
    *v = Mix(*v, cen, amount);
  }
}

void MeshFace_ExtrudeDir(Mesh *mesh, int faceID, const Vec3F & dir, float amount) {
  const MeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  for(int i=0; i<num; i++) {
    Vec3F *v = (Vec3F *)mesh->GetVertexData(STREAMID, face->mIndex[i], POSID);
    *v += dir*amount;
  }
}

void MeshFace_Extrude(Mesh *mesh, int faceID, float amount) {
  const Vec3F nor = MeshFace_GetNormal(mesh, faceID, nullptr);
  MeshFace_ExtrudeDir(mesh, faceID, nor, amount);
}


void MeshFace_TransformGlobal(Mesh *mesh, int faceID, const Mat44F & mat) {
  const MeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  for(int i=0; i<num; i++) {
    Vec3F *v = (Vec3F *)mesh->GetVertexData(STREAMID, face->mIndex[i], 0);

    Vec3F p = Transform(mat, Vec3F(v->x,v->y,v->z));
    v->x = p.x;
    v->y = p.y;
    v->z = p.z;
  }
}

void MeshFace_TransformLocal(Mesh *mesh, int faceID, const Mat44F & mat) {
  const Mat44F wtl = MeshFace_GetFrame(mesh, faceID);
  const Mat44F ltw = Invert(wtl);
  const Mat44F tot = ltw * mat * wtl;

  const MeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;
  for(int i=0; i<num; i++) {
    Vec3F *v = (Vec3F *)mesh->GetVertexData(STREAMID, face->mIndex[i], 0);

    const Vec3F p = Transform(tot, Vec3F((const float*)v));
    v->x = p.x;
    v->y = p.y;
    v->z = p.z;
  }
}

void MeshFace_ExtrudeTo(Mesh *mesh, int faceID, const Vec3F & xyz) {
  const MeshFace *face = mesh->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;

  const Vec3F cen = MeshFace_GetCenter((const Mesh*)mesh, faceID);
  const Vec3F nor = MeshFace_GetNormal((const Mesh*)mesh, faceID, NULL);

  Vec3F pn = xyz - cen;

  //piVec3FF_Norm(&pn);
  const float pk = -Dot(pn, xyz);
  const float ins = 1.0f / Dot(pn, pn);
  for(int i=0; i<num; i++) {
    Vec3F * v = (Vec3F *)mesh->GetVertexData(STREAMID, face->mIndex[i], 0);

    float dist = Distance(*v, cen);

    // project into the plane
    const float t = - (pk + Dot(pn,*v)) * ins;
    *v = *v + t*pn;

    Vec3F di = Normalize(*v - xyz);
    v->x = xyz[0] + di.x*dist; 
    v->y = xyz[1] + di.y*dist; 
    v->z = xyz[2] + di.z*dist; 
  }
}

}
