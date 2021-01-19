// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2015 - 2017 Inigo Quilez
// Copyright (c) 2017 - 2020 Huldra
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

#include "engine/easy_hw_sprite_instance.h"

#include <cstring>
#include <memory>
#include <sstream>

#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/rgba.h"
#include "engine/opengl.h"
#include "engine/easy_sprite_instance.h"

namespace arctic {


  HwSpriteInstance::HwSpriteInstance(Si32 width, Si32 height)
    : width_(width)
      , height_(height)
      , texture_id_(0) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    Check(glIsTexture(texture_id_), "no texture");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, nullptr);

    {
        GLenum errCode = glGetError();
        *Log() << "gl sprite instance creation code: " << (Ui64)errCode;
    }
  }

  HwSpriteInstance::~HwSpriteInstance() {
    glDeleteTextures(1, &texture_id_);
  }

  std::shared_ptr<HwSpriteInstance> HwSpriteInstance::LoadTga(const Ui8 *data, const Si64 size) {
    std::shared_ptr<SpriteInstance> sw_sprite = arctic::LoadTga(data, size);
    std::shared_ptr<HwSpriteInstance> sprite = std::make_shared<HwSpriteInstance>(sw_sprite->width(), sw_sprite->height());

    if (sw_sprite) {
        glBindTexture(GL_TEXTURE_2D, sprite->texture_id());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sprite->width(), sprite->height(), GL_RGBA, GL_UNSIGNED_BYTE, sw_sprite->RawData());

        {
            GLenum errCode = glGetError();
            *Log() << "gl sprite loading code: " << (Ui64)errCode;
        }
    }

    return sprite;
  }

  void HwSpriteInstance::SaveTga(std::shared_ptr<HwSpriteInstance> sprite, std::vector<Ui8> *data) {
    std::shared_ptr<SpriteInstance> sw_sprite = std::make_shared<SpriteInstance>(sprite->width(), sprite->height());

    glBindTexture(GL_TEXTURE_2D, sprite->texture_id());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, sw_sprite->RawData());

    {
        GLenum errCode = glGetError();
        *Log() << "gl sprite instance saving code: " << (Ui64)errCode;
    }

    arctic::SaveTga(sw_sprite, data);
  }

}  // namespace arctic

template class std::shared_ptr<arctic::HwSpriteInstance>;
