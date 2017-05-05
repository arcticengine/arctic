// The MIT License(MIT)
//
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "engine/arctic_platform.h"
#include "engine/easy_sprite.h"

namespace arctic {

class Engine {
 private:
    Si32 width_ = 0;
    Si32 height_ = 0;
    Ui32 backbuffer_texture_name_ = 0;
    easy::Sprite backbuffer_texture_;

    ByteArray visible_verts_;
    ByteArray visible_normals_;
    ByteArray tex_coords_;
    ByteArray visible_indices_;

    Si32 verts_ = 0;
    Si32 normals_ = 0;
    Si32 tex_ = 0;
    Si32 indices_ = 0;

 public:
    void Init(Si32 width, Si32 height);
    void Draw2d();
    easy::Sprite GetBackbuffer() {
        return backbuffer_texture_;
    }
    void ResizeBackbuffer(const Si32 width, const Si32 height);
};

}  // namespace arctic

#endif  // ENGINE_ENGINE_H_
