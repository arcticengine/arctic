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

#include <cmath>
#include <cstring>
#include <vector>

#include "engine/mesh.h"
#include "engine/mesh_gen.h"
#include "engine/mesh_gen_adjacency.h"
#include "engine/mesh_gen_mod_complex.h"
#include "engine/vec3f.h"


namespace arctic {


static void MeshVert_Scale(Mesh *dst, int dstID, float scale) {
  float *dstdata = (float*)dst->GetVertexData(STREAMID, dstID, POSID);
  for (int j=0; j<3; j++) {
    dstdata[j] *= scale;
  }
}

static void MeshVert_SInc(Mesh *dst, int dstID, float s, Mesh *src, int srcID) {
  float *srcdata = (float*)src->GetVertexData(STREAMID, srcID, POSID);
  float *dstdata = (float*)dst->GetVertexData(STREAMID, dstID, POSID);
  for (int j=0; j<3; j++) {
    dstdata[j] += s*srcdata[j];
  }
}

const static float vpw_cc[4] = { 9.0f, 3.0f, 1.0f, 3.0f };
const static float epw_cc[4] = { 3.0f, 3.0f, 1.0f, 1.0f };
const static float vpw_te[4] = { 16.0f, 0.0f, 0.0f, 0.0f };
const static float epw_te[4] = { 4.0f, 4.0f, 0.0f, 0.0f };

bool Mesh_Subdivide(Mesh *mesh, int times, int mode) {
  const float *vpw = (mode==1) ? vpw_te : vpw_cc;
  const float *epw = (mode==1) ? epw_te : epw_cc;

  while (times--) {
    Mesh   aux;
    MeshAdjacency adj;

    if (!MeshAdjacency_Init(&adj, mesh)) {
      return false;
    }

    const int numf = mesh->mFaceData.mIndexArray[0].mNum;
    const int numv = mesh->mVertexData.mVertexArray[0].mNum;
    const int nume = MeshAdjacency_GetNumEdges(&adj);

    if (!aux.Init(1, numv+numf+nume, &mesh->mVertexData.mVertexArray[0].mFormat, kRMVEDT_Polys, 1, numf*4)) {
      return false;
    }

    //////////////////////////////////////////////////////////////////////
    const int foff = 0;
    const int voff = numf;
    const int eoff = numf + numv;

    int numOutFaces = 0;

    for (int i=0; i<numf; i++) {
      MeshFace *fo = mesh->mFaceData.mIndexArray[0].mBuffer + i;
      const int nv = 3;

      int eid[4]; 
      for (int j=0; j<nv; j++) {
        int abcd[4]; for (int k=0; k<nv; k++) abcd[k] = fo->mIndex[ (j+k)%nv ];

        adj.mVertexValence[ abcd[0] ] ++;

        eid[j] = MeshAdjacency_GetEdgeIDContainingVerts(&adj, abcd[0], abcd[1]);

        MeshVert_SInc(&aux, foff + i, 0.25f, mesh, abcd[0]);   // face vertex

        for (int k=0; k<nv; k++) {
          MeshVert_SInc(&aux, voff + abcd[0], vpw[k], mesh, abcd[k]);   // vertex vertex
          MeshVert_SInc(&aux, eoff + eid[j],  epw[k], mesh, abcd[k]);   // edge vertex
        }
      }

      for (int j=0; j<nv; j++) {                                           // make child faces
        MeshFace *df = aux.mFaceData.mIndexArray[0].mBuffer + numOutFaces++;
        df->mIndex[0] = foff + i;
        df->mIndex[1] = eoff + eid[(j+nv-1)%nv]; 
        df->mIndex[2] = voff + fo->mIndex[j];
        df = aux.mFaceData.mIndexArray[0].mBuffer + numOutFaces++;
        df->mIndex[0] = voff + fo->mIndex[j];
        df->mIndex[1] = eoff + eid[ j       ];
        df->mIndex[2] = foff + i;
      }
    }

    for (int i=0; i<numv; i++) {
      MeshVert_Scale(&aux, voff+i, 0.0625f/(float)adj.mVertexValence[i]);
    }
    for (int i=0; i<nume; i++) {
      MeshVert_Scale(&aux, eoff+i, 0.1250f/(float)adj.arisaux[i].va);
    }


    //////////////////////////////////////////////////////////////////////

    mesh->DeInit();
    MeshAdjacency_DeInit(&adj);

    aux.mFaceData.mIndexArray[0].mNum = numOutFaces;
    *mesh = aux;

  }

  return true;
}

bool Mesh_Relax(Mesh *mesh, int times, float r) {
  const int numf = mesh->mFaceData.mIndexArray[0].mNum;

  for (int j=0; j<times; j++) {
    for (int i=0; i<numf; i++) {
      MeshFace *q = mesh->mFaceData.mIndexArray[0].mBuffer + i;

      const int num = 3;
      for (int k=0; k<num; k++) {
        const int ia = q->mIndex[k];
        const int ib = q->mIndex[(k+1)%num];

        float *va = (float*)mesh->GetVertexData(STREAMID, ia, POSID);
        float *vb = (float*)mesh->GetVertexData(STREAMID, ib, POSID);

        float dif[3] = { vb[0]-va[0], vb[1]-va[1], vb[2]-va[2] };

        va[0] = va[0] + dif[0]*r;
        va[1] = va[1] + dif[1]*r;
        va[2] = va[2] + dif[2]*r;

        vb[0] = vb[0] - dif[0]*r;
        vb[1] = vb[1] - dif[1]*r;
        vb[2] = vb[2] - dif[2]*r;
      }
    }
  }
  return true;
}



#if 0
// (base,top) = (fp[0], fp[1])
int mod_extrude(QMESH *mesh, float *fp, unsigned char *bp, intptr *vp)
{
  int           i, j, n, nv, nq;
  int           va, vb;
  int           newnumquads;
  QUAD          *qori;
  QUAD          *qdst;
  QMESH         aux;

  if (!qmesh_alloc(&aux, mesh->nv*2, mesh->nq*6))
    return 0;

  // duplicate verts
  n = mesh->nv;
  for (i=0; i<n; i++)
  {
    aux.verts[  i] = mesh->verts[i];
    aux.verts[  i].vert.y = fp[0];
    aux.verts[n+i] = mesh->verts[i];
    aux.verts[n+i].vert.y = fp[1];
  }


  // duplicate faces
  nq   = mesh->nq;
  qori = mesh->quads;
  qdst = aux.quads;
  newnumquads = 0;
  for (i=0; i<nq; i++, qori++)
  {
    nv = qori->n;

    // top cap
    *qdst++ = *qori;

    // botton cap
    for (j=0; j<nv; j++)
      qdst->v[j] = qori->v[nv-1-j] + n;
    qdst->n = nv;
    qdst++;

    // edge quad
    for (j=0; j<nv; j++)
    {
      va = qori->v[j];
      vb = qori->v[(j+1)%nv];
      //get_ab(qori, j, &va, &vb);
      if (isedge(mesh, va, vb))
      {
        qdst->v[0] = vb;
        qdst->v[1] = va;
        qdst->v[2] = va + n;
        qdst->v[3] = vb + n;
        qdst->n = nv;
        qdst++;
        newnumquads++;
      }
    }
  }

  qmesh_free(mesh);
  mesh->quads = aux.quads;
  mesh->verts = aux.verts;
  mesh->nv    = mesh->nv*2;
  mesh->nq    = mesh->nq*2 + newnumquads;

  return 1;
}



//----------------------------------------------------------------------------------------


static int init_aristas_qmesh(AMANAG *am, QMESH *m)
{
  int     i, a;
  QUAD    *q;

  if (!init_aristas(am, m->nq<<2))
    return 0;

  for (i=0; i<m->nq; i++)
  {
    q = m->quads + i;
    for (a=0; a<q->n; a++)
      mete(am, q->v[a], q->v[(a+1)%q->n], i);
  }

  return 1;
}





static void VERT_Add(QVERT *v1, QVERT *dst)
{
  int i;
  float *sf = (float*)v1;
  float *df = (float*)dst;

  for (i=0; i<5; i++)
    *df++ += *sf++;
}

static void VERT_Scale(QVERT *v, float s)
{
  int i;
  float *df = (float*)v;

  for (i=0; i<5; i++)
    *df++ *= s;
}


static void crea_vertices_catmulclark(QMESH *aux, QMESH *mesh, AMANAG *am)
{
  int     j, k, n, l;
  QVERT    v1, v2;
  QVERT    *dst;
  int     a, b;
  QUAD    *fo;

  dst = aux->verts;

  // puntos de cara [mesh->nfaces]
  fo = mesh->quads;
  for (j=0; j<mesh->nq; j++)
  {
    for (k=0; k<fo->n; k++)
      VERT_Add(mesh->verts + fo->v[k], dst);
    VERT_Scale(dst++, 1.0f/(float)fo->n);
    fo++;
  }

  // puntos de arista [naristas]
  for (j=0; j<am->numaristas; j++)
  {
    a = am->arisaux[j].v0;
    b = am->arisaux[j].v1;

#ifdef DEBUG
    /*
       if (a==-1 || b==-1)
       {
       mdebug_printstr("qmesh_subdivide() :: error 0!!!!!\r\n");
       halt_program();
       }
       */
#endif

    VERT_Add(mesh->verts+a, dst);
    VERT_Add(mesh->verts+b, dst);

    a = am->arisaux[j].c0;
    b = am->arisaux[j].c1;

    //pi!!!!!!!!!!!! bordes!
    if (b==-1) b=a;

    VERT_Add(aux->verts+a, dst);
    VERT_Add(aux->verts+b, dst);
    VERT_Scale(dst++, 0.25f);
  }


  // los vertices viejos [mesh->nverts]
  for (j=0; j<mesh->nv; j++)
  {
    n = aristas_num4v(am, j);

    if (n<3) n=3;



    //        if (n<3) n=3;

    //--- Q ---
    memset(&v1, 0, sizeof(QVERT));
    l = 0;
    fo = mesh->quads;
    for (k=0; k<mesh->nq; k++)
    {

      if (fo->v[0]==j || fo->v[1]==j || fo->v[2]==j || ((fo->v[3]==j)&&(fo->n==4)))
      {
        VERT_Add(aux->verts+k, &v1);
        l++;
      }
      fo++;
    }
    VERT_Scale(&v1, 1.0f/(float)l);
    //--- R ---
    memset(&v2, 0, sizeof(QVERT));
    l = 0;
    for (k=0; k<am->numaristas; k++)
    {
      a = am->arisaux[k].v0;
      b = am->arisaux[k].v1;
#ifdef DEBUG
      //if (a==-1 || b==-1)
      //{
      //mdebug_printstr("qmesh_subdivide() :: error 3!!!!!\r\n");
      //return 0;
      //}
#endif
      if (a==j || b==j)
      {
        VERT_Add(mesh->verts+a, &v2);
        VERT_Add(mesh->verts+b, &v2);
        l++;
      }
    }
    VERT_Scale(&v2, 1.0f/(float)l);
    //--- V ---
    *dst = mesh->verts[j];
    VERT_Scale(dst, (float)(n-3));
    VERT_Add(&v1, dst);
    VERT_Add(&v2, dst);
    VERT_Scale(dst++, 1.0f/(float)n);
  }

}







*/


/*
// (dir, tolerance) = (bp[0], fp[0])
int mod_symmetrice( QMESH *mesh, float *fp, unsigned char *bp, intptr *vp)
{
int           i, j, nv, nq;
QUAD          *qori;
QUAD          *qdst;
QMESH         aux;

nv = mesh->nv;
nq = mesh->nq;

if (!qmesh_alloc(&aux, nv*2, nq*2))
return 0;


for (i=0; i<nv; i++)
{
aux.verts[   i] = mesh->verts[i];
aux.verts[nv+i] = mesh->verts[i];

((float*)&aux.verts[nv+i].vert.x)[bp[0]] *= -1.0f;
}

for (i=0; i<nq; i++)
{
qori = mesh->quads + i;
qdst = aux.quads + i;

qdst[0]  = qori[0];
qdst[nq] = qori[0];

for (j=0; j<qori->n; j++)
{
int k = qori->v[j];

float x = ((float*)&aux.verts[k].vert.x)[bp[0]];
if (x>fp[0])
k += nv;
qdst[nq].v[qori->n-1-j] = k;
}   

}


qmesh_free(mesh);
/*
mesh->quads = aux.quads;
mesh->verts = aux.verts;
mesh->nv    = aux.nv;
mesh->nq    = aux.nq;
*/
*mesh = aux;

return 1;
}

