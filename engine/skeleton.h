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

#include <vector>
#include "engine/mesh.h"
#include "engine/mat44f.h"

namespace arctic {

class piSkeleton {
public:
  piSkeleton();
  ~piSkeleton();

  bool Init(int maxBones);
  int AddBone(int parentID);


  void UpdateBone(int id, const Mat44F & m);
  void Update();
  void GetData(void *data);

  typedef struct _piBone {
    Mat44F mLocalMatrix;
    Mat44F mGlobalMatrix;

    int mNumChildren;
    struct _piBone *mChild[8];
  }piBone;

private:
  void iUpdateBoneGlobalMatrix(piBone *me, piBone *parent);

private:

  std::vector<piBone> mBones;
  void *mRoot;
};


} // namespace arctic
