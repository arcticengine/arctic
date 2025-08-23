// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include <cstring>
#include <sstream>

#include "engine/opengl.h"
#include "engine/engine.h"
#include "engine/log.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_math.h"
#include "engine/unicode.h"
#include "engine/gl_state.h"

namespace arctic {

// Define static thread-local variables
thread_local std::independent_bits_engine<std::mt19937_64, 64, Ui64> Engine::rnd_64_;
thread_local std::independent_bits_engine<std::mt19937_64, 32, Ui64> Engine::rnd_32_;
thread_local std::independent_bits_engine<std::mt19937_64, 16, Ui64> Engine::rnd_16_;
thread_local std::independent_bits_engine<std::mt19937_64, 8, Ui64> Engine::rnd_8_;
thread_local bool Engine::is_rng_initialized_ = false;

void MathTables::Init() {
  circle_16_16_size = 4097;
  circle_16_16_one = circle_16_16_size - 1;
  circle_16_16_half = circle_16_16_one / 2;
  circle_16_16.resize(circle_16_16_size);
  for (Si32 y = 0; y < (Si32)circle_16_16.size(); ++y) {
    double yy = static_cast<double>(y) / static_cast<double>(circle_16_16_one);
    double xx = std::sqrt(1.0 - yy * yy);
    Si32 x = Si32(xx * 65536.0 );
    circle_16_16[static_cast<size_t>(y)] = x;
  }
}

void Engine::SetArgcArgv(Si32 argc, const char **argv) {
  cmd_line_arguments_.resize(static_cast<size_t>(argc));
  cmd_line_argv_.resize(static_cast<size_t>(argc));
  for (size_t i = 0; i < static_cast<size_t>(argc); ++i) {
    cmd_line_arguments_[i].assign(argv[i]);
    cmd_line_argv_[i] = cmd_line_arguments_[i].c_str();
  }
}

void Engine::SetArgcArgvW(Si32 argc, const wchar_t **argv) {
  cmd_line_arguments_.resize(static_cast<size_t>(argc));
  cmd_line_argv_.resize(static_cast<size_t>(argc));
  for (size_t i = 0; i < static_cast<size_t>(argc); ++i) {
    if (sizeof(wchar_t) == sizeof(Ui16)) {
      cmd_line_arguments_[i] = Utf16ToUtf8(
        reinterpret_cast<const void*>(argv[i]));
    } else {
      cmd_line_arguments_[i] = Utf32ToUtf8(
        reinterpret_cast<const void*>(argv[i]));
    }
    cmd_line_argv_[i] = cmd_line_arguments_[i].c_str();
  }
}

void Engine::SetInitialPath(const std::string &initial_path) {
  initial_path_ = initial_path;
}

std::string Engine::GetInitialPath() const {
  return initial_path_;
}

void Engine::InitThreadLocalRng() {
  // Get a unique seed for this thread
  Si64 ms = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  
  // Initialize thread-local random number generators with time-based seed
  rnd_8_.seed(static_cast<Ui64>(ms));
  rnd_16_.seed(static_cast<Ui64>(ms + 1));
  rnd_32_.seed(static_cast<Ui64>(ms + 2));
  rnd_64_.seed(static_cast<Ui64>(ms + 3));
  is_rng_initialized_ = true;
}

void Engine::HeadlessInit() {
  start_time_ = std::chrono::high_resolution_clock::now();
  time_correction_ = 0.0;
  last_time_ = 0.0;

  // Initialize random number generators for the main thread
  InitThreadLocalRng();

  math_tables_.Init();
}

void Engine::Init(Si32 window_width, Si32 window_height) {
  window_width_ = window_width;
  window_height_ = window_height;

  SetVSync(true);

  ResizeBackbuffer(window_width_, window_height_);

  HeadlessInit();

  const char copy_backbuffers_vShaderStr[] = R"SHADER(
#ifdef GL_ES
#endif
attribute vec3 vPosition;
attribute vec2 vTex;
varying vec2 v_texCoord;
void main() {
  gl_Position = vec4(vPosition, 1.0);
  v_texCoord = vTex;
}
)SHADER";

