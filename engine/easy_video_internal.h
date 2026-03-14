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

#ifndef ENGINE_EASY_VIDEO_INTERNAL_H_
#define ENGINE_EASY_VIDEO_INTERNAL_H_

#include "engine/arctic_types.h"
#include "engine/gl_texture2d.h"
#include "engine/gl_program.h"
#include "engine/gl_buffer.h"

namespace arctic {

/// @brief Checks input for video skip (Escape, Space, left mouse click).
/// @return true if the user wants to skip the video.
bool CheckVideoSkipInput();

/// @brief Draws a textured fullscreen quad with letterboxing.
/// @param texture The GL texture containing the current video frame.
/// @param program The shader program to use for drawing.
/// @param vbo Vertex buffer object.
/// @param ebo Element buffer object.
/// @param video_width Width of the video in pixels.
/// @param video_height Height of the video in pixels.
void DrawVideoFrame(GlTexture2D &texture, GlProgram &program,
    GlBuffer &vbo, GlBuffer &ebo,
    Si32 video_width, Si32 video_height);

}  // namespace arctic

#endif  // ENGINE_EASY_VIDEO_INTERNAL_H_
