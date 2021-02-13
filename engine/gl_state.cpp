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

#include "engine/gl_state.h"

#include <sstream>
#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgba.h"
#include "engine/opengl.h"

namespace arctic {

Vec4Si32 GlState::current_viewport_ = Vec4Si32(0, 0, 0, 0);
DrawBlendingMode GlState::current_blending_mode_ = kDrawBlendingModeCopyRgba;

void GlState::SetViewport(Si32 x, Si32 y, Si32 w, Si32 h) {
    if (current_viewport_ != Vec4Si32(x, y, w, h)) {
        current_viewport_ = Vec4Si32(x, y, w, h);
        ARCTIC_GL_CHECK_ERROR(glViewport(x, y, w, h));
    }
}

void GlState::SetBlending(DrawBlendingMode mode) {
    if (current_blending_mode_ != mode) {
        current_blending_mode_ = mode;
        switch (mode) {
            case kDrawBlendingModeCopyRgba:
                ARCTIC_GL_CHECK_ERROR(glDisable(GL_BLEND));
                break;
            case kDrawBlendingModeAlphaBlend:
                ARCTIC_GL_CHECK_ERROR(glEnable(GL_BLEND));
                ARCTIC_GL_CHECK_ERROR(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
                break;
            case kDrawBlendingModeColorize:
                ARCTIC_GL_CHECK_ERROR(glEnable(GL_BLEND));
                ARCTIC_GL_CHECK_ERROR(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
                break;
            case kDrawBlendingModeAdd:
                ARCTIC_GL_CHECK_ERROR(glEnable(GL_BLEND));
                ARCTIC_GL_CHECK_ERROR(glBlendFunc(GL_ONE, GL_ONE));
                break;
            default:
                ARCTIC_GL_CHECK_ERROR(glDisable(GL_BLEND));
                break;
        }
    }
}

}  // namespace arctic