  const char copy_backbuffers_fShaderStr[] = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D s_texture;
void main() {
  gl_FragColor = texture2D(s_texture, v_texCoord);
}
)SHADER";

  copy_backbuffers_program_ = std::make_shared<GlProgram>();
  copy_backbuffers_program_->Create(copy_backbuffers_vShaderStr, copy_backbuffers_fShaderStr);

  const char colorize_vShaderStr[] = R"SHADER(
#ifdef GL_ES
#endif
attribute vec3 vPosition;
attribute vec2 vTex;
varying vec2 v_texCoord;
void main() {
  gl_Position = vec4(vPosition, 1.0);
  v_texCoord = vTex;
}
)SHADER";

const char colorize_fShaderStr[] = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D s_texture;
uniform vec4 in_color;
void main() {
  vec4 c = texture2D(s_texture, v_texCoord);
  float ca = c.a * in_color.a;
  vec4 res;
  res.rgb = c.rgb * in_color.rgb;
  res.a = ca;
  gl_FragColor = res;
}
)SHADER";

  colorize_program_ = std::make_shared<GlProgram>();
  colorize_program_->Create(colorize_vShaderStr, colorize_fShaderStr);

  const char solid_color_vShaderStr[] = R"SHADER(
#ifdef GL_ES
#endif
attribute vec3 vPosition;
attribute vec2 vTex;
varying vec2 v_texCoord;
void main() {
  gl_Position = vec4(vPosition, 1.0);
  v_texCoord = vTex;
}
)SHADER";

const char solid_color_fShaderStr[] = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform vec4 in_color;
void main() {
  gl_FragColor = in_color;
}
)SHADER";

  solid_color_program_ = std::make_shared<GlProgram>();
  solid_color_program_->Create(solid_color_vShaderStr, solid_color_fShaderStr);

  const char default_sprite_vShaderStr[] = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
attribute vec2 vPosition;
attribute vec2 vTex;
varying vec2 v_texCoord;
uniform vec4 pivot_scale;
uniform vec2 to_sprite_size;
void main() {
  vec2 position = vPosition;
  position *= pivot_scale.zw;
  position += pivot_scale.xy;
  position *= vec2(2.0 / to_sprite_size.x, 2.0 / to_sprite_size.y);
  position -= vec2(1.0, 1.0);
  gl_Position = vec4(position, 0.0, 1.0);

  v_texCoord = vTex;
}
)SHADER";

  const char default_sprite_fShaderStr[] = R"SHADER(
#ifdef GL_ES
precision lowp float;
#endif
varying vec2 v_texCoord;
uniform sampler2D s_texture;
uniform vec4 in_color;
uniform int is_solid_color;
void main() {
  if (is_solid_color == 1) {
    gl_FragColor.rgb = in_color.rgb;
    gl_FragColor.a = texture2D(s_texture, v_texCoord).a*in_color.a;
  } else {
    gl_FragColor = texture2D(s_texture, v_texCoord)*in_color;
  }
}
)SHADER";

  default_sprite_program_ = std::make_shared<GlProgram>();
  default_sprite_program_->Create(default_sprite_vShaderStr, default_sprite_fShaderStr);

  vbo_.Create();
  ebo_.Create();
}

struct Vertex {
  Vec3F pos;
  Vec3F normal;
  Vec2F tex;

  Vertex() = default;
  Vertex(Vec3F in_pos, Vec3F in_normal, Vec2F in_tex)
      : pos(in_pos)
      , normal(in_normal)
      , tex(in_tex) {
  }
};