#endif


bool Mesh_ExtrudeFace(Mesh *me, int faceID) {
  {
    const MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
    const int num = 3;
    if (!me->Expand(num, num)) {
      return false;
    }
  }

  MeshFace *face = me->mFaceData.mIndexArray[0].mBuffer + faceID;
  const int num = 3;


  const int newIndices = me->mVertexData.mVertexArray[0].mNum;
  const int newFace = me->mFaceData.mIndexArray[0].mNum;
  for (int i=0; i<num; i+=2) {
    MeshFace *nface = me->mFaceData.mIndexArray[0].mBuffer + newFace + i;
    nface->mIndex[0] =  face->mIndex[i];
    nface->mIndex[1] =  face->mIndex[(i+1)%num];
    nface->mIndex[2] =  newIndices + ((i+1)%num);
    nface = me->mFaceData.mIndexArray[0].mBuffer + newFace + i + 1;
    nface->mIndex[0] =  newIndices + ((i+1)%num);
    nface->mIndex[1] =  newIndices +   i;
    nface->mIndex[2] =  face->mIndex[i];
  }
  //--------

  for (int i=0; i<num; i++) {
    Vec3F *w = (Vec3F *)me->GetVertexData(STREAMID, face->mIndex[i], POSID);
    Vec3F *v = (Vec3F *)me->GetVertexData(STREAMID, newIndices+i, POSID);

    *v = *w;
  }
  //--------
  for (int i=0; i<num; i++) {
    face->mIndex[i] = newIndices + i;
  }

  me->mVertexData.mVertexArray[0].mNum += num;
  me->mFaceData.mIndexArray[0].mNum += num;

  return true;
}



