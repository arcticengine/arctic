// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2022 Huldra
// Copyright (c) 2003 - 2016 Inigo Quilez
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
#include <cstring>
#include "engine/mesh_gen.h"
#include "engine/mesh_gen_mod_simple.h"

namespace arctic {


void Mesh_Scale(Mesh *me, const Vec3F & xyz) {
  const int num = me->mVertexData.mVertexArray[STREAMID].mNum;
  for (int i=0; i<num; i++) {
    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);
    v[0] *= xyz.x;
    v[1] *= xyz.y;
    v[2] *= xyz.z;
  }
}

void Mesh_Translate(Mesh *me, const Vec3F & xyz) {
  const int num = me->mVertexData.mVertexArray[STREAMID].mNum;
  for (int i=0; i<num; i++) {
    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);
    v[0] += xyz.x;
    v[1] += xyz.y;
    v[2] += xyz.z;
  }
}

void Mesh_Sit(Mesh *me) {
  me->CalcBBox(STREAMID, POSID);
  const int num = me->mVertexData.mVertexArray[STREAMID].mNum;
  for (int i=0; i<num; i++) {
    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);
    v[1] -= me->mBBox[1];
  }
}

void Mesh_Tap(Mesh *me, float factor) {
  me->CalcBBox(STREAMID, POSID);
  const int num = me->mVertexData.mVertexArray[STREAMID].mNum;
  for (int i=0; i<num; i++) {
    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);
    const float f = (v[1]-me->mBBox[1])/(me->mBBox[4]-me->mBBox[1]);
    const float s = 1.0f*(1.0f-f) + f*factor;
    v[0] *= s;        
    v[2] *= s;        
  }
}

void Mesh_Orientate(Mesh *me, const float *au, const float *av, const float *aw) {
  const int num = me->mVertexData.mVertexArray[STREAMID].mNum;
  for (int i=0; i<num; i++) {
    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);
#if 0
    float t[3] = { au[0]*v[0] + au[1]*v[1] + au[2]*v[2],
      av[0]*v[0] + av[1]*v[1] + av[2]*v[2],
      aw[0]*v[0] + aw[1]*v[1] + aw[2]*v[2] };
#else
    float t[3] = { au[0]*v[0] + av[0]*v[1] + aw[0]*v[2],
      au[1]*v[0] + av[1]*v[1] + aw[1]*v[2],
      au[2]*v[0] + av[2]*v[1] + aw[2]*v[2] };
#endif
    v[0] = t[0];
    v[1] = t[1];
    v[2] = t[2];
  }
}

void Mesh_SwapYZ(Mesh *me) {
  const int num = me->mVertexData.mVertexArray[STREAMID].mNum;
  for (int i = 0; i<num; i++) {
    float *v = (float*)me->GetVertexData(STREAMID, i, POSID);
    float tmp = v[1];
    v[1] = v[2];
    v[2] = -tmp;
    if (me->mVertexData.mVertexArray[STREAMID].mFormat.mNumElems==2)
    {
      v = (float*)me->GetVertexData(STREAMID, i, NORID);
      tmp = v[1];
      v[1] = v[2];
      v[2] = -tmp;
    }
  }
}

}

#if 0


// (axis),(angle) = (bp0, bp1, bp2), (fp3)
int mod_rotate(QMESH *m, float *fp, unsigned char *bp, intptr *vp)
{
  int     i;
  QVERT    *v;
  VEC3D   p;
  float   mat[16];
  VEC4D   q;

  ExpandAAQuat(&q, bp);

  AxisAng2Quat(&q);
  Quat2Matrix(mat, &q);


  for (v=m->verts, i=0; i<m->nv; i++, v++)
  {
    p = v->vert;
    v->vert.x = VecDot(&p, (VEC3D*)(mat+0));
    v->vert.y = VecDot(&p, (VEC3D*)(mat+4));
    v->vert.z = VecDot(&p, (VEC3D*)(mat+8));
  }

  return 1;
}


