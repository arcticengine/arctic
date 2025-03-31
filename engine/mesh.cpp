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

#include "engine/data_reader.h"
#include "engine/data_writer.h"
#include "engine/easy_files.h"
#include "engine/mesh.h"
#include "engine/arctic_platform_fatal.h"

namespace arctic {

static const unsigned int typeSizeof[] = {
  1,  // piRMVEDT_UByte = 0,
  4,  // piRMVEDT_Float = 1,
  4,  // piRMVEDT_Int   = 2,
  8,  // piRMVEDT_Double= 3,
};

unsigned int MeshVertexElemInfo::GetElementSize() const {
    return mNumComponents * typeSizeof[mType];
}

int MeshVertexFormat::AddElement(unsigned int numComponents, MeshVertexElemDataType type, bool normalize) {
    if (mNumElems >= Mesh_MAXELEMS) {
      return -1;  // Format is already full
    }
    
    // Set up the new element
    MeshVertexElemInfo& elem = mElems[mNumElems];
    elem.mNumComponents = numComponents;
    elem.mType = type;
    elem.mNormalize = normalize;
    
    // Calculate the offset based on existing elements
    if (mNumElems > 0) {
      const MeshVertexElemInfo& prevElem = mElems[mNumElems - 1];
      elem.mOffset = prevElem.mOffset + prevElem.GetElementSize();
    } else {
      elem.mOffset = 0;
    }
    
    // Update the stride
    mStride += elem.GetElementSize();
    
    // Return the index of the newly added element
    return mNumElems++;
}

Mesh::Mesh() {
}

Mesh::~Mesh() {
  DeInit();
}

bool Mesh::Init(int numVertexStreams, int nv,
    const MeshVertexFormat *vertexFormat,
    MeshType type, int numElementsArrays, int numElements) {
  if ((numElements<0) || (vertexFormat[0].mNumElems>Mesh_MAXELEMS)) {
    return false;
  }
  if ((type==kRMVEDT_Polys) && (nv<3)) {
    return false;
  }

  memset((char*)this, 0, sizeof(Mesh));


  mVertexData.mNumVertexArrays = 0;//numVertexStreams;
  for (int j=0; j<numVertexStreams; j++) {
    if (!this->AddVertexStream(nv, vertexFormat+j)) {
      return false;
    }
  }

  //--------------------
  mFaceData.mNumIndexArrays = numElementsArrays;
  mFaceData.mType = type;
  for (int i=0; i<numElementsArrays; i++) {
    mFaceData.mIndexArray[i].mMax  = numElements;
    mFaceData.mIndexArray[i].mNum  = 0;
    if (numElements>0) {
      const int bufferSize = numElements * ((mFaceData.mType==kRMVEDT_Polys) ? sizeof(MeshFace) : sizeof(unsigned int));
      mFaceData.mIndexArray[i].mBuffer = (MeshFace*)malloc(bufferSize);
      if (!mFaceData.mIndexArray[i].mBuffer) {
        return false;
      }
      memset(mFaceData.mIndexArray[i].mBuffer, 0, bufferSize);
    }
  }
  return true;
}

bool Mesh::AddVertexStream(const int nv, const MeshVertexFormat *vf) {
  const int id = mVertexData.mNumVertexArrays;

  mVertexData.mVertexArray[id].mMax = nv;
  mVertexData.mVertexArray[id].mNum = 0;
  mVertexData.mVertexArray[id].mFormat = *vf;

  mVertexData.mVertexArray[id].mBuffer = malloc(nv*vf->mStride);
  if (!mVertexData.mVertexArray[id].mBuffer) {
    return false;
  }
  memset(mVertexData.mVertexArray[id].mBuffer, 0, nv*vf->mStride);

  int off = 0;
  for (int i=0; i<vf->mNumElems; i++) {
    mVertexData.mVertexArray[id].mFormat.mElems[i].mOffset = off;
    off += vf->mElems[i].mNumComponents*typeSizeof[vf->mElems[i].mType];
  }

  mVertexData.mNumVertexArrays = id + 1;

  return true;
}

bool Mesh::Clone(Mesh *dst) {
  const int nv = mVertexData.mVertexArray[0].mNum;
  const int numElements = mFaceData.mIndexArray[0].mNum;

  const MeshVertexFormat *vf = &mVertexData.mVertexArray[0].mFormat;

  if (!dst->Init(1, nv, vf, mFaceData.mType, mFaceData.mNumIndexArrays, numElements)) {
    return false;
  }

  const int bufferSize = numElements * ((mFaceData.mType == kRMVEDT_Polys) ? sizeof(MeshFace) : sizeof(unsigned int));

  memcpy(dst->mVertexData.mVertexArray[0].mBuffer,
      mVertexData.mVertexArray[0].mBuffer,
      nv*vf->mStride);
  memcpy(dst->mFaceData.mIndexArray[0].mBuffer,
      mFaceData.mIndexArray[0].mBuffer,
      bufferSize);
  return true;
}

void Mesh::DeInit() {
  for (int i=0; i<mVertexData.mNumVertexArrays; i++) {
    if (mVertexData.mVertexArray[i].mMax) {
      free(mVertexData.mVertexArray[i].mBuffer);
      mVertexData.mVertexArray[i].mMax = 0;
    }
  }
  for (int i=0; i<mFaceData.mNumIndexArrays; i++) {
    if (mFaceData.mIndexArray[i].mMax) { 
      free(mFaceData.mIndexArray[i].mBuffer);
      mFaceData.mIndexArray[i].mMax = 0;
    }
  }
}

void Mesh::ClearGeometry() {
  for (int i=0; i<mVertexData.mNumVertexArrays; i++) {
    mVertexData.mVertexArray[i].mNum = 0;
  }
  for (int i=0; i<mFaceData.mNumIndexArrays; i++) {
    mFaceData.mIndexArray[i].mNum = 0;
  }
}

int Mesh::Save(const char *name) {
  DataWriter fp;
  fp.data.reserve(1024*1024);

  fp.WriteFloatarray2((float*)&mBBox, 6);

  fp.WriteUInt32( mVertexData.mNumVertexArrays);

  for (int j = 0; j<mVertexData.mNumVertexArrays; j++) {
    MeshVertexArray *va = mVertexData.mVertexArray + j;
    fp.WriteUInt32(va->mNum);
    fp.WriteUInt32(va->mFormat.mStride);
    fp.WriteUInt32(va->mFormat.mDivisor);
    fp.WriteUInt32(va->mFormat.mNumElems);

    for (int i=0; i<va->mFormat.mNumElems; i++) {
      fp.WriteUInt32((int)va->mFormat.mElems[i].mType);
      fp.WriteUInt32(va->mFormat.mElems[i].mNumComponents);
      fp.WriteUInt32(va->mFormat.mElems[i].mNormalize ? 1 : 0);
      fp.WriteUInt32(va->mFormat.mElems[i].mOffset);
    }

    if (va->mNum>0) {
      fp.WriteFloatarray2((float*)va->mBuffer, va->mNum*va->mFormat.mStride / 4);
    }
  }

  const int esize = (mFaceData.mType==kRMVEDT_Polys) ? sizeof(MeshFace) : sizeof(unsigned int);
  fp.WriteUInt32( (mFaceData.mType==kRMVEDT_Polys)?0:1);
  fp.WriteUInt32( mFaceData.mNumIndexArrays);
  for (int i=0; i<mFaceData.mNumIndexArrays; i++) {
    fp.WriteUInt32( mFaceData.mIndexArray[i].mNum);

    if (mFaceData.mIndexArray[i].mNum>0) {
      fp.WriteUInt32array((unsigned int*)mFaceData.mIndexArray[i].mBuffer, mFaceData.mIndexArray[i].mNum*esize/4);
    }
  }

  WriteFile(name, fp.data.data(), fp.data.size());
  return 0;
}

bool Mesh::Load(const char *name) {
  DataReader fp;
  fp.Reset(ReadFile(name));

  fp.ReadFloatarray2((float*)&mBBox, 6);

  mVertexData.mNumVertexArrays = fp.ReadUInt32();

  for (int j = 0; j<mVertexData.mNumVertexArrays; j++) {
    MeshVertexArray *va = mVertexData.mVertexArray + j;

    va->mNum = fp.ReadUInt32();
    va->mFormat.mStride = fp.ReadUInt32();
    va->mFormat.mDivisor = fp.ReadUInt32();
    va->mFormat.mNumElems = fp.ReadUInt32();
    va->mMax = va->mNum;

    for (int i=0; i<va->mFormat.mNumElems; i++) {
      va->mFormat.mElems[i].mType = (MeshVertexElemDataType)fp.ReadUInt32();
      va->mFormat.mElems[i].mNumComponents = fp.ReadUInt32();
      va->mFormat.mElems[i].mNormalize = (fp.ReadUInt32() == 1);
      va->mFormat.mElems[i].mOffset = fp.ReadUInt32();
    }

    if (va->mNum>0) {
      va->mBuffer = malloc(va->mNum*va->mFormat.mStride);
      if (!va->mBuffer) {
        return false;
      }

      fp.ReadFloatarray2((float*)va->mBuffer, va->mNum*va->mFormat.mStride / 4);
    }

  }


  mFaceData.mType = (fp.ReadUInt32()==0)?kRMVEDT_Polys:kRMVEDT_Points;
  mFaceData.mNumIndexArrays = fp.ReadUInt32();
  const int esize = (mFaceData.mType==kRMVEDT_Polys) ? sizeof(MeshFace) : sizeof(unsigned int);
  for (int i=0; i<mFaceData.mNumIndexArrays; i++) {
    mFaceData.mIndexArray[i].mNum = fp.ReadUInt32();
    mFaceData.mIndexArray[i].mMax = mFaceData.mIndexArray[i].mNum;
    if (mFaceData.mIndexArray[i].mNum>0) {
      mFaceData.mIndexArray[i].mBuffer = (MeshFace*)malloc(mFaceData.mIndexArray[i].mNum*esize);
      if (!mFaceData.mIndexArray[i].mBuffer) {
        return false;
      }

      fp.ReadUInt32array( (unsigned int*)mFaceData.mIndexArray[i].mBuffer, mFaceData.mIndexArray[i].mNum*esize/4);
    }
  }

  return true;
}


#if 0
  int Mesh_MoveStream(Mesh *me, int dst, int ori)
  {
    pStream[dst] = pStream[ori];
    memset(pStream+ori, 0, sizeof(MeshStream));
    return 1;
  }