bool Mesh_Clip(Mesh *mesh, float y) {
  const int numf = mesh->mFaceData.mIndexArray[0].mNum;

  MeshFace *faces = (MeshFace*)malloc(numf*sizeof(MeshFace));
  if (!faces) {
    return false;
  }
  int nnumf = 0;
  for (int i=0; i<numf; i++) {
    MeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;
    //bool canRemove = true;
    int above = 0;
    for (int j=0; j<3; j++) {
      const int vid = mf->mIndex[j];
      Vec3F *vert = (Vec3F *)mesh->GetVertexData(STREAMID, vid, POSID);
      //if (vert->y>y) { canRemove=false; break; }
      if (vert->y>y) {
        above++;
      }
    }

    //if (above==mf->mNum) faces[nnumf++] = *mf;
    if (above>0) {
      faces[nnumf++] = *mf;
    }

    //if (!canRemove) faces[nnumf++] = *mf;
  }

  free(mesh->mFaceData.mIndexArray[0].mBuffer);
  mesh->mFaceData.mIndexArray[0].mBuffer  = faces;
  mesh->mFaceData.mIndexArray[0].mNum = nnumf;
  return true;
}


bool Mesh_Split(Mesh *outMesh, Mesh *mesh, int nx, int ny, int nz, int *resNum) {
  const int numf = mesh->mFaceData.mIndexArray[0].mNum;

  mesh->CalcBBox(STREAMID, POSID);

  const Bound3F ob = mesh->mBBox;

  const int numOutMeshs = nx*ny*nz;
  if (numOutMeshs>256) {
    return false;
  }
  std::vector<MeshFace> faces[256];
  for (int i=0; i<numOutMeshs; i++) {
    faces[i].reserve(numf); 
  }

  int nnumf = 0;
  for (int i=0; i<numf; i++) {
    const MeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;
    const float *vert = (float*)mesh->GetVertexData(STREAMID, mf->mIndex[0], POSID);

    const int ix = (int)floorf((vert[0] - ob.min_x)/(ob.max_x - ob.min_x) * (float)nx * 0.99999f);
    const int iy = (int)floorf((vert[1] - ob.min_y)/(ob.max_y - ob.min_y) * (float)ny * 0.99999f);
    const int iz = (int)floorf((vert[2] - ob.min_z)/(ob.max_z - ob.min_z) * (float)nz * 0.99999f);
    const int id = ix + iy*nx + iz*nx*ny;

    if (id<0 || id>=numOutMeshs) {
      return false;
    }

    faces[id].push_back(*mf);
  }

  int remap[256];
  int num = 0;
  for (int i=0; i<numOutMeshs; i++) {
    const int numDstFaces = faces[i].size();
    if (numDstFaces!=0) {
      remap[num++] = i;
    }
  }

  *resNum = num;

  for (int i=0; i<num; i++) {
    Mesh *dst = outMesh + i;
    std::vector<MeshFace> *src = faces + remap[i];
    const int numDstFaces = src->size();
    const int numDstVerts = mesh->mVertexData.mVertexArray[0].mNum;
    if (!dst->Init(1, numDstVerts, &mesh->mVertexData.mVertexArray[0].mFormat, mesh->mFaceData.mType, 1, numDstFaces)) {
      return false;
    }
    memcpy(dst->mFaceData.mIndexArray[0].mBuffer, &src[0], numDstFaces*sizeof(MeshFace));
    memcpy(dst->mVertexData.mVertexArray[0].mBuffer, mesh->mVertexData.mVertexArray[0].mBuffer, numDstVerts*mesh->mVertexData.mVertexArray[0].mFormat.mStride);

    if (!dst->Compact()) {
      return false;
    }
  }

  return true;
}


