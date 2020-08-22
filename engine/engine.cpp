// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
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

#include <sstream>

#include "engine/opengl.h"
#include "engine/engine.h"
#include "engine/log.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_math.h"
#include "engine/unicode.h"

namespace arctic {

void MathTables::Init() {
  cicrle_16_16_size_bits = 12;
  cicrle_16_16_mask = (Si32)(1 << (cicrle_16_16_size_bits - 1)) - 1;
  cicrle_16_16_half_mask = cicrle_16_16_mask / 2;
  circle_16_16.resize(size_t(1ull << (cicrle_16_16_size_bits - 1)));
  for (Si32 y = 0; y < (Si32)circle_16_16.size(); ++y) {
    double yy = static_cast<double>(y)
      / static_cast<double>(circle_16_16.size() - 1);
    double xx = std::sqrt(1.0 - yy * yy);
    Si32 x = Si32(xx * 65536.0 + 0.5);
    circle_16_16[static_cast<size_t>(y)] = x;
  }
}

GLuint Engine::LoadShader(const char *shaderSrc, GLenum type) {
  // Create the shader object
  GLuint shader = glCreateShader(type);
  if (shader == 0) {
    GLenum errCode = glGetError();
    std::stringstream info;
    info << "Can't create shader, code: " << (Ui64)errCode;
    Fatal(info.str().c_str());
    return 0;
  }
  // Load the shader source
  glShaderSource(shader, 1, &shaderSrc, NULL);
  // Compile the shader
  glCompileShader(shader);
  // Check the compile status
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
      char* infoLog = reinterpret_cast<char*>(malloc(sizeof(char) * static_cast<size_t>(infoLen)));
      glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
      Fatal("Error compiling shader: ", infoLog);
      free(infoLog); //-V779
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

void Engine::SetArgcArgv(Si64 argc, const char **argv) {
  cmd_line_arguments_.resize(static_cast<size_t>(argc));
  cmd_line_argv_.resize(static_cast<size_t>(argc));
  for (size_t i = 0; i < static_cast<size_t>(argc); ++i) {
    cmd_line_arguments_[i].assign(argv[i]);
    cmd_line_argv_[i] = cmd_line_arguments_[i].c_str();
  }
}

void Engine::SetArgcArgvW(Si64 argc, const wchar_t **argv) {
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

void Engine::Init(Si32 width, Si32 height) {
  width_ = width;
  height_ = height;

  SetVSync(true);

  ResizeBackbuffer(width, height);

  start_time_ = std::chrono::high_resolution_clock::now();
  time_correction_ = 0.0;
  last_time_ = 0.0;

  Si64 ms = start_time_.time_since_epoch().count();
  rnd_8_.seed(static_cast<Ui64>(ms));
  rnd_16_.seed(static_cast<Ui64>(ms + 1));
  rnd_32_.seed(static_cast<Ui64>(ms + 2));
  rnd_64_.seed(static_cast<Ui64>(ms + 3));

  math_tables_.Init();


  const char vShaderStr[] = R"SHADER(
#ifdef GL_ES
#endif
attribute vec4 vPosition;
attribute vec2 vTex;
varying vec2 v_texCoord;
void main() {
  gl_Position = vPosition;
  v_texCoord = vTex;
}
)SHADER";

  const char fShaderStr[] = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D s_texture;
void main() {
  gl_FragColor = texture2D(s_texture, v_texCoord);
}
)SHADER";

  // Load the vertex/fragment shaders
  GLuint vertexShader = LoadShader(vShaderStr, GL_VERTEX_SHADER);
  GLuint fragmentShader = LoadShader(fShaderStr, GL_FRAGMENT_SHADER);
  // Create the program object
  g_programObject = glCreateProgram();
  if (g_programObject == 0) {
    Fatal("Unknown error creating program");
  }
  glAttachShader(g_programObject, vertexShader);
  glAttachShader(g_programObject, fragmentShader);
  // Bind vPosition to attribute 0
  glBindAttribLocation(g_programObject, 0, "vPosition");
  glBindAttribLocation(g_programObject, 1, "vTex");
  // Link the program
  glLinkProgram(g_programObject);
  // Check the link status
  GLint linked;
  glGetProgramiv(g_programObject, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint infoLen = 0;
    glGetProgramiv(g_programObject, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
      char* infoLog = reinterpret_cast<char*>(malloc(sizeof(char) * static_cast<size_t>(infoLen)));
      glGetProgramInfoLog(g_programObject, infoLen, NULL, infoLog);
      Fatal("Error linking program: ", infoLog);
      free(infoLog); //-V779
    }
    glDeleteProgram(g_programObject);
    Fatal("Unknown error linking program");
  }
}

void Engine::Draw2d() {
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
      backbuffer_texture_.Width(), backbuffer_texture_.Height(), GL_RGBA,
      GL_UNSIGNED_BYTE, static_cast<GLvoid*>(backbuffer_texture_.RawData()));

  // render


  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  // draw quad

  visible_verts_.resize(16 << 20);
  visible_normals_.resize(16 << 20);
  visible_indices_.resize(16 << 20);
  tex_coords_.resize(16 << 20);

  verts_ = 0;
  normals_ = 0;
  tex_ = 0;
  indices_ = 0;