  int Mesh_NewStream(Mesh *me, int num, const int *chunktypes, int *pos)
  {
    for (int i=0; i<MAX_STREAMS; i++)
    {
      MeshStream *st = pStream + i;
      if (st->stride==0)
      {
        st->num_chunks = 0;
        st->stride = 0;
        for (int j=0; chunktypes[j]!=0; j++)
        {
          st->num_chunks++;
          st->chunktype[j] = chunktypes[j];
          st->stride += Mesh_getchunksize(chunktypes[j]);
        }

        if (num)
        {
          size_t amount = (size_t)num*(size_t)st->stride*(size_t)sizeof(float);
          st->buffer = (float*)malloc(amount);
          if (!st->buffer)
          {
            //                    LOG_Printf(LT_EXTRA_PARAMS, LT_ERROR, "Could not allocate %d megabytes\n", amount>>20);
            return 0;
          }
        }
        *pos = i;
        return 1;
      }
    }

    return 0;

  }




  /*
     int  Mesh_Expand(Mesh *me, int nnv, int nnf, int *vbase, int *fbase)
     {
     MeshFace  *nf;
     int     i;
     float   *ns[MAX_STREAMS];
     long amount;

     nf = (MeshFace*)malloc((pNumFaces+nnf)*sizeof(MeshFace));
     if (!nf)
     {
  //LOG_Printf(LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", ((pNumFaces+nnf)*sizeof(MeshFace))>>20);
  return 0;
  }

  if (pNumFaces)
  memcpy(nf, pFace, pNumFaces*sizeof(MeshFace));


  for (i=0; i<MAX_STREAMS; i++)
  if (pStream[i].stride)
  {
  amount = ((size_t)pNumVertices+(size_t)nnv)*(size_t)pStream[i].stride*(size_t)sizeof(float);
  ns[i] = (float*)malloc(amount);
  if (!ns[i])
  {
  //LOG_Printf(LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", amount>>20);
  free(nf);
  return 0;
  }
  }

  if (pNumVertices)
  for (i=0; i<MAX_STREAMS; i++)
  if (pStream[i].stride)
  {
  amount = (long)pNumVertices*(long)pStream[i].stride*(long)sizeof(float);
  memcpy(ns[i], pStream[i].buffer, amount);
  }

  if (pNumFaces)    
  free(pFace);
  pFace = nf;


  for (i=0; i<MAX_STREAMS; i++)
  if (pStream[i].stride)
  {
  if (pNumVertices)
  free(pStream[i].buffer);
  pStream[i].buffer = ns[i];
  }

   *vbase = pNumVertices;
   *fbase = pNumFaces;

   pNumVertices += nnv;
   pNumFaces += nnf;

   return 1;
   }
   */