void Engine::Draw2d() {
  gl_backbuffer_texture_.UpdateData(backbuffer_texture_.RawData());

  // render

  GlFramebuffer::BindDefault();
  GlState::SetViewport(0, 0, window_width_, window_height_);

  glDisable(GL_SCISSOR_TEST);
  ARCTIC_GL_CHECK_ERROR(glClearColor(0.f, 0.f, 0.f, 0.f));
  ARCTIC_GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT));
  // draw quad

  if (mesh_.mVertexData.mNumVertexArrays == 0) {
    MeshVertexFormat vf;
    // Vertex coordinates
    vf.mElems[0].mNormalize = false;
    vf.mElems[0].mNumComponents = 3;
    vf.mElems[0].mOffset = 0;
    vf.mElems[0].mType = kRMVEDT_Float;
    // Vertex normal
    vf.mElems[1].mNormalize = true;
    vf.mElems[1].mNumComponents = 3;
    vf.mElems[1].mOffset = 3*4;
    vf.mElems[1].mType = kRMVEDT_Float;
    // Vertex texture coordinates
    vf.mElems[2].mNormalize = false;
    vf.mElems[2].mNumComponents = 2;
    vf.mElems[2].mOffset = 3*4+3*4;
    vf.mElems[2].mType = kRMVEDT_Float;
    vf.mNumElems = 3;
    vf.mStride = 3*4+3*4+2*4;
    vf.mDivisor = 0;

    mesh_.Init(1, 16 << 20, &vf, kRMVEDT_Polys, 1, 16 << 20);
  }
  mesh_.ClearGeometry();

  float aspect = static_cast<float>(window_width_) / static_cast<float>(window_height_);
  float back_aspect = static_cast<float>(backbuffer_texture_.Width()) /
    static_cast<float>(backbuffer_texture_.Height());
  float ratio = back_aspect / aspect;
  float x_aspect = aspect < back_aspect ? 1.f : ratio;
  float y_aspect = aspect < back_aspect ? 1.f / ratio : 1.f;

  Vec3F base = Vec3F(-1.f * x_aspect, -1.f * y_aspect, 0.f);
  Vec3F tx = Vec3F(2.f * x_aspect, 0.f, 0.f);
  Vec3F ty = Vec3F(0.f, 2.f * y_aspect, 0.f);
  Vec3F n = Vec3F(0.f, 0.f, 1.f);




  glScissor(
    GLint((1.f + base.x)*0.5f*window_width_),
    GLint((1.f + base.y)*0.5f*window_height_),
    GLsizei(ceilf((tx.x)*0.5f*window_width_)),
    GLsizei(ceilf((ty.y)*0.5f*window_height_)));
  glEnable(GL_SCISSOR_TEST);

  size_t first_idx = 0;
  size_t idx = 0;
  bool do_draw = false;
  GlTexture2D* first_texture = nullptr;
  DrawBlendingMode first_blending_mode = kDrawBlendingModeCopyRgba;
  Rgba first_in_color = Rgba(0, 0, 0, 0);
  mesh_.ClearGeometry();

  while(idx < hw_sprite_drawing_.size()) {
    HwSpriteDrawing &h = hw_sprite_drawing_[idx];
    GlTexture2D *texture = &h.from_sprite.sprite_instance()->texture();
    if (first_texture != texture || first_blending_mode != h.blending_mode || first_in_color != h.in_color) {
      if (first_idx != idx) {
        do_draw = true;
      } else {
        first_texture = texture;
        first_blending_mode = h.blending_mode;
        first_in_color = h.in_color;
      }
    }

    if (!do_draw) {
      Vec2F pivot = Vec2F(h.to_x_pivot, h.to_y_pivot);
      float sin_a = sinf(h.angle_radians);
      float cos_a = cosf(h.angle_radians);
      Vec2F left = Vec2F(-cos_a, -sin_a) *
        static_cast<float>(h.from_sprite.Pivot().x) * h.to_width / h.from_width;
      Vec2F right = Vec2F(cos_a, sin_a) *
        static_cast<float>(h.from_sprite.Width() - h.from_sprite.Pivot().x) * h.to_width / h.from_width;
      Vec2F up = Vec2F(-sin_a, cos_a) *
        static_cast<float>(h.from_sprite.Height() - h.from_sprite.Pivot().y)* h.to_height / h.from_height;
      Vec2F down = Vec2F(sin_a, -cos_a) *
        static_cast<float>(h.from_sprite.Pivot().y) * h.to_height / h.from_height;

      // d c
      // a b
      Vec2F a(pivot + left + down);
      Vec2F b(pivot + right + down);
      Vec2F c(pivot + right + up);
      Vec2F d(pivot + left + up);

      Vec3F xm = tx / static_cast<float>(backbuffer_texture_.Width());
      Vec3F ym = ty / static_cast<float>(backbuffer_texture_.Height());

      int vertex_id = mesh_.mVertexData.mVertexArray[0].mNum;
      Vertex v;
      v = Vertex(
          base 
          + a.x * xm
          + a.y * ym,
          n, Vec2F(0.0f, is_inverse_y_ ? 1.0f : 0.0f));
      mesh_.SetVertex(0, vertex_id, &v);
      v = Vertex(
          base 
          + b.x * xm
          + b.y * ym,
          n, Vec2F(1.0f, is_inverse_y_ ? 1.0f : 0.0f));
      mesh_.SetVertex(0, vertex_id + 1, &v);
      v = Vertex(
          base 
          + c.x * xm
          + c.y * ym,
          n, Vec2F(1.0f, is_inverse_y_ ? 0.0f : 1.0f));
      mesh_.SetVertex(0, vertex_id + 2, &v);
      v = Vertex(
          base 
          + d.x * xm
          + d.y * ym,
          n, Vec2F(0.0f, is_inverse_y_ ? 0.0f : 1.0f));
      mesh_.SetVertex(0, vertex_id + 3, &v);
      mesh_.mVertexData.mVertexArray[0].mNum += 4;

      int triangle_id = mesh_.mFaceData.mIndexArray[0].mNum;
      mesh_.SetTriangle(0, triangle_id, vertex_id + 0, vertex_id + 1, vertex_id + 2);
      mesh_.SetTriangle(0, triangle_id + 1, vertex_id + 2, vertex_id + 3, vertex_id + 0);
      mesh_.mFaceData.mIndexArray[0].mNum += 2;

      ++idx;
      if (idx == hw_sprite_drawing_.size()) {
        do_draw = true;
      }
    }
    if (do_draw) {
      do_draw = false;
      if (mesh_.mFaceData.mIndexArray[0].mNum) {
        vbo_.Bind(GL_ARRAY_BUFFER);

        // 1. Bind vertex buffer, upload the data
        vbo_.SetData(mesh_.mVertexData.mVertexArray[0].mBuffer,
                    mesh_.mVertexData.mVertexArray[0].mNum * mesh_.mVertexData.mVertexArray[0].mFormat.mStride);

        // 2. Bind index buffer
        ebo_.Bind(GL_ELEMENT_ARRAY_BUFFER);
        ebo_.SetData(mesh_.mFaceData.mIndexArray[0].mBuffer[0].mIndex,
                    mesh_.mFaceData.mIndexArray[0].mNum * 3 * sizeof(GLuint));

        // 3. Set vertex attributes using offsets within VBO
        ARCTIC_GL_CHECK_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            mesh_.mVertexData.mVertexArray[0].mFormat.mStride,
            (const GLvoid*)(uintptr_t)mesh_.mVertexData.mVertexArray[0].mFormat.mElems[0].mOffset));
        ARCTIC_GL_CHECK_ERROR(glEnableVertexAttribArray(0));
        ARCTIC_GL_CHECK_ERROR(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
            mesh_.mVertexData.mVertexArray[0].mFormat.mStride,
            (const GLvoid*)(uintptr_t)mesh_.mVertexData.mVertexArray[0].mFormat.mElems[2].mOffset));
        ARCTIC_GL_CHECK_ERROR(glEnableVertexAttribArray(1));

        GlProgram *program = copy_backbuffers_program_.get();
        if (first_blending_mode == kDrawBlendingModeColorize) {
          program = colorize_program_.get();
        } else if (first_blending_mode == kDrawBlendingModeSolidColor) {
          program = solid_color_program_.get();
        }
        program->Bind();

        if (first_blending_mode == kDrawBlendingModeSolidColor) {
          program->SetUniform("in_color", Vec4F(first_in_color.r*(1.f/255.f),
                                                first_in_color.g*(1.f/255.f),
                                                first_in_color.b*(1.f/255.f),
                                                first_in_color.a*(1.f/255.f)));
          program->CheckActiveUniforms(1);
        } else {
          program->SetUniform("s_texture", 0);
          if (first_blending_mode == kDrawBlendingModeColorize) {
            program->SetUniform("in_color", Vec4F(first_in_color.r*(1.f/255.f),
                                                  first_in_color.g*(1.f/255.f),
                                                  first_in_color.b*(1.f/255.f),
                                                  first_in_color.a*(1.f/255.f)));
            program->CheckActiveUniforms(2);
          } else {
            program->CheckActiveUniforms(1);
          }
        }

        GlState::SetBlending(first_blending_mode);
        first_texture->Bind(0);

        // 4. Draw (with index buffer bound)
        ARCTIC_GL_CHECK_ERROR(glDrawElements(GL_TRIANGLES,
              mesh_.mFaceData.mIndexArray[0].mNum * 3,
              GL_UNSIGNED_INT,
              0));  // Offset into the bound index buffer
        first_idx = idx;
        mesh_.ClearGeometry();
      }
    }
  }
  hw_sprite_drawing_.clear();

  if (is_sw_renderer_enabled_) {
    mesh_.mVertexData.mVertexArray[0].mNum = 4;
    Vertex v;
    v = Vertex(base, n, Vec2F(0.0f, is_inverse_y_ ? 1.0f : 0.0f));
    mesh_.SetVertex(0, 0, &v);
    v = Vertex(base + tx, n, Vec2F(1.0f, is_inverse_y_ ? 1.0f : 0.0f));
    mesh_.SetVertex(0, 1, &v);
    v = Vertex(base + ty + tx, n, Vec2F(1.0f, is_inverse_y_ ? 0.0f : 1.0f));
    mesh_.SetVertex(0, 2, &v);
    v = Vertex(base + ty, n, Vec2F(0.0f, is_inverse_y_ ? 0.0f : 1.0f));
    mesh_.SetVertex(0, 3, &v);

    mesh_.mFaceData.mIndexArray[0].mNum = 2;
    mesh_.SetTriangle(0, 0, 0, 1, 2);
    mesh_.SetTriangle(0, 1, 2, 3, 0);

    // Create and bind VBO

    vbo_.Bind(GL_ARRAY_BUFFER);

    // 1. Bind vertex buffer, upload the data
    vbo_.SetData(mesh_.mVertexData.mVertexArray[0].mBuffer,
                mesh_.mVertexData.mVertexArray[0].mNum * mesh_.mVertexData.mVertexArray[0].mFormat.mStride);

    // 2. Bind index buffer
    ebo_.Bind(GL_ELEMENT_ARRAY_BUFFER);
    ebo_.SetData(mesh_.mFaceData.mIndexArray[0].mBuffer[0].mIndex,
                mesh_.mFaceData.mIndexArray[0].mNum * 3 * sizeof(GLuint));

    // 3. Set vertex attributes using offsets within VBO
    ARCTIC_GL_CHECK_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        mesh_.mVertexData.mVertexArray[0].mFormat.mStride,
        (const GLvoid*)(uintptr_t)mesh_.mVertexData.mVertexArray[0].mFormat.mElems[0].mOffset));
    ARCTIC_GL_CHECK_ERROR(glEnableVertexAttribArray(0));

    ARCTIC_GL_CHECK_ERROR(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        mesh_.mVertexData.mVertexArray[0].mFormat.mStride,
        (const GLvoid*)(uintptr_t)mesh_.mVertexData.mVertexArray[0].mFormat.mElems[2].mOffset));
    ARCTIC_GL_CHECK_ERROR(glEnableVertexAttribArray(1));

    copy_backbuffers_program_->Bind();
    copy_backbuffers_program_->SetUniform("s_texture", 0);
    copy_backbuffers_program_->CheckActiveUniforms(1);

    GlState::SetBlending(kDrawBlendingModePremultipliedAlphaBlend);
  
    gl_backbuffer_texture_.Bind(0);
    // 4. Draw (with index buffer bound)
    ARCTIC_GL_CHECK_ERROR(glDrawElements(GL_TRIANGLES,
                  mesh_.mFaceData.mIndexArray[0].mNum * 3,
                  GL_UNSIGNED_INT,
                  0));  // Offset into the bound index buffer
  }

  Swap();
}

