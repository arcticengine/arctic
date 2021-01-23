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

#include "engine/gl_framebuffer.h"

#include <sstream>
#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgba.h"
#include "engine/opengl.h"

namespace arctic {


GLFramebuffer::GLFramebuffer()
    : framebuffer_id_(0) {
}

GLFramebuffer::~GLFramebuffer() {
    ARCTIC_GL_CALL(glDeleteFramebuffers(1, &framebuffer_id_));
}

void GLFramebuffer::Create(GLTexture2D &texture) {
    if (framebuffer_id_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_id_);
    }

    ARCTIC_GL_CALL(glGenFramebuffers(1, &framebuffer_id_));
    ARCTIC_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_));
    ARCTIC_GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.texture_id(), 0));
    ARCTIC_GL_CALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    ARCTIC_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void GLFramebuffer::Bind() {
    ARCTIC_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_));
}

void GLFramebuffer::BindDefault() {
    ARCTIC_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}


}  // namespace arctic