  int Mesh::Load(const char *name) {
    int		i, j;
    MeshStream *st;
    size_t amount;

    DataReader dr;
    dr.Reset(std::move(ReadFile(name)));

    memset(me, 0, sizeof(Mesh));

    //LOG_Printf("sizeof(DMATTABLE) = %d\n", sizeof(DMATTABLE));

    pNumVertices = piFile_readUInt32(fp);
    pNumFaces = piFile_readUInt32(fp);
    //   pMatTable.numMats = 0;

    for (i=0; i<MAX_STREAMS; i++) {
      st = pStream+i;

      st->num_chunks = piFile_readUInt32(fp);
      st->stride = 0;
      if (st->num_chunks) {
        for (j=0; j<st->num_chunks; j++) {
          st->chunktype[j] = piFile_readUInt32(fp);
          st->stride += Mesh_getchunksize(st->chunktype[j]);
        }
      }
    }



    amount = (size_t)pNumFaces*(size_t)sizeof(MeshFace);

    pFace = (MeshFace*)malloc(amount);
    if (!pFace) {
      //LOG_Printf(LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", amount>>20);
      return 0;
    }

    piFile_readUInt32array(fp, (unsigned int*)pFace, pNumFaces, 3, 4);
    //fread((unsigned int*)pFace, pNumFaces, 3*4, fp);

    for (i=0; i<MAX_STREAMS; i++) {
      st = pStream+i;
      if (st->num_chunks) {
        amount = (size_t)pNumVertices*(size_t)st->stride*(size_t)sizeof(float);
        st->buffer = (float*)malloc(amount);
        if (!st->buffer) {
          //LOG_Printf(LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", amount>>20);
          return 0;
        }
        piFile_readfloatarray2(fp, st->buffer, (size_t)(pNumVertices*st->stride));
      }
    }


    fp.Close();

    return 1;
  }

