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

#ifndef ENGINE_EASY_SPRITE_H_
#define ENGINE_EASY_SPRITE_H_

#include <string>
#include <memory>

#include "engine/easy_sprite_instance.h"
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"
#include "engine/rgba.h"

namespace arctic {
namespace easy {

class Sprite {
 private:
    std::shared_ptr<SpriteInstance> sprite_instance_;

 public:
    void Load(const char *file_name);
    void Load(const std::string &file_name);
    void Create(const Si32 width, const Si32 height);
    void Reference(Sprite from, const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height);
    void Clear();
    void Clear(Rgba color);
    void Draw(const Si32 to_x, const Si32 to_y);
    void Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height);
    void Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height,
        const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height);
    void Draw(const Vec2Si32 to_pos);
    void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size);
    void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
        const Vec2Si32 from_pos, const Vec2Si32 from_size);
    void Draw(const Si32 to_x, const Si32 to_y,
        const Si32 to_width, const Si32 to_height,
        const Si32 from_x, const Si32 from_y,
        const Si32 from_width, const Si32 from_height,
        Sprite to_sprite);
    Si32 width() const;
    Si32 height() const;
    Vec2Si32 Size() const;
    Ui8* RawData();
    Rgba* RgbaData();
};

}  // namespace easy
}  // namespace arctic

#endif  // ENGINE_EASY_SPRITE_H_
