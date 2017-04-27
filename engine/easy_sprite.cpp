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

namespace arctic {
namespace easy {

void Sprite::Load(const char *file_name) {
    // TODO(Huldra): Implement this
}

void Sprite::Load(const std::string &file_name) {
    Load(file_name.c_str());
}

void Sprite::Create(const Si32 width, const Si32 height) {
    // TODO(Huldra): Implement this
}

void Sprite::Reference(Sprite from, const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height) {
    // TODO(Huldra): Implement this
}
void Sprite::Clear() {
    // TODO(Huldra): Implement this
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y) {
    Draw(to_x, to_y, sprite_instance->width(), sprite_instance->height(),
        0, 0, sprite_instance->width(), sprite_instance->height());
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height) {
    Draw(to_x, to_y, to_width, to_height,
        0, 0, sprite_instance->width(), sprite_instance->height());
}
void Sprite::Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height,
        const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 form_height) {
    // TODO(Huldra): Implement this
}

void Sprite::Draw(const Vec2Si32 to_pos) {
    Draw(to_pos.x, to_pos.y,
        sprite_instance->width(), sprite_instance->height(),
        0, 0, sprite_instance->width(), sprite_instance->height());
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
        0, 0, sprite_instance->width(), sprite_instance->height());
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
        const Vec2Si32 from_pos, const Vec2Si32 from_size) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
        from_pos.x, from_pos.y, from_size.x, from_size.y);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height,
        const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 form_height,
        Sprite *to_sprite) {
    // TODO(Huldra): Implement this
}

Si32 Sprite::width() const {
    return sprite_instance->width();
}

Si32 Sprite::height() const {
    return sprite_instance->height();
}

Ui8* Sprite::RawData() {
    return sprite_instance->RawData();
}


}  // namespace easy
}  // namespace arctic