  */


    /*

       void Mesh_FreeStream(Mesh *me, int i)
       {
       if (pStream[i].stride)
       {
       pStream[i].stride = 0;

       if (pStream[i].buffer)
       free(pStream[i].buffer);

       pStream[i].buffer = 0;
       }
       }




       void Mesh_Free(Mesh *me)
       {
       int	i;

       if (pFace) 
       {
       free(pFace);
       pFace = 0;
       }

       for (i=0; i<MAX_STREAMS; i++)
       Mesh_FreeStream(me, i);

    //   DMATTABLE_Free(&pMatTable);
    }

    void Mesh_Scale(Mesh *me, float x, float y, float z)
    {
    piVec3F *v = (piVec3F*)pStream[0].buffer;
    for (unsigned i=0; i<pNumVertices; i++, v++)
    {
    v->x *= x;
    v->y *= y;
    v->z *= z;
    }
    }


    void Mesh_CalcBBOx(Mesh *me, float *bbox)
    {
    piVec3F *v = (piVec3F*)pStream[0].buffer;

    bbox[0] = v[0].x;
    bbox[1] = v[0].y;
    bbox[2] = v[0].z;
    bbox[3] = v[0].x;
    bbox[4] = v[0].y;
    bbox[5] = v[0].z;

    for (unsigned int i=0; i<pNumVertices; i++, v++)
    {
    const float x = v[0].x;
    const float y = v[0].y;
    const float z = v[0].z;

    if (x<bbox[0]) bbox[0]=x;
    else if (x>bbox[3]) bbox[3]=x;

    if (y<bbox[1]) bbox[1]=y;
    else if (y>bbox[4]) bbox[4]=y;

    if (z<bbox[2]) bbox[2]=z;
    else if (z>bbox[5]) bbox[5]=z;
}
}

//--------------------------------------

static void polynorm(float *pos, int pos_inc, float *nor, int nor_inc, int ia, int ib, int ic)
{
  piVec3F  ab, cb, no;

  piVec3F_Sub(&ab, (piVec3F*)(pos+ia*pos_inc), (piVec3F*)(pos+ib*pos_inc));
  piVec3F_Sub(&cb, (piVec3F*)(pos+ic*pos_inc), (piVec3F*)(pos+ib*pos_inc));

  piVec3F_Cross(&ab, &cb, &no);
  //Vec3Norm(&no);

  piVec3F_Inc((piVec3F*)(nor+ia*nor_inc), &no);
  piVec3F_Inc((piVec3F*)(nor+ib*nor_inc), &no);
  piVec3F_Inc((piVec3F*)(nor+ic*nor_inc), &no);
}

void Mesh_CalcNormals(Mesh *me, int pos_stream, int pos_stride, int pos_offset,
    int nor_stream, int nor_stride, int nor_offset,
    float ang_th)
{
  unsigned int	    i;
  float	*nor_ptr;
  float	*pos_ptr;
  int		nor_inc, pos_inc;
  float	*n;
  MeshFace   *f;

  pos_ptr = pStream[pos_stream].buffer + pos_offset;
  pos_inc = pos_stride / sizeof(float);
  nor_ptr = pStream[nor_stream].buffer + nor_offset;
  nor_inc = nor_stride / sizeof(float);

  ang_th = ang_th*3.1415927f/180.0f;

  n = nor_ptr;
  for (i=0; i<pNumVertices; i++, n += nor_inc)
  {
    n[0] = 0.0f;
    n[1] = 0.0f;
    n[2] = 0.0f;
  }

  f = pFace;
  for (i=0; i<pNumFaces; i++, f++)
  {
    polynorm(pos_ptr, pos_inc,
        nor_ptr, nor_inc,
        f->i[0], f->i[1], f->i[2]);
  }

  n = nor_ptr;
  for (i=0; i<pNumVertices; i++, n += nor_inc)
    piVec3F_Norm((piVec3F*)n);
}


//-------------------------------------------

int Mesh_Merge(Mesh *me) {
  float *buffer;
  unsigned int		i;
  float	*fa, *fb, *fc;

  buffer = (float*)malloc(pNumVertices*8*sizeof(float));
  if (!buffer)
  {
    //LOG_Printf(LT_EXTRA_PARAMS, LT_ERROR, "could not allocate %d megabytes\n", pNumVertices*8*sizeof(float));
    return 0;
  }

  fa = (float*)pStream[0].buffer;
  fb = (float*)pStream[1].buffer;

  fc = buffer;
  for (i=0; i<pNumVertices; i++)
  {
    fc[0] = fa[0];
    fc[1] = fa[1];
    fc[2] = fa[2];
    fc[3] = fb[0];
    fc[4] = fb[1];
    fc[5] = fb[2];
    fc[6] = fb[3];
    fc[7] = fb[4];
    fc += 8;
    fa += 3;
    fb += 5;
  }

  Mesh_FreeStream(me, 0);
  Mesh_FreeStream(me, 1);

  pStream[0].num_chunks = 3;
  pStream[0].stride = 3+3+2;
  pStream[0].buffer = buffer;

  return 1;
}









//============================================================
//include "f_mesh.h"

int Mesh_SerialSave_Open(SESA_DATA *sd, const char *name)
{
  FILE *fp = piFile_Open(name, L"wb");
  if (!fp)
    return 0;

  piFile_writeUInt32(fp, 0);   // nv
  piFile_writeUInt32(fp, 0);   // nf

  sd->me.pNumFaces = 0;
  sd->me.pNumVertices = 0;
  sd->actual_stream = 0;
  sd->fp = fp;

  return 1;
}


void Mesh_SerialSave_AddStride_Add(SESA_DATA *sd, int *chunktypes)
{
  int i;
  int num_chunks = 0;

  for (i=0; chunktypes[i]!=0; i++)
    num_chunks++;

  piFile_writeUInt32(sd->fp, num_chunks);

  sd->me.pStream[sd->actual_stream].stride = 0;
  if (num_chunks)
  {
    for (i=0; chunktypes[i]!=0; i++)
    {
      piFile_writeUInt32(sd->fp, chunktypes[i]);
      sd->me.pStream[sd->actual_stream].stride += Mesh_getchunksize(chunktypes[i]);
    }
  }

  sd->actual_stream++;
}

void Mesh_SerialSave_AddStride_End(SESA_DATA *sd)
{
  int i;

  for (i=sd->actual_stream; i<MAX_STREAMS; i++)
    piFile_writeUInt32(sd->fp, 0);

  sd->actual_stream = 0;
}

void Mesh_SerialSave_AdMeshFace(SESA_DATA *sd, MeshFace *face)
{
  piFile_writeUInt32array(sd->fp, (unsigned int*)face, sizeof(MeshFace)/4);
  sd->me.pNumFaces++;
}

void Mesh_SerialSave_AddVertex_StartStream(SESA_DATA *sd)
{
  sd->actual_stream = 0;
}


void Mesh_SerialSave_AddVertex_Add(SESA_DATA *sd, float *data)
{
  piFile_writeUInt32array(sd->fp, (unsigned int*)data, sd->me.pStream[sd->actual_stream].stride);

  if (sd->actual_stream==0)
    sd->me.pNumVertices++;
}

void Mesh_SerialSave_AddVertex_EnMeshStream(SESA_DATA *sd)
{
  sd->actual_stream++;
}


void Mesh_SerialSave_Close(SESA_DATA *sd)
{
  fseek(sd->fp, 0, SEEK_SET);

  piFile_writeUInt32(sd->fp, sd->me.pNumVertices);
  piFile_writeUInt32(sd->fp, sd->me.pNumFaces);

  piFile_Close(sd->fp);
}

int Mesh_Check(Mesh *me)
{
  unsigned int i, j;

  unsigned int numf = pNumFaces;
  unsigned int numv = pNumVertices;

  // check VA
  for (j=0; j<MAX_STREAMS; j++)
  {
    if (pStream[j].num_chunks)
    {
      float *fptr = pStream[j].buffer;
      int   *iptr = (int*)pStream[j].buffer;
      int   s = pStream[j].stride;
      for (i=0; i<s*numv; i++)
        if (_isnan((double)fptr[i]) || !_finite((double)fptr[i]) || iptr[i]==0xffffffaa)
          return 0;
    }
  }

  // check IA
  for (i=0; i<numf; i++)
  {
    if (pFace[i].i[0]>=numv)
      return 0;
    if ( pFace[i].i[1]>=numv)
      return 0;
    if (pFace[i].i[2]>=numv)
      return 0;
  }

  return 1;
}

//------------------------------------------
/*
   typedef struct
   {
   int a;
   int b;
   }TmpEdge;

   void insertEdge(ARRAY *edges, int a, int b)
   {
   TmpEdge      *e;
   unsigned int i;

   if (a>b) { int t=a; a=b; b=t; } 

   unsigned int num = ARRAY_GetLength(edges);
   e = (TmpEdge*)ARRAY_GetPointer(edges, 0);
   for (i=0; i<num; i++)
   if (e[i].a==a && e[i].b==b)
   return;

   e = (TmpEdge *)ARRAY_Alloc(edges, 1);
   if (!e)
   return;
   e->a = a;
   e->b = b;

   }

   int Mesh_CalcEdges(const Mesh *me, ARRAY *edges)
   {
   unsigned int i;
   unsigned int numF = pNumFaces;

   if (!ARRAY_Init(edges, numF*3, sizeof(TmpEdge)))
   return 0;

   for (i=0; i<numF; i++)
   {
   int a = pFace[i].i[0];
   int b = pFace[i].i[1];
   int c = pFace[i].i[2];

   insertEdge(edges, a, b);
   insertEdge(edges, b, c);
   insertEdge(edges, c, a);
   }


   return 1;
   }*/
#endif

void Mesh::Normalize(int stream, int ppos, int npos) {
  const int numv = mVertexData.mVertexArray[stream].mNum;
  const int numt = mFaceData.mIndexArray[0].mNum;

  for (int i=0; i<numv; i++) {
    float *v = (float*)GetVertexData(stream, i, npos);
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
  }

  for (int i=0; i<numt; i++) {
    MeshFace *face = mFaceData.mIndexArray[0].mBuffer + i;

    const int ft = 3;

    Vec3F nor = Vec3F(0.0f, 0.0f, 0.0f);
    for (int j=0; j<ft; j++) {
      const Vec3F va = *((Vec3F*)GetVertexData(stream, face->mIndex[ j      ], ppos));
      const Vec3F vb = *((Vec3F*)GetVertexData(stream, face->mIndex[(j+1)%ft], ppos));

      nor += Cross(va, vb);
    }

    for (int j=0; j<ft; j++) {
      Vec3F *n = (Vec3F*)GetVertexData(stream, face->mIndex[j], npos);
      n->x += nor.x;
      n->y += nor.y;
      n->z += nor.z;
    }
  }

  for (int i=0; i<numv; i++) {
    Vec3F *v = (Vec3F*)GetVertexData(stream, i, npos);
    *v = arctic::Normalize(*v);
  }
}

void Mesh::CalcBBox(int stream, int pPos) {
  mBBox = Bound3F(1e20f, -1e20f, 1e20f, -1e20f, 1e20f, -1e20f);

  const int num = mVertexData.mVertexArray[stream].mNum;
  for (int i=0; i<num; i++) {
    float *v = (float*)GetVertexData(stream, i, pPos);
    if (v[0]<mBBox.min_x) mBBox.min_x = v[0];
    if (v[1]<mBBox.min_y) mBBox.min_y = v[1];
    if (v[2]<mBBox.min_z) mBBox.min_z = v[2];
    if (v[0]>mBBox.max_x) mBBox.max_x = v[0];
    if (v[1]>mBBox.max_y) mBBox.max_y = v[1];
    if (v[2]>mBBox.max_z) mBBox.max_z = v[2];
  }
}

bool Mesh::Expand(int nv, int nf) {
  for (int j=0; j<mVertexData.mNumVertexArrays; j++) {
    MeshVertexArray *va = mVertexData.mVertexArray + j;

    if ((va->mNum + nv) >= va->mMax){
      const unsigned int newNV = va->mMax + ((nv<64)?64:nv);
      va->mBuffer = realloc(va->mBuffer, newNV*va->mFormat.mStride);
      if (!va->mBuffer) {
        return false;
      }
      va->mMax = newNV;
    }

    if ((mFaceData.mIndexArray[0].mNum + nf) >= mFaceData.mIndexArray[0].mMax) {
      const unsigned int newNF = mFaceData.mIndexArray[0].mMax + ((nf<64)?64:nf);
      mFaceData.mIndexArray[0].mBuffer = (MeshFace*)realloc(mFaceData.mIndexArray[0].mBuffer, newNF*sizeof(MeshFace));
      if (!mFaceData.mIndexArray[0].mBuffer)   
        return false;
      mFaceData.mIndexArray[0].mMax = newNF;
    }
  }

  return true;
}

/*
   void Mesh_RandomizeVerts(Mesh *me) {
     const int num = mVertexData.mNum;
     const int stride = mVertexData.mFormat.mStride;
     char tmp[32*4];
     int seed = 123456;
     for (int i=0; i<num; i++) {
       int j = pirand_int15b(&seed) % num;

       char *ptrA = (char*)mVertexData.mBuffer + stride*i;
       char *ptrB = (char*)mVertexData.mBuffer + stride*j;

       memcpy(tmp, ptrA, stride);
       memcpy(ptrA, ptrB, stride);
       memcpy(ptrB, tmp, stride);
     }
   }*/

int Mesh::Compact() {
  for (int j=0; j<mVertexData.mNumVertexArrays; j++) {
    MeshVertexArray *va = mVertexData.mVertexArray + j;

    const int numSrcVerts = va->mNum;

    const int stride = va->mFormat.mStride;

    std::vector<char> tmp;
    tmp.reserve(numSrcVerts * stride);

    int *tmpHis = (int*)malloc(numSrcVerts * sizeof(int));
    if (!tmpHis) {
      return 0;
    }
    memset(tmpHis, 0, numSrcVerts * sizeof(int));

    const int numf = mFaceData.mIndexArray[0].mNum;
    for (int i=0; i<numf; i++) {
      const MeshFace *fa = mFaceData.mIndexArray[0].mBuffer + i;
      const int nv = 3;
      for (int j=0; j<nv; j++) {
        tmpHis[ fa->mIndex[j] ] += 1;
      }
    }

    int num = 0;
    for (int i=0; i<numSrcVerts; i++) {
      if (tmpHis[i] > 0) {
        for (Si32 idx = 0; idx < stride; ++idx) {
          tmp.emplace_back(*((char*)va->mBuffer + stride*i + idx));
        }

        // change all references
        for (int j=0; j<numf; j++) {
          MeshFace *fa = mFaceData.mIndexArray[0].mBuffer + j;
          const int nv = 3;
          for (int k=0; k<nv; k++) {
            if (fa->mIndex[k] == i) {
              fa->mIndex[k] = num;
            }
          }
        }
        num++;
      }
    }

    free(va->mBuffer);
    va->mBuffer = malloc(num*stride);
    if (!va->mBuffer) {
      free(tmpHis);
      return 0;
    }
    memcpy(va->mBuffer, &tmp[0], num*stride);
    va->mNum = num;
    va->mMax = num;

    free(tmpHis);
  }

  return 1;
}

/*
   void Mesh_CopyChannel(Mesh *mesh, int stream, int src, int dst) {
   const int nv = mesh->mVertexData.mNum;
   for (int i = 0; i<nv; i++) {
   Vec3F *s = (Vec3F*)Mesh_GetVertexData(mesh, i, src);
   Vec3F *d = (Vec3F*)Mesh_GetVertexData(mesh, i, dst);
   d[0] = s[0];
   }
   }
   */

int Mesh::GetVertexSize(int streamID) {
  return mVertexData.mVertexArray[streamID].mFormat.mStride;
}

void Mesh::GetVertex(int streamID, int vertexID, void *data) {
  memcpy(data,
      (char*)mVertexData.mVertexArray[streamID].mBuffer
      + mVertexData.mVertexArray[streamID].mFormat.mStride*vertexID,
      mVertexData.mVertexArray[streamID].mFormat.mStride);
}

void Mesh::SetVertex(int streamID, int vertexID, void *data) {
  memcpy((char*)mVertexData.mVertexArray[streamID].mBuffer + mVertexData.mVertexArray[streamID].mFormat.mStride*vertexID,
      data,
      mVertexData.mVertexArray[streamID].mFormat.mStride);
}

bool Mesh::SetTriangle(int streamID, int triangleID, int a, int b, int c) {
  MeshFace *face = mFaceData.mIndexArray[streamID].mBuffer + triangleID;
  face->mIndex[0] = a;
  face->mIndex[1] = b;
  face->mIndex[2] = c;

  return true;
}

void *Mesh::GetVertexData(int streamID, int vertexID, int elementID) const {
  return (void*)((char*)mVertexData.mVertexArray[streamID].mBuffer
      + vertexID*mVertexData.mVertexArray[streamID].mFormat.mStride
      + mVertexData.mVertexArray[streamID].mFormat.mElems[elementID].mOffset);
}

int Mesh::AddVertex(int streamID, ...) {
    MeshVertexArray* va = &mVertexData.mVertexArray[streamID];
    int vertexIndex = va->mNum;
    if (vertexIndex >= va->mMax) {
      return -1;
    }
    
    // Get the base pointer for the vertex data
    float* vertexData = (float*)((char*)va->mBuffer + vertexIndex * va->mFormat.mStride);
    
    va_list args;
    va_start(args, streamID);

    // Process each element based on the vertex format
    for (int i = 0; i < va->mFormat.mNumElems; i++) {
      MeshVertexElemInfo &elem = va->mFormat.mElems[i];
      char *elemAddr = (char *)vertexData + elem.mOffset;

      // Handle each element type
      switch (elem.mType) {
        case kRMVEDT_UByte:
        {
          unsigned char *elemPtr = (unsigned char *)elemAddr;
          for (unsigned int j = 0; j < elem.mNumComponents; j++) {
            elemPtr[j] = va_arg(args, int);
          }
        }
          break;
        case kRMVEDT_Float:
        {
          float *elemPtr = (float *)elemAddr;
          for (unsigned int j = 0; j < elem.mNumComponents; j++) {
            elemPtr[j] = va_arg(args, double);  // float is promoted to double in varargs
          }
        }
          break;
        case kRMVEDT_Int:
        {
          int *elemPtr = (int *)elemAddr;
          for (unsigned int j = 0; j < elem.mNumComponents; j++) {
            elemPtr[j] = va_arg(args, int);
          }
        }
          break;
        case kRMVEDT_Double:
        {
          double *elemPtr = (double *)elemAddr;
          for (unsigned int j = 0; j < elem.mNumComponents; j++) {
            elemPtr[j] = va_arg(args, double);
          }
        }
          break;
        default:
          Fatal("Unsupported element type");
      }
    }

    va_end(args);
    // Increment vertex count
    va->mNum++;
    return vertexIndex;
}

int Mesh::AddFace(int streamID, int v1, int v2, int v3) {
    MeshIndexArray* ia = &mFaceData.mIndexArray[streamID];
    int faceIndex = ia->mNum;
    
    MeshFace* face = &ia->mBuffer[faceIndex];
    face->mIndex[0] = v1;
    face->mIndex[1] = v2;
    face->mIndex[2] = v3;
    
    ia->mNum++;
    return faceIndex;
}

int Mesh::GetCurrentVertexCount(int streamID) const {
    return mVertexData.mVertexArray[streamID].mNum;
}

int Mesh::GetCurrentFaceCount(int streamID) const {
    return mFaceData.mIndexArray[streamID].mNum;
}

}