// (params) = (fp0..fp13)
int mod_texturize(QMESH *m, float *fp, unsigned char *bp, intptr *vp)
{
  int     i;
  QVERT    *v;

  //qmesh_normalize(m);
  qmesh_calccenter(m);


  v = m->verts;
  for (i=0; i<m->nv; i++, v++)
  {
    // planar
    if (bp[0]==0)
    {
      v->t.x = fp[0]*v->vert.x + fp[1]*v->vert.y + fp[2]*v->vert.z + fp[3];
      v->t.y = fp[4]*v->vert.x + fp[5]*v->vert.y + fp[6]*v->vert.z + fp[7];

      //fp[ 5]*(.5f+.5f*v->norm.x) + 
      //fp[ 6]*(.5f+.5f*v->norm.y) + 
      //fp[ 7]*(.5f+.5f*v->norm.z) + 

      //fp[10]*(.5f+.5f*v->norm.x) + 
      //fp[11]*(.5f+.5f*v->norm.y) +
      //fp[12]*(.5f+.5f*v->norm.z) + 
    }
    // cylindrical
    else
    {
      v->t.y = v->vert.y*fp[0] + fp[1];

      // TODO: Use other parameters!!!!!!!!!
      v->t.x = .5f - (.5f/3.1415927f)*atan2f(fp[2] + v->vert.x-m->center.x, 
          fp[3] + v->vert.z-m->center.z);
    }

  }


  return 1;
}


// (func) = (vp[0])
int mod_vertex_retouch(QMESH *m, float *fp, unsigned char *bp, intptr *vp)
{
  int     i;
  QVERT    *v = m->verts;
  D_FUNC func = (D_FUNC)(vp[0]);

  qmesh_normalize(m);

  for (i=0; i<m->nv; i++, v++)
    func(v, bp[0]);

  qmesh_normalize(m);

  return 1;
}




static void cfuncCircular(QVERT *vert, int num,        int clone, int nclones)
{


}

static void cfuncLinear(QVERT *vert, int num,        int clone, int nclones, VEC3D *dir, float step)
{
  int i;

  for (i=0; i<num; i++)
  {
    VecSinc(&vert->vert, step*(float)clone, dir);
    vert++;
  }
}

// (num), (func) = (bp[0]), (vp[0])
int mod_clone(QMESH *m, float *fp, unsigned char *bp, intptr *vparams)
{
  QMESH	aux;
  int	    i, j, k, off;
  int     flip;
  QUAD    *dst;
  QUAD    *ori;
  VEC3D   dir;
  int     type = bp[0] & 0x80;
  int     num = (int)(bp[0]&0x7f);


  flip = 0;

  // copia "nindices, nverts, nstrips, stripsize" y los punteros
  // a los buffers de vertices e indices.
  aux = *m;


  // crea nuevo multiobjeto
  if (!qmesh_alloc(m, aux.nv*num, aux.nq*num))
    return 0;

  // TODO: move this to a common place
  if (type==1)
  {
  }
  else
  {
    dir.x = (float)(bp[1]-128)*(1.0f/127.0f);
    dir.y = (float)(bp[2]-128)*(1.0f/127.0f);
    dir.z = (float)(bp[3]-128)*(1.0f/127.0f);
  }

  // for each clone	
  for (j=0; j<num; j++)
  {
    off = aux.nv*j;

    // copy verts
    memcpy(&m->verts[off], aux.verts, aux.nv*sizeof(QVERT));

    // retouch los clones
    //if (cfunc)
    //flip = cfunc(m->verts+off, aux.nv, j, num);

    if (type==1)
      cfuncCircular(m->verts+off, aux.nv, j, num);
    else
      cfuncLinear(m->verts+off, aux.nv, j, num, &dir, fp[0]);


    // copy faces
    dst = m->quads  + aux.nq*j;
    ori = aux.quads;
    for (i=0; i<aux.nq; i++)
    {
      *dst = *ori;

      for (k=0; k<ori->n; k++)
      {
        //if (flip) dst->v[k] = ori->v[ori->n-1-k];
        dst->v[k] += off;
      }

      dst++;
      ori++;
    }
  }


  // libera vertices e indices viejos
  qmesh_free(&aux);

  qmesh_normalize(m);
  //mesh_copyvert(m);      

  return 1;
}

#endif
