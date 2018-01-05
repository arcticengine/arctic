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

#ifndef ENGINE_EASY_H_
#define ENGINE_EASY_H_

#include <string>
#include <vector>

#include "engine/arctic_input.h"
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"
#include "engine/rgba.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/engine.h"

namespace arctic {
namespace easy {

void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color);
void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b);
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color);
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color_a, Rgba color_b, Rgba color_c);

void ShowFrame();

// true if key transitioned from up to down state last frame
bool IsKeyDownward(const KeyCode key_code);
bool IsKeyDownward(const char *keys);
bool IsKeyDownward(const char key);
bool IsKeyDownward(const std::string &keys);
// true is key is currently down
bool IsKeyDown(const KeyCode key_code);
bool IsKeyDown(const char *keys);
bool IsKeyDown(const char key);
bool IsKeyDown(const std::string &keys);

// true if key transitioned from down to up since last frame
bool IsKeyUpward(const KeyCode key_code);
bool IsKeyUpward(const char *keys);
bool IsKeyUpward(const char key);
bool IsKeyUpward(const std::string &keys);


// true if key transitioned from up to down state last frame
bool IsAnyKeyDownward();
// true is key is currently down
bool IsAnyKeyDown();
// true if key transitioned from down to up since last frame
bool IsAnyKeyUpward();

void SetKey(const KeyCode key_code, bool is_set_down);
void SetKey(const char key, bool is_set_down);

Vec2Si32 MousePos();
Vec2Si32 MouseMove();
Si32 MouseWheelDelta();

Vec2Si32 WindowSize();
Vec2Si32 ScreenSize();
void ResizeScreen(const Si32 width, const Si32 height);
void SetInverseY(bool is_inverse);

void Clear();
void Clear(Rgba color);

double Time();
Si64 Random(Si64 min, Si64 max);
Si32 Random32(Si32 min, Si32 max);
void Sleep(double duration_seconds);

std::vector<Ui8> ReadFile(const char *file_name);
void WriteFile(const char *file_name, const Ui8 *data, const Ui64 data_size);

Engine* GetEngine();

[[deprecated("Replaced by IsKeyDownward, which has a better name")]]
bool WasKeyPressed(const KeyCode key_code);
[[deprecated("Replaced by IsKeyDownward, which has a better name")]]
bool WasKeyPressed(const char *keys);
[[deprecated("Replaced by IsKeyDownward, which has a better name")]]
bool WasKeyPressed(const char key);
[[deprecated("Replaced by IsKeyDownward, which has a better name")]]
bool WasKeyPressed(const std::string &keys);

[[deprecated("Replaced by IsKeyDown, which has a better name")]]
bool IsKey(const KeyCode key_code);
[[deprecated("Replaced by IsKeyDown, which has a better name")]]
bool IsKey(const char *keys);
[[deprecated("Replaced by IsKeyDown, which has a better name")]]
bool IsKey(const char key);
[[deprecated("Replaced by IsKeyDown, which has a better name")]]
bool IsKey(const std::string &keys);

}  // namespace easy
}  // namespace arctic

#endif  // ENGINE_EASY_H_
