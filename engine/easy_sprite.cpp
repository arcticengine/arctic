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

#include <vector>
#include "engine/easy.h"
#include "engine/rgba.h"

namespace arctic {
namespace easy {

void Sprite::Load(const char *file_name) {
    Check(!!file_name, "Error in Sprite::Load, file_name is nullptr.");
    const char *last_dot = strchr(file_name, '.');
    Check(!!last_dot, "Error in Sprite::Load, file_name has no extension.");
    if (strcmp(last_dot, ".tga") == 0) {
        std::vector<Ui8> data = ReadFile(file_name);
        sprite_instance_ = LoadTga(data.data(), data.size());
    } else {
        Fatal("Error in Sprite::Load, unknown file extension.");
    }
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

void Sprite::Clear(Rgba color) {
    if (!sprite_instance_.get()) {
        return;
    }
    Si64 size = static_cast<Si64>(sprite_instance_->width())
        * static_cast<Si64>(sprite_instance_->height());
    Rgba *p = reinterpret_cast<Rgba*>(sprite_instance_->RawData());
    Rgba *end = p + size;
    while (p != end) {
        *p = color;
    }
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
    const Si32 from_sprite_width = width();
    const Si32 to_sprite_width = to_sprite.width();

    Rgba *to = to_sprite.RgbaData()
        + to_y * to_sprite_width
        + to_x;
    const Rgba *from = RgbaData()
        + from_y * from_sprite_width
        + from_x;

    const Si32 to_y_db = (to_y >= 0 ? 0 : -to_y);
    const Si32 to_y_d_max = to_sprite.height() - to_y;
    const Si32 to_y_de = (to_height < to_y_d_max ? to_height : to_y_d_max);

    const Si32 to_x_db = (to_x >= 0 ? 0 : -to_x);
    const Si32 to_x_d_max = to_sprite_width - to_x;
    const Si32 to_x_de = (to_width < to_x_d_max ? to_width : to_x_d_max);

    for (Si32 to_y_disp = to_y_db; to_y_disp < to_y_de; ++to_y_disp) {
        const Si32 from_y_disp = (from_height * to_y_disp) / to_height;
        
        const Si32 from_x_b = (from_width * to_x_db) / to_width;
        const Si32 from_x_step_16 = 65536 * from_width / to_width;
        Si32 from_x_acc_16 = 0;

        const Rgba *from_line = from + from_y_disp * from_sprite_width;
        Rgba *to_line = to + to_y_disp * to_sprite_width;
        
        for (Si32 to_x_disp = to_x_db; to_x_disp < to_x_de; ++to_x_disp) {
            Rgba *to_rgba = to_line + to_x_disp;
            const Si32 from_x_disp = from_x_b + (from_x_acc_16 / 65536);
            from_x_acc_16 += from_x_step_16;
            const Rgba *from_rgba = from_line + from_x_disp;
            if (from_rgba->a) {
                to_rgba->rgba = from_rgba->rgba;
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

Vec2Si32 Sprite::Size() const {
    return Vec2Si32(sprite_instance_->width(), sprite_instance_->height());
}

Ui8* Sprite::RawData() {
    return sprite_instance_->RawData();
}

Rgba* Sprite::RgbaData() {
    return static_cast<Rgba*>(static_cast<void*>(sprite_instance_->RawData()));
}


}  // namespace easy
}  // namespace arctic
