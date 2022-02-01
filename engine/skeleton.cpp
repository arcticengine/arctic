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
  mRoot = nullptr;
  return true;
}

int piSkeleton::AddBone(int parentID) {
  const int id = (int)mBones.size();
  mBones.emplace_back();
  piBone *me = &mBones.back();
  me->mNumChildren = 0;
  if (parentID>0) {
    piBone *parent = &mBones[parentID];
    parent->mChild[parent->mNumChildren++] = me;
  } else {
    mRoot = me;
  }
  return id;
}


void piSkeleton::UpdateBone(int id, const Mat44F & m) {
  piBone *me = (piBone*)&mBones[id];
  me->mLocalMatrix = m;
}

void piSkeleton::iUpdateBoneGlobalMatrix(piBone *me, piBone *parent) {
  Mat44F m = me->mLocalMatrix;    

  if (parent !=nullptr) {
    m = parent->mGlobalMatrix * m;
  }

  me->mGlobalMatrix = m;

  const int num = me->mNumChildren;
  for (int i=0; i<num; i++) {
    iUpdateBoneGlobalMatrix(me->mChild[i], me);
  }
}

void piSkeleton::Update() {
  iUpdateBoneGlobalMatrix((piBone*)mRoot, nullptr);    
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
