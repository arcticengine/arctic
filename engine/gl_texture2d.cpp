// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
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

#include "engine/gl_texture2d.h"

#include <sstream>
#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgba.h"
#include "engine/opengl.h"

namespace arctic {


GLTexture2D::GLTexture2D()
    : width_(0)
      , height_(0)
      , texture_id_(0) {
}

GLTexture2D::~GLTexture2D() {
    ARCTIC_GL_CALL(glDeleteTextures(1, &texture_id_));
}

void GLTexture2D::Create(Si32 w, Si32 h) {
    width_ = w;
    height_ = h;

    if (texture_id_ != 0) {
        ARCTIC_GL_CALL(glDeleteTextures(1, &texture_id_));
    }

    ARCTIC_GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    ARCTIC_GL_CALL(glPixelStorei(GL_PACK_ALIGNMENT, 1));
    ARCTIC_GL_CALL(glGenTextures(1, &texture_id_));
    ARCTIC_GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_id_));
    Check(glIsTexture(texture_id_), "no texture");

    ARCTIC_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    ARCTIC_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    ARCTIC_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    ARCTIC_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    ARCTIC_GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, nullptr));
}

void GLTexture2D::Bind(Ui32 slot) const {
    ARCTIC_GL_CALL(glActiveTexture(GL_TEXTURE0 + slot));
    ARCTIC_GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_id_));
}

void GLTexture2D::SetData(const void *data, Si32 w, Si32 h) {
    width_ = w;
    height_ = h;

    ARCTIC_GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_id_));
    ARCTIC_GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
}

void GLTexture2D::UpdateData(const void *data) {
    ARCTIC_GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_id_));
    ARCTIC_GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, data));
}

void GLTexture2D::ReadData(void *dst) const {
    ARCTIC_GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_id_));
    ARCTIC_GL_CALL(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst));
}


}  // namespace arctic
