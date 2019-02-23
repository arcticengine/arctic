// The MIT License (MIT)
//
// Copyright (c) 2017 Huldra
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

#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include <chrono>  // NOLINT
#include <random>

#include "engine/arctic_platform.h"
#include "engine/easy_sprite.h"
#include "engine/vec2f.h"

namespace arctic {

struct MathTables {
  std::vector<Si32> circle_16_16;
  Si32 cicrle_16_16_size_bits = 0;
  Si32 cicrle_16_16_mask = 0;
  
  void Init();
};
  
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

  std::chrono::high_resolution_clock clock_;
  std::chrono::high_resolution_clock::time_point start_time_;
  double time_correction_;
  double last_time_;

  std::independent_bits_engine<std::mt19937_64, 32, Ui64> rnd_64_;
  std::independent_bits_engine<std::mt19937_64, 32, Ui64> rnd_32_;
  std::independent_bits_engine<std::mt19937_64, 32, Ui64> rnd_16_;
  std::independent_bits_engine<std::mt19937_64, 32, Ui64> rnd_8_;


  bool is_inverse_y_ = false;
  
  MathTables math_tables_;

 public:
  void Init(Si32 width, Si32 height);
  void Draw2d();
  easy::Sprite GetBackbuffer() {
    return backbuffer_texture_;
  }
  void ResizeBackbuffer(const Si32 width, const Si32 height);
  double GetTime();
  Si64 GetRandom(Si64 min, Si64 max);
  Vec2Si32 MouseToBackbuffer(Vec2F pos) const;
  void OnWindowResize(Si32 width, Si32 height);
  Vec2Si32 GetWindowSize() const;
  void SetInverseY(bool is_inverse);
  MathTables &GetMathTables() {
    return math_tables_;
  }
};

}  // namespace arctic

#endif  // ENGINE_ENGINE_H_