void Engine::ResizeBackbuffer(const Si32 width, const Si32 height) {
  hw_backbuffer_texture_.Create(width, height);
  backbuffer_texture_.Create(width, height);

  gl_backbuffer_texture_.Create(width, height);
}

double Engine::GetTime() {
  auto now = std::chrono::high_resolution_clock::now();
  if (now > start_time_) {
    double duration = std::chrono::duration<double, std::ratio<1>>(
      now - start_time_).count();
    double time = duration + time_correction_;
    if (time >= last_time_) {
      last_time_ = time;
      return time;
    }
    time_correction_ = last_time_ - duration;
    return last_time_;
  }
  start_time_ = now;
  time_correction_ = last_time_;
  return last_time_;
}

Si64 Engine::GetRandom(Si64 min, Si64 max) {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  Check(min <= max, "GetRandom min should be <= max");
  Ui64 range = static_cast<Ui64>(max - min) + 1ull;
  if (range < 0x1000) {
    if (range < 0x10) {
      return min + static_cast<Si64>(rnd_8_() % range);
    } else {
      return min + static_cast<Si64>(rnd_16_() % range);
    }
  } else {
    if (range < 0x1000000) {
      return min + static_cast<Si64>(rnd_32_() % range);
    } else {
      return min + static_cast<Si64>(rnd_64_() % range);
    }
  }
}


