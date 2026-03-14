// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2026 Huldra
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

#include "engine/easy_video.h"
#include "engine/easy_video_internal.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "engine/opengl.h"
#include "engine/engine.h"
#include "engine/easy_advanced.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_input.h"
#include "engine/gl_texture2d.h"
#include "engine/gl_framebuffer.h"
#include "engine/gl_program.h"
#include "engine/gl_buffer.h"
#include "engine/gl_state.h"
#include "engine/log.h"

namespace arctic {

bool CheckVideoSkipInput() {
  InputMessage msg;
  while (PopInputMessage(&msg)) {
    if (msg.kind == InputMessage::kKeyboard && msg.keyboard.key_state == 1) {
      if (msg.keyboard.key == kKeyEscape || msg.keyboard.key == kKeySpace) {
        return true;
      }
    }
    if (msg.kind == InputMessage::kMouse && msg.keyboard.key_state == 1) {
      if (msg.keyboard.key == kKeyMouseLeft) {
        return true;
      }
    }
  }
  return false;
}

void DrawVideoFrame(GlTexture2D &texture, GlProgram &program,
    GlBuffer &vbo, GlBuffer &ebo,
    Si32 video_width, Si32 video_height) {
  Engine *engine = GetEngine();
  Vec2Si32 win = engine->GetWindowSize();
  Si32 window_width = win.x;
  Si32 window_height = win.y;

  GlFramebuffer::BindDefault();
  GlState::SetViewport(0, 0, window_width, window_height);

  glDisable(GL_SCISSOR_TEST);
  ARCTIC_GL_CHECK_ERROR(glClearColor(0.f, 0.f, 0.f, 1.f));
  ARCTIC_GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT));

  float aspect = static_cast<float>(std::max(1, window_width)) /
                 static_cast<float>(std::max(1, window_height));
  float video_aspect = static_cast<float>(std::max(1, video_width)) /
                       static_cast<float>(std::max(1, video_height));
  float ratio = video_aspect / aspect;
  float x_scale = aspect < video_aspect ? 1.f : ratio;
  float y_scale = aspect < video_aspect ? 1.f / ratio : 1.f;

  struct VideoVertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
  };

  VideoVertex vertices[4] = {
    { -x_scale, -y_scale, 0.f,  0.f, 0.f, 1.f,  0.f, 1.f },
    {  x_scale, -y_scale, 0.f,  0.f, 0.f, 1.f,  1.f, 1.f },
    {  x_scale,  y_scale, 0.f,  0.f, 0.f, 1.f,  1.f, 0.f },
    { -x_scale,  y_scale, 0.f,  0.f, 0.f, 1.f,  0.f, 0.f },
  };

  GLuint indices[6] = { 0, 1, 2, 2, 3, 0 };

  vbo.Bind(GL_ARRAY_BUFFER);
  vbo.SetData(vertices, sizeof(vertices));

  ebo.Bind(GL_ELEMENT_ARRAY_BUFFER);
  ebo.SetData(indices, sizeof(indices));

  Si32 stride = sizeof(VideoVertex);
  ARCTIC_GL_CHECK_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
      stride, (const GLvoid *)0));
  ARCTIC_GL_CHECK_ERROR(glEnableVertexAttribArray(0));

  ARCTIC_GL_CHECK_ERROR(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
      stride, (const GLvoid *)(6 * sizeof(float))));
  ARCTIC_GL_CHECK_ERROR(glEnableVertexAttribArray(1));

  program.Bind();
  program.SetUniform("s_texture", 0);

  GlState::SetBlending(kDrawBlendingModeCopyRgba);
  texture.Bind(0);
  texture.SetFilterMode(kFilterBilinear);

  ARCTIC_GL_CHECK_ERROR(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
}

bool PlayFullscreenVideo(const std::string &file_name) {
  return PlayFullscreenVideo(file_name.c_str());
}

}  // namespace arctic
