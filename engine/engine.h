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
  Si32 cicrle_16_16_size = 0;
  Si32 cicrle_16_16_one = 0;
  Si32 cicrle_16_16_half = 0;

  void Init();
};

class Engine {
 private:
  Si32 window_width_ = 0;
  Si32 window_height_ = 0;
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
  bool is_sw_renderer_enabled_ = true;

  MathTables math_tables_;

  std::shared_ptr<GlProgram> copy_backbuffers_program_;
  std::shared_ptr<GlProgram> colorize_program_;
  std::shared_ptr<GlProgram> solid_color_program_;
  std::shared_ptr<GlProgram> default_sprite_program_;

  std::vector<const char*> cmd_line_argv_;
  std::vector<std::string> cmd_line_arguments_;
  std::string initial_path_;

 public:
  /// @brief Adds a new HwSpriteDrawing to the engine.
  /// @return Pointer to the newly added HwSpriteDrawing.
  HwSpriteDrawing *AddHwSpriteDrawing() {
    hw_sprite_drawing_.emplace_back();
    return &hw_sprite_drawing_.back();
  }
  void SetArgcArgv(Si32 argc, const char **argv);

  /// @brief Sets the command line arguments for the engine (wide character version).
  /// @param argc The number of command line arguments.
  /// @param argv The array of wide character command line argument strings.
  void SetArgcArgvW(Si32 argc, const wchar_t **argv);

  /// @brief Sets the initial path for the engine.
  /// @param initial_path The initial path to set.
  void SetInitialPath(const std::string &initial_path);

  /// @brief Gets the initial path set for the engine.
  /// @return The initial path as a string.
  std::string GetInitialPath() const;

  /// @brief Gets the number of command line arguments.
  /// @return The number of command line arguments. 
  Si32 GetArgc() const {
    return static_cast<Si32>(cmd_line_arguments_.size());
  }

  /// @brief Gets the array of command line argument strings.
  /// @return Pointer to the array of command line argument strings.
  const char *const * GetArgv() const {
    return cmd_line_argv_.data();
  }

  /// @brief Initializes the engine for headless mode.
  void HeadlessInit();

  /// @brief Initializes the engine with the specified width and height.
  /// @param width The width of the engine window.
  /// @param height The height of the engine window.
  void Init(Si32 width, Si32 height);

  /// @brief Performs 2D drawing operations.
  void Draw2d();

  /// @brief Gets the backbuffer sprite.
  /// @return Reference to the backbuffer sprite.
  Sprite &GetBackbuffer() {
    return backbuffer_texture_;
  }

  /// @brief Gets the hardware backbuffer sprite.
  /// @return Reference to the hardware backbuffer sprite.
  HwSprite &GetHwBackbuffer() {
      return hw_backbuffer_texture_;
  }

  /// @brief Resizes the backbuffer to the specified dimensions.
  /// @param width The new width of the backbuffer.
  /// @param height The new height of the backbuffer.
  void ResizeBackbuffer(const Si32 width, const Si32 height);

  /// @brief Gets the current time.
  /// @return The current time as a double.
  double GetTime();

  /// @brief Generates a random integer within the specified range.
  /// @param min The minimum value of the range (inclusive).
  /// @param max The maximum value of the range (inclusive).
  /// @return A random integer within the specified range.
  Si64 GetRandom(Si64 min, Si64 max);

  /// @brief Generates a random 64-bit unsigned integer.
  /// @return A random 64-bit unsigned integer.
  Ui64 GetRandom64();

  /// @brief Generates a random 32-bit unsigned integer.
  /// @return A random 32-bit unsigned integer.
  Ui32 GetRandom32();

  /// @brief Generates a random 16-bit unsigned integer.
  /// @return A random 16-bit unsigned integer.
  Ui16 GetRandom16();

  /// @brief Generates a random 8-bit unsigned integer.
  /// @return A random 8-bit unsigned integer.
  Ui8 GetRandom8();

  /// @brief Generates a random float in the range [0, 1) with 32-bit precision.
  /// @return A random float in the range [0, 1).
  float GetRandomF();

  /// @brief Generates a random signed float in the range [-1, 1) with 32-bit precision.
  /// @return A random signed float in the range [-1, 1).
  float GetRandomSF();

  /// @brief Generates a random double in the range [0, 1) with 64-bit precision.
  /// @return A random double in the range [0, 1).
  double GetRandomD();

  /// @brief Generates a random signed double in the range [-1, 1) with 64-bit precision.
  /// @return A random signed double in the range [-1, 1).
  double GetRandomSD();

  /// @brief Converts mouse coordinates to backbuffer coordinates.
  /// @param pos The mouse position as a Vec2F.
  /// @return The corresponding position in backbuffer coordinates as a Vec2Si32.
  Vec2Si32 MouseToBackbuffer(Vec2F pos) const;

  /// @brief Handles window resize events.
  /// @param width The new width of the window.
  /// @param height The new height of the window.
  void OnWindowResize(Si32 width, Si32 height);

  /// @brief Gets the current window size.
  /// @return The current window size as a Vec2Si32.
  Vec2Si32 GetWindowSize() const;

  /// @brief Sets the inverse Y flag.
  /// @param is_inverse True to set inverse Y, false otherwise.
  void SetInverseY(bool is_inverse);

  /// @brief Gets the math tables used by the engine.
  /// @return Reference to the MathTables object.
  MathTables &GetMathTables() {
    return math_tables_;
  }

  /// @brief Gets the default sprite program.
  /// @return Const reference to the shared pointer of the default GlProgram.
  const std::shared_ptr<GlProgram> &GetDefaultSpriteProgram() const {
    return default_sprite_program_;
  }
};
/// @}

}  // namespace arctic

#endif  // ENGINE_ENGINE_H_
