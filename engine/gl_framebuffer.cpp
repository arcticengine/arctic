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

GLuint GlFramebuffer::current_framebuffer_id_ = 0;

GlFramebuffer::GlFramebuffer()
    : framebuffer_id_(0), depth_renderbuffer_id_(0) {
}

GlFramebuffer::~GlFramebuffer() {
    if (current_framebuffer_id_ == framebuffer_id_) {
        current_framebuffer_id_ = 0;
    }
    if (depth_renderbuffer_id_ != 0) {
        ARCTIC_GL_CHECK_ERROR(glDeleteRenderbuffers(1, &depth_renderbuffer_id_));
    }
    ARCTIC_GL_CHECK_ERROR(glDeleteFramebuffers(1, &framebuffer_id_));
}

void GlFramebuffer::Create(GlTexture2D &texture) {
    if (framebuffer_id_ != 0) {
        if (current_framebuffer_id_ == framebuffer_id_) {
            current_framebuffer_id_ = 0;
        }
        ARCTIC_GL_CHECK_ERROR(glDeleteFramebuffers(1, &framebuffer_id_));
    }

    ARCTIC_GL_CHECK_ERROR(glGenFramebuffers(1, &framebuffer_id_));
    Bind();
    ARCTIC_GL_CHECK_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.texture_id(), 0));
    auto code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (code != GL_FRAMEBUFFER_COMPLETE) {
      current_framebuffer_id_ = 0;
      ARCTIC_GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
      ARCTIC_GL_CHECK_ERROR(glDeleteFramebuffers(1, &framebuffer_id_));
      framebuffer_id_ = 0;
      switch(code) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
          Fatal("GlFramebuffer::Create failed: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
          Fatal("GlFramebuffer::Create failed: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
          break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
          Fatal("GlFramebuffer::Create failed: GL_FRAMEBUFFER_UNSUPPORTED");
          break;
        default:
          Fatal("GlFramebuffer::Create failed, error code: ",
            std::to_string(code).c_str());
          break;
      }
    }
}

void GlFramebuffer::CreateDepthOnly(GlTexture2D &depth_texture) {
    if (framebuffer_id_ != 0) {
        if (current_framebuffer_id_ == framebuffer_id_) {
            current_framebuffer_id_ = 0;
        }
        ARCTIC_GL_CHECK_ERROR(glDeleteFramebuffers(1, &framebuffer_id_));
    }

    ARCTIC_GL_CHECK_ERROR(glGenFramebuffers(1, &framebuffer_id_));
    Bind();
    ARCTIC_GL_CHECK_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture.texture_id(), 0));
#if !defined(ARCTIC_PLATFORM_PI_ES_EGL) && !defined(ARCTIC_PLATFORM_WEB)
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
#endif
    auto code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (code != GL_FRAMEBUFFER_COMPLETE) {
        current_framebuffer_id_ = 0;
        ARCTIC_GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        ARCTIC_GL_CHECK_ERROR(glDeleteFramebuffers(1, &framebuffer_id_));
        framebuffer_id_ = 0;
        Fatal("GlFramebuffer::CreateDepthOnly failed, error code: ",
            std::to_string(code).c_str());
    }
}

void GlFramebuffer::AttachDepthBuffer(Si32 width, Si32 height) {
    if (depth_renderbuffer_id_ != 0) {
        ARCTIC_GL_CHECK_ERROR(glDeleteRenderbuffers(1, &depth_renderbuffer_id_));
        depth_renderbuffer_id_ = 0;
    }
    Bind();
    ARCTIC_GL_CHECK_ERROR(glGenRenderbuffers(1, &depth_renderbuffer_id_));
    ARCTIC_GL_CHECK_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_id_));
    ARCTIC_GL_CHECK_ERROR(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height));
    ARCTIC_GL_CHECK_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_id_));
    ARCTIC_GL_CHECK_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

void GlFramebuffer::Bind() {
    if (current_framebuffer_id_ != framebuffer_id_) {
        current_framebuffer_id_ = framebuffer_id_;
        ARCTIC_GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_));
    }
}

void GlFramebuffer::BindDefault() {
    if (current_framebuffer_id_ != 0) {
        current_framebuffer_id_ = 0;
        ARCTIC_GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        auto code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        switch(code) {
          case GL_FRAMEBUFFER_COMPLETE:
            break;
          case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            *Log() << "Framebuffer is incomplete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
          case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            *Log() << "Framebuffer is incomplete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
          case GL_FRAMEBUFFER_UNSUPPORTED:
            *Log() << "Framebuffer is incomplete: GL_FRAMEBUFFER_UNSUPPORTED";
            break;
          default:
            *Log() << "Framebuffer is incomplete! Error code: " << code; 
            break;
        }
    }
}

}  // namespace arctic