  Vec3F *vertex = static_cast<Vec3F*>(reinterpret_cast<void*>(
        visible_verts_.data()));
  Vec3F *normal = static_cast<Vec3F*>(reinterpret_cast<void*>(
        visible_normals_.data()));
  Vec2F *tex = static_cast<Vec2F*>(reinterpret_cast<void*>(
        tex_coords_.data()));
  Ui32 *index = static_cast<Ui32*>(reinterpret_cast<void*>(
        visible_indices_.data()));

  float aspect = static_cast<float>(width_) / static_cast<float>(height_);
  float back_aspect = static_cast<float>(backbuffer_texture_.Width()) /
    static_cast<float>(backbuffer_texture_.Height());
  float ratio = back_aspect / aspect;
  float x_aspect = aspect < back_aspect ? 1.f : ratio;
  float y_aspect = aspect < back_aspect ? 1.f / ratio : 1.f;

  Vec3F base = Vec3F(-1.f * x_aspect, -1.f * y_aspect, 0.f);
  Vec3F tx = Vec3F(2.f * x_aspect, 0.f, 0.f);
  Vec3F ty = Vec3F(0.f, 2.f * y_aspect, 0.f);
  Vec3F n = Vec3F(0.f, 0.f, 1.f);

  Si32 idx = verts_;
  vertex[verts_] = base;
  ++verts_;
  vertex[verts_] = base + tx;
  ++verts_;
  vertex[verts_] = base + ty + tx;
  ++verts_;
  vertex[verts_] = base + ty;
  ++verts_;

  normal[normals_] = n;
  ++normals_;
  normal[normals_] = n;
  ++normals_;
  normal[normals_] = n;
  ++normals_;
  normal[normals_] = n;
  ++normals_;

  tex[tex_] = Vec2F(0.0f, is_inverse_y_ ? 1.0f : 0.0f);
  ++tex_;
  tex[tex_] = Vec2F(1.0f, is_inverse_y_ ? 1.0f : 0.0f);
  ++tex_;
  tex[tex_] = Vec2F(1.0f, is_inverse_y_ ? 0.0f : 1.0f);
  ++tex_;
  tex[tex_] = Vec2F(0.0f, is_inverse_y_ ? 0.0f : 1.0f);
  ++tex_;

  index[indices_] = static_cast<Ui32>(idx);
  indices_++;
  index[indices_] = static_cast<Ui32>(idx + 1);
  indices_++;
  index[indices_] = static_cast<Ui32>(idx + 2);
  indices_++;
  index[indices_] = static_cast<Ui32>(idx + 2);
  indices_++;
  index[indices_] = static_cast<Ui32>(idx + 3);
  indices_++;
  index[indices_] = static_cast<Ui32>(idx);
  indices_++;

  glViewport(0, 0, width_, height_);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0,
      visible_verts_.data());
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0,
      tex_coords_.data());
  glEnableVertexAttribArray(1);

  GLint loc = glGetUniformLocation(g_programObject, "s_texture");
  Check(loc >= 0, "s_texture not found");
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, backbuffer_texture_name_);


  glUniform1i(loc, 0);
  glUseProgram(g_programObject);

  GLint ufs;
  glGetProgramiv(g_programObject, GL_ACTIVE_UNIFORMS, &ufs);
  Check(ufs == 1, "no ufs");

  glDrawElements(GL_TRIANGLES, indices_, GL_UNSIGNED_INT,
      visible_indices_.data());

  Swap();
}

void Engine::ResizeBackbuffer(const Si32 width, const Si32 height) {
  backbuffer_texture_.Create(width, height);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glGenTextures(1, &backbuffer_texture_name_);
  // generate a texture handler really reccomanded (mandatory in openGL 3.0)
  glBindTexture(GL_TEXTURE_2D, backbuffer_texture_name_);
  // tell openGL that we are using the texture
  Check(glIsTexture(backbuffer_texture_name_), "no texture");

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, backbuffer_texture_.RawData());
  {
    GLenum errCode = glGetError();
    *Log() << "code: " << (Ui64)errCode;
  }
  // send the texture data
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

Vec2Si32 Engine::MouseToBackbuffer(Vec2F pos) const {
  Vec2F rel_pos = pos - Vec2F(0.5f, 0.5f);
  float aspect = static_cast<float>(width_) / static_cast<float>(height_);
  float back_aspect = static_cast<float>(backbuffer_texture_.Width()) /
    static_cast<float>(backbuffer_texture_.Height());
  float ratio = back_aspect / aspect;
  float x_aspect = aspect < back_aspect ? 1.f : ratio;
  float y_aspect = aspect < back_aspect ? 1.f / ratio : 1.f;
  Vec2F cor_pos(rel_pos.x / x_aspect, rel_pos.y / y_aspect);
  Vec2F back_rel_pos = cor_pos + Vec2F(0.5f, 0.5f);
  Vec2F scale = Vec2F(backbuffer_texture_.Size());
  Vec2F back_pos(
      Clamp(back_rel_pos.x, 0.f, 1.f) * scale.x,
      Clamp(back_rel_pos.y, 0.f, 1.f) * scale.y);

  return Vec2Si32(static_cast<Si32>(back_pos.x),
      static_cast<Si32>(back_pos.y));
}

void Engine::OnWindowResize(Si32 width, Si32 height) {
  width_ = width;
  height_ = height;
}

Vec2Si32 Engine::GetWindowSize() const {
  return Vec2Si32(width_, height_);
}

void Engine::SetInverseY(bool is_inverse) {
  is_inverse_y_ = is_inverse;
}

}  // namespace arctic