Ui64 Engine::GetRandom64() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  return rnd_64_();
}

Ui32 Engine::GetRandom32() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  return static_cast<Ui32>(rnd_32_());
}

Ui16 Engine::GetRandom16() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  return static_cast<Ui16>(rnd_16_());
}

Ui8 Engine::GetRandom8() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  return static_cast<Ui8>(rnd_8_());
}

float Engine::GetRandomF() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  Ui32 a = static_cast<Ui32>(rnd_32_());
  a = (a>>9) | 0x3f800000;
  float res;
  memcpy(&res, &a, sizeof(float));
  res -= 1.0f;
  return res;
}

float Engine::GetRandomSF() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  Ui32 a = static_cast<Ui32>(rnd_32_());
  a = (a>>9) | 0x40000000;
  float res;
  memcpy(&res, &a, sizeof(float));
  res -= 3.0f;
  return res;
}

double Engine::GetRandomD() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  Ui64 a = static_cast<Ui64>(rnd_64_());
  a = (a>>12) | 0x3ff0000000000000;
  double res;
  memcpy(&res, &a, sizeof(double));
  res -= 1.0;
  return res;
}

double Engine::GetRandomSD() {
  if (!is_rng_initialized_) {
    InitThreadLocalRng();
  }
  Ui64 a = static_cast<Ui64>(rnd_64_());
  a = (a>>12) | 0x4000000000000000;
  double res;
  memcpy(&res, &a, sizeof(double));
  res -= 3.0;
  return res;
}

