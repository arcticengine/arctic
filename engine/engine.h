// The MIT License (MIT)
//
// Copyright (c) 2017 - 2022 Huldra
// Copyright (c) 2021 Vlad2001_MFS
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
#include <memory>
#include <string>
#include <random>
#include <vector>

#include "engine/arctic_platform.h"
#include "engine/easy_sprite.h"
#include "engine/easy_hw_sprite.h"
#include "engine/vec2f.h"
#include "engine/opengl.h"
#include "engine/gl_texture2d.h"
#include "engine/gl_program.h"
#include "engine/gl_buffer.h"
#include "engine/mesh.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

struct MathTables {
  std::vector<Si32> circle_16_16;
  Si32 cicrle_16_16_size_bits = 0;
  Si32 cicrle_16_16_mask = 0;
  Si32 cicrle_16_16_half_mask = 0;

  void Init();
};

class Engine {
 private:
  Si32 width_ = 0;
  Si32 height_ = 0;
  GlTexture2D gl_backbuffer_texture_;
  Sprite backbuffer_texture_;
  HwSprite hw_backbuffer_texture_;

  Mesh mesh_;
  std::vector<HwSpriteDrawing> hw_sprite_drawing_;

  std::chrono::high_resolution_clock::time_point start_time_;
  double time_correction_ = 0.0;
  double last_time_ = 0.0;

  std::independent_bits_engine<std::mt19937_64, 64, Ui64> rnd_64_;
  std::independent_bits_engine<std::mt19937_64, 32, Ui64> rnd_32_;
  std::independent_bits_engine<std::mt19937_64, 16, Ui64> rnd_16_;
  std::independent_bits_engine<std::mt19937_64, 8, Ui64> rnd_8_;


  bool is_inverse_y_ = false;

  MathTables math_tables_;

  std::shared_ptr<GlProgram> copy_backbuffers_program_;
  std::shared_ptr<GlProgram> default_sprite_program_;

  std::vector<const char*> cmd_line_argv_;
  std::vector<std::string> cmd_line_arguments_;
  std::string initial_path_;

 public:
  HwSpriteDrawing *AddHwSpriteDrawing() {
    hw_sprite_drawing_.emplace_back();
    return &hw_sprite_drawing_.back();
  }
  void SetArgcArgv(Si64 argc, const char **argv);
  void SetArgcArgvW(Si64 argc, const wchar_t **argv);

  void SetInitialPath(const std::string &initial_path);
  std::string GetInitialPath() const;

  Si64 GetArgc() const {
    return static_cast<Si64>(cmd_line_arguments_.size());
  }
  const char *const * GetArgv() const {
    return cmd_line_argv_.data();
  }
  void Init(Si32 width, Si32 height);
  void Draw2d();
  Sprite &GetBackbuffer() {
    return backbuffer_texture_;
  }
  HwSprite &GetHwBackbuffer() {
      return hw_backbuffer_texture_;
  }
  void ResizeBackbuffer(const Si32 width, const Si32 height);
  double GetTime();
  Si64 GetRandom(Si64 min, Si64 max);
  Ui64 GetRandom64();
  Ui32 GetRandom32();
  Ui16 GetRandom16();
  Ui8 GetRandom8();
  float GetRandomFloat23();
  float GetRandomSFloat23();
  Vec2Si32 MouseToBackbuffer(Vec2F pos) const;
  void OnWindowResize(Si32 width, Si32 height);
  Vec2Si32 GetWindowSize() const;
  void SetInverseY(bool is_inverse);
  MathTables &GetMathTables() {
    return math_tables_;
  }
  const std::shared_ptr<GlProgram> &GetDefaultSpriteProgram() const {
    return default_sprite_program_;
  }
};
/// @}

}  // namespace arctic

#endif  // ENGINE_ENGINE_H_
