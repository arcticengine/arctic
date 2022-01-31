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
#include "engine/vec3f.h"

namespace arctic {


void Mesh_Scale(Mesh *me, const Vec3F &xyz);
void Mesh_Translate(Mesh *me, const Vec3F &xyz);
void Mesh_Orientate(Mesh *me, const float *au, const float *av, const float *aw);
void Mesh_Sit(Mesh *me);
void Mesh_Tap(Mesh *me, float factor);
void Mesh_SwapYZ(Mesh *me);

} // namespace arctic
/*
typedef void (*D_FUNC)(QVERT *vert, unsigned char param);//, float *params);
//typedef int  (*C_FUNC)(VERT *vert, int num, int clone, int nclones); 

// (scale) = (fp0, fp1, fp2)
int  mod_scale(       QMESH *m, float *fp, unsigned char *bp, intptr *vp);

// (translate) = (bp0, bp1, bp2, fp0)
int  mod_translate(   QMESH *m, float *fp, unsigned char *bp, intptr *vp);

// (axis),(angle) = (bp0, bp1, bp2), (fp3)
int  mod_rotate(      QMESH *m, float *fp, unsigned char *bp, intptr *vp);

// (params) = (fp0..fp13)
int  mod_texturize(   QMESH *m, float *fp, unsigned char *bp, intptr *vp);

// (func) = (vp[0])
int  mod_vertex_retouch(QMESH *m, float *fp, unsigned char *bp, intptr *vp);

// (num), (func) = (bp[0]), (vp[0])
int  mod_clone(       QMESH *m, float *fp, unsigned char *bp, intptr *vp);
*/