bool Mesh_Separate(Mesh *mesh) {
  const int nums = mesh->mVertexData.mNumVertexArrays;
  const int nume = mesh->mFaceData.mNumIndexArrays;

  if (nums>1 || nume>1) {
    return false;
  }

  const int numf = mesh->mFaceData.mIndexArray[0].mNum;

  int numv = 0;
  for (int i = 0; i<numf; i++) {
    MeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;
    numv += 3;
  }

  Mesh tmp;

  if (!tmp.Init(1, numv, &mesh->mVertexData.mVertexArray[0].mFormat, mesh->mFaceData.mType, 1, numf)) {
    return false;
  }

  tmp.mFaceData.mIndexArray[0].mMax = numf;
  tmp.mFaceData.mIndexArray[0].mNum = numf;

  numv = 0;
  for (int i = 0; i<numf; i++) {
    MeshFace *mf = mesh->mFaceData.mIndexArray[0].mBuffer + i;

    for (int j=0; j<3; j++) {
      const int vid = mf->mIndex[j];
      char data[256];
      mesh->GetVertex(0, vid, data);
      tmp.SetVertex(0, numv, data);
      tmp.mFaceData.mIndexArray[0].mBuffer[i].mIndex[j] = numv;
      numv++;
    }
  }

  mesh->DeInit();
  memcpy(mesh, &tmp, sizeof(Mesh));

  return true;
}



}
