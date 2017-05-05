// The MIT License(MIT)
//
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#include "engine/easy_sprite.h"

#include "engine/easy.h"
#include "engine/rgba.h"

namespace arctic {
namespace easy {

void Sprite::Load(const char *file_name) {
    // TODO(Huldra): Implement this
}

void Sprite::Load(const std::string &file_name) {
    Load(file_name.c_str());
}

void Sprite::Create(const Si32 width, const Si32 height) {
    sprite_instance_.reset(new SpriteInstance(width, height));
    Clear();
}

void Sprite::Reference(Sprite from, const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height) {
    // TODO(Huldra): Implement this
}

void Sprite::Clear() {
    if (!sprite_instance_.get()) {
        return;
    }
    Si64 size = static_cast<Si64>(sprite_instance_->width())
        * static_cast<Si64>(sprite_instance_->height())
        * sizeof(Rgba);
    memset(sprite_instance_->RawData(), 0, static_cast<size_t>(size));
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y) {
    if (!sprite_instance_) {
        return;
    }
    Draw(to_x, to_y, sprite_instance_->width(), sprite_instance_->height(),
        0, 0, sprite_instance_->width(), sprite_instance_->height());
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height) {
    Draw(to_x, to_y, to_width, to_height,
        0, 0, sprite_instance_->width(), sprite_instance_->height());
}
void Sprite::Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height,
        const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height) {
    Draw(to_x, to_y, to_width, to_height,
        from_x, from_y, from_width, from_height,
        GetEngine()->GetBackbuffer());
}

void Sprite::Draw(const Vec2Si32 to_pos) {
    Draw(to_pos.x, to_pos.y,
        sprite_instance_->width(), sprite_instance_->height(),
        0, 0, sprite_instance_->width(), sprite_instance_->height());
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
        0, 0, sprite_instance_->width(), sprite_instance_->height());
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
        const Vec2Si32 from_pos, const Vec2Si32 from_size) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
        from_pos.x, from_pos.y, from_size.x, from_size.y);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height,
        const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height,
        Sprite to_sprite) {
    Rgba *to = to_sprite.RgbaData()
        + to_y * to_sprite.width()
        + to_x;
    Rgba *from = RgbaData()
        + from_y * width()
        + from_x;
    for (Si32 to_y_disp = 0; to_y_disp < to_height; ++to_y_disp) {
        for (Si32 to_x_disp = 0; to_x_disp < to_width; ++to_x_disp) {
            Rgba *to_rgba = to + to_y_disp * to_sprite.width() + to_x_disp;
            Si32 from_x_disp = (from_width * to_x_disp) / to_width;
            Si32 from_y_disp = (from_height * to_y_disp) / to_height;
            Rgba *from_rgba = from + from_y_disp * width() + from_x_disp;
            if (from_rgba->a) {
                *to_rgba = *from_rgba;
            }
        }
    }
}

Si32 Sprite::width() const {
    return sprite_instance_->width();
}

Si32 Sprite::height() const {
    return sprite_instance_->height();
}

Ui8* Sprite::RawData() {
    return sprite_instance_->RawData();
}

Rgba* Sprite::RgbaData() {
    return static_cast<Rgba*>(static_cast<void*>(sprite_instance_->RawData()));
}


}  // namespace easy
}  // namespace arctic
