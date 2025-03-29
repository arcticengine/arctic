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

#include "engine/gl_buffer.h"

#include <sstream>
#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgba.h"
#include "engine/opengl.h"

namespace arctic {

GLuint GlBuffer::current_buffer_id_ = 0;

GlBuffer::GlBuffer() : buffer_id_(0), size_(0), target_(0) {
}

GlBuffer::~GlBuffer() {
    ARCTIC_GL_CHECK_ERROR(glDeleteBuffers(1, &buffer_id_));
}

void GlBuffer::Create() {
    ARCTIC_GL_CHECK_ERROR(glDeleteBuffers(1, &buffer_id_));
    ARCTIC_GL_CHECK_ERROR(glGenBuffers(1, &buffer_id_));
}

void GlBuffer::Bind(GLenum target) {
  if (current_buffer_id_ != buffer_id_ || target != target_) {
    target_ = target;
    current_buffer_id_ = buffer_id_;
    ARCTIC_GL_CHECK_ERROR(glBindBuffer(target_, buffer_id_));
  }
}

void GlBuffer::SetData(const void *data, size_t size) {
    Bind(target_);
    ARCTIC_GL_CHECK_ERROR(glBufferData(target_, size, data, GL_STATIC_DRAW));
    size_ = size;
}

void GlBuffer::UpdateData(const void *data) {
    Bind(target_);
    ARCTIC_GL_CHECK_ERROR(glBufferSubData(target_, 0, size_, data));
}

void GlBuffer::BindDefault(GLenum target) {
    if (current_buffer_id_ != 0) {
        current_buffer_id_ = 0;
        ARCTIC_GL_CHECK_ERROR(glBindBuffer(target, 0));
    }
}

}  // namespace arctic
