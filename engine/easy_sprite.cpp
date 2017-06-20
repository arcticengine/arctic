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

#include <algorithm>
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
        ref_pos_ = Vec2Si32(0, 0);
        ref_size_ = Vec2Si32(sprite_instance_->width(),
            sprite_instance_->height());
    } else {
        Fatal("Error in Sprite::Load, unknown file extension.");
    }
}

void Sprite::Load(const std::string &file_name) {
    Load(file_name.c_str());
}

void Sprite::Create(const Si32 width, const Si32 height) {
    sprite_instance_.reset(new SpriteInstance(width, height));
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = Vec2Si32(width, height);
    Clear();
}

void Sprite::Reference(Sprite from, const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height) {
    ref_pos_ = Vec2Si32(
        from.ref_pos_.x + std::min(std::max(from_x, 0), from.ref_size_.x - 1),
        from.ref_pos_.y + std::min(std::max(from_y, 0), from.ref_size_.y - 1));
    const Vec2Si32 max_size = from.ref_pos_ + from.ref_size_ - ref_pos_;
    ref_size_ = Vec2Si32(
        std::min(from_width, max_size.x),
        std::min(from_height, max_size.y));
    sprite_instance_ = from.sprite_instance_;
}

void Sprite::Clear() {
    if (!sprite_instance_.get()) {
        return;
    }
    const size_t size = static_cast<size_t>(ref_size_.x) * sizeof(Rgba);
    Ui8 *data = sprite_instance_->RawData();
    const Si32 stride = StrideBytes();
    for (Si32 y = 0; y < ref_size_.y; ++y) {
        memset(data, 0, size);
        data += stride;
    }
}

void Sprite::Clear(Rgba color) {
    if (!sprite_instance_.get()) {
        return;
    }
    const Si32 stride = StridePixels();
    Rgba *begin = reinterpret_cast<Rgba*>(sprite_instance_->RawData());
    Rgba *end = begin + ref_size_.x;
    for (Si32 y = 0; y < ref_size_.y; ++y) {
        Rgba *p = begin;
        while (p != end) {
            p->rgba = color.rgba;
            p++;
        }
        begin += stride;
        end += stride;
    }
}

void Sprite::SetPivot(Vec2Si32 pivot) {
    pivot_ = pivot;
}

Vec2Si32 Sprite::Pivot() const {
    return pivot_;
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y) {
    if (!sprite_instance_) {
        return;
    }
    Draw(to_x, to_y, ref_size_.x, ref_size_.y,
        0, 0, ref_size_.x, ref_size_.y);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height) {
    Draw(to_x, to_y, to_width, to_height,
        0, 0, ref_size_.x, ref_size_.y);
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
        ref_size_.x, ref_size_.y,
        0, 0, ref_size_.x, ref_size_.y);
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
        0, 0, ref_size_.x, ref_size_.y);
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
        const Vec2Si32 from_pos, const Vec2Si32 from_size) {
    Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
        from_pos.x, from_pos.y, from_size.x, from_size.y);
}

void Sprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot,
        const Si32 to_width, const Si32 to_height,
        const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height,
        Sprite to_sprite) {
    const Si32 from_stride_pixels = StridePixels();
    const Si32 to_stride_pixels = to_sprite.width();

    const Si32 to_x = to_x_pivot - pivot_.x * to_width / from_width;
    const Si32 to_y = to_y_pivot - pivot_.y * to_height / from_height;

    Rgba *to = to_sprite.RgbaData()
        + to_y * to_stride_pixels
        + to_x;
    const Rgba *from = RgbaData()
        + from_y * from_stride_pixels
        + from_x;

    const Si32 to_y_db = (to_y >= 0 ? 0 : -to_y);
    const Si32 to_y_d_max = to_sprite.height() - to_y;
    const Si32 to_y_de = (to_height < to_y_d_max ? to_height : to_y_d_max);

    const Si32 to_x_db = (to_x >= 0 ? 0 : -to_x);
    const Si32 to_x_d_max = to_sprite.width() - to_x;
    const Si32 to_x_de = (to_width < to_x_d_max ? to_width : to_x_d_max);

    for (Si32 to_y_disp = to_y_db; to_y_disp < to_y_de; ++to_y_disp) {
        const Si32 from_y_disp = (from_height * to_y_disp) / to_height;

        const Si32 from_x_b = (from_width * to_x_db) / to_width;
        const Si32 from_x_step_16 = 65536 * from_width / to_width;
        Si32 from_x_acc_16 = 0;

        const Rgba *from_line = from + from_y_disp * from_stride_pixels;
        Rgba *to_line = to + to_y_disp * to_stride_pixels;

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
    return ref_size_.x;
}

Si32 Sprite::height() const {
    return ref_size_.y;
}

Vec2Si32 Sprite::Size() const {
    return ref_size_;
}

Si32 Sprite::StrideBytes() const {
    return sprite_instance_->width() * sizeof(Rgba);
}

Si32 Sprite::StridePixels() const {
    return sprite_instance_->width();
}

Ui8* Sprite::RawData() {
    return sprite_instance_->RawData();
}

Rgba* Sprite::RgbaData() {
    return (static_cast<Rgba*>(static_cast<void*>(
            sprite_instance_->RawData())) +
        ref_pos_.y * StridePixels() +
        ref_pos_.x);
}

}  // namespace easy
}  // namespace arctic