Vec2Si32 Engine::MouseToBackbuffer(Vec2F pos) const {
  Vec2F rel_pos = pos - Vec2F(0.5f, 0.5f);
  float aspect = static_cast<float>(window_width_) / static_cast<float>(window_height_);
  float back_aspect = static_cast<float>(backbuffer_texture_.Width()) /
    static_cast<float>(backbuffer_texture_.Height());
  float ratio = back_aspect / aspect;
  float x_aspect = aspect < back_aspect ? 1.f : ratio;
  float y_aspect = aspect < back_aspect ? 1.f / ratio : 1.f;
  Vec2F cor_pos(rel_pos.x / x_aspect, rel_pos.y / y_aspect);
  Vec2F back_rel_pos = cor_pos + Vec2F(0.5f, 0.5f);
  Vec2Si32 back_size = backbuffer_texture_.Size();
  Vec2F scale = Vec2F(backbuffer_texture_.Size());
  Vec2F back_pos(
      Clamp(back_rel_pos.x, 0.f, 1.f) * scale.x,
      Clamp(back_rel_pos.y, 0.f, 1.f) * scale.y);

  Vec2Si32 int_pos = Vec2Si32(static_cast<Si32>(back_pos.x), static_cast<Si32>(back_pos.y));
  int_pos = Clamp(int_pos, Vec2Si32(0, 0), Vec2Si32(back_size.x - 1, back_size.y - 1));
  return int_pos;
}

void Engine::OnWindowResize(Si32 width, Si32 height) {
  window_width_ = width;
  window_height_ = height;
}

Vec2Si32 Engine::GetWindowSize() const {
  return Vec2Si32(window_width_, window_height_);
}

void Engine::SetInverseY(bool is_inverse) {
  is_inverse_y_ = is_inverse;
}

}  // namespace arctic
