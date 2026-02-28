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

GLuint GlTexture2D::current_texture_slot_ = 0;
GLuint GlTexture2D::current_texture_id_[max_textures_slots] = { 0, 0, 0, 0, 0, 0, 0, 0 };

GlTexture2D::GlTexture2D()
    : width_(0)
    , height_(0)
    , texture_id_(0) {
}

GlTexture2D::~GlTexture2D() {
    for (GLuint i = 0; i < max_textures_slots; ++i) {
        if (current_texture_id_[i] == texture_id_) {
            current_texture_id_[i] = 0;
        }
    }
    ARCTIC_GL_CHECK_ERROR(glDeleteTextures(1, &texture_id_));
}

void GlTexture2D::Create(Si32 w, Si32 h) {
    width_ = w;
    height_ = h;

    for (GLuint i = 0; i < max_textures_slots; ++i) {
        if (current_texture_id_[i] == texture_id_) {
            current_texture_id_[i] = 0;
        }
    }
    ARCTIC_GL_CHECK_ERROR(glDeleteTextures(1, &texture_id_));

    ARCTIC_GL_CHECK_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    ARCTIC_GL_CHECK_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
    ARCTIC_GL_CHECK_ERROR(glGenTextures(1, &texture_id_));
    Bind(0);
    Check(glIsTexture(texture_id_), "no texture");

    ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    current_filter_mode_ = kFilterBilinear;
    SetFilterMode(kFilterNearest);

    ARCTIC_GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, nullptr));
}

void GlTexture2D::CreateDepth(Si32 w, Si32 h) {
    width_ = w;
    height_ = h;

    for (GLuint i = 0; i < max_textures_slots; ++i) {
        if (current_texture_id_[i] == texture_id_) {
            current_texture_id_[i] = 0;
        }
    }
    ARCTIC_GL_CHECK_ERROR(glDeleteTextures(1, &texture_id_));

    ARCTIC_GL_CHECK_ERROR(glGenTextures(1, &texture_id_));
    Bind(0);
    Check(glIsTexture(texture_id_), "no depth texture");

    ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    current_filter_mode_ = kFilterNearest;

    ARCTIC_GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr));
}

void GlTexture2D::Bind(Ui32 slot) const {
    Check(slot < max_textures_slots, "invalid texture slot");
    if (current_texture_slot_ != slot) {
        current_texture_slot_ = slot;
        ARCTIC_GL_CHECK_ERROR(glActiveTexture(GL_TEXTURE0 + slot));
    }
    if (current_texture_id_[slot] != texture_id_) {
        current_texture_id_[slot] = texture_id_;
        ARCTIC_GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture_id_));
    }
}

void GlTexture2D::SetData(const void *data, Si32 w, Si32 h) {
    width_ = w;
    height_ = h;
    
    Bind(0);
    ARCTIC_GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
}

void GlTexture2D::UpdateData(const void *data) {
    Bind(0);
    ARCTIC_GL_CHECK_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, data));
}

/*void GlTexture2D::ReadData(void *dst) const {
    Bind(0);
    ARCTIC_GL_CHECK_ERROR(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst));
    Bind();
    int data_size = mWidth * mHeight * 4;
    GLubyte* pixels = new GLubyte[mWidth * mHeight * 4];

    GLuint textureObj = ...; // the texture object - glGenTextures  

    GLuint fbo;
    glGenFramebuffers(1, &fbo); 
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureObj, 0);

    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}*/

void GlTexture2D::SetFilterMode(DrawFilterMode filter_mode) {
    if (current_filter_mode_ != filter_mode) {
        current_filter_mode_ = filter_mode;
        Bind(0);
        switch (filter_mode) {
            case kFilterNearest:
                ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
                ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
                break;
            case kFilterBilinear:
                ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                break;
        }
    }
}


}  // namespace arctic
