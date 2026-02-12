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
#include "skeleton.h"

namespace arctic {

piSkeleton::piSkeleton() {
}

piSkeleton::~piSkeleton() {
}


bool piSkeleton::Init(int maxBones) {
  mBones.clear();
  mBones.reserve(maxBones);
  mRootIndex = -1;
  return true;
}

int piSkeleton::AddBone(int parentID) {
  const int id = (int)mBones.size();
  mBones.emplace_back();
  mBones[id].mNumChildren = 0;
  if (parentID >= 0) {
    piBone &parent = mBones[parentID];
    parent.mChildIndex[parent.mNumChildren++] = id;
  } else {
    mRootIndex = id;
  }
  return id;
}


void piSkeleton::UpdateBone(int id, const Mat44F & m) {
  mBones[id].mLocalMatrix = m;
}

void piSkeleton::iUpdateBoneGlobalMatrix(int boneIndex, int parentIndex) {
  piBone &me = mBones[boneIndex];
  Mat44F m = me.mLocalMatrix;

  if (parentIndex >= 0) {
    m = mBones[parentIndex].mGlobalMatrix * m;
  }

  me.mGlobalMatrix = m;

  const int num = me.mNumChildren;
  for (int i = 0; i < num; i++) {
    iUpdateBoneGlobalMatrix(me.mChildIndex[i], boneIndex);
  }
}

void piSkeleton::Update() {
  if (mRootIndex < 0) {
    return;
  }
  iUpdateBoneGlobalMatrix(mRootIndex, -1);
}

void piSkeleton::GetData(void *data) {
  char *ptr = (char*)data;
  const int num = (int)mBones.size();
  for (int i=0; i<num; i++) {
    const piBone *me = &mBones[i];
    memcpy(ptr, &me->mGlobalMatrix, sizeof(Mat44F));
    ptr += sizeof(Mat44F);
  }

}

}
