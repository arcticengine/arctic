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

#include "engine/vec3f.h"
#include "engine/mesh.h"

namespace arctic {

Vec3F MeshFace_GetNormal(const Mesh *me, int faceID, float * nlen);
Vec3F MeshFace_GetCenter(const Mesh *me, int faceID);
Mat44F MeshFace_GetFrame(const Mesh *me, int faceID);
void MeshFace_GetFrame2(const Mesh *me, int faceID, Mat44F & ltw, Mat44F & w2l);

void MeshFace_TransformGlobal(Mesh *mesh, int faceID, const Mat44F & mat);
void MeshFace_TransformLocal(Mesh *mesh, int faceID, const Mat44F & mat);

// obsolete?
void MeshFace_Shrink(Mesh *mesh, int faceID, float amount);
void MeshFace_Extrude(Mesh *mesh, int faceID, float amount);
void MeshFace_ExtrudeDir(Mesh *mesh, int faceID, const Vec3F & dir, float amount);
void MeshFace_ExtrudeTo(Mesh *mesh, int faceID, const Vec3F & xyz);

}