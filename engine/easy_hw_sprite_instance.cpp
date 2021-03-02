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


  HwSpriteInstance::HwSpriteInstance(Si32 width, Si32 height) {
    texture_.Create(width, height);
    framebuffer_.Create(texture_);
  }

  std::shared_ptr<HwSpriteInstance> HwSpriteInstance::LoadTga(const Ui8 *data, const Si64 size) {
    std::shared_ptr<SpriteInstance> sw_sprite = arctic::LoadTga(data, size);
    std::shared_ptr<HwSpriteInstance> sprite = std::make_shared<HwSpriteInstance>(sw_sprite->width(), sw_sprite->height());

    if (sw_sprite) {
        sprite->texture_.SetData(sw_sprite->RawData(), sw_sprite->width(), sw_sprite->height());
    }

    return sprite;
  }

/*  void HwSpriteInstance::SaveTga(std::shared_ptr<HwSpriteInstance> sprite, std::vector<Ui8> *data) {
    std::shared_ptr<SpriteInstance> sw_sprite = std::make_shared<SpriteInstance>(sprite->texture().width(), sprite->texture().height());
    sprite->texture().ReadData(sw_sprite->RawData());
    arctic::SaveTga(sw_sprite, data);
  }
  */

}  // namespace arctic

template class std::shared_ptr<arctic::HwSpriteInstance>;
