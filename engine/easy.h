// The MIT License (MIT)
//
// Copyright (c) 2017 Huldra
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

#ifndef ENGINE_EASY_H_
#define ENGINE_EASY_H_

#include <string>
#include <vector>

#include "engine/arctic_input.h"
#include "engine/arctic_types.h"
#include "engine/csv.h"
#include "engine/easy_sound.h"
#include "engine/easy_sprite.h"
#include "engine/engine.h"
#include "engine/font.h"
#include "engine/gui.h"
#include "engine/log.h"
#include "engine/rgba.h"
#include "engine/vec2si32.h"

namespace arctic {
namespace easy {

void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color);
void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b);
void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Rgba color);
void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b,
    Rgba color_a, Rgba color_b);
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color);
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color_a, Rgba color_b, Rgba color_c);
void DrawRectangle(Vec2Si32 ll, Vec2Si32 ur, Rgba color);
Rgba GetPixel(Si32 x, Si32 y);
void SetPixel(Si32 x, Si32 y, Rgba color);
void DrawCircle(Vec2Si32 c, Si32 r, Rgba color);
void DrawOval(Vec2Si32 c, Vec2Si32 r, Rgba color);

/// @brief Show the current backbuffer and update the input state
void ShowFrame();

// true if key transitioned from up to down state last frame
/// @brief Returns true if the key with the specified key_code travelled downwards during the last frame
/// @param key_code The key code
bool IsKeyDownward(const KeyCode key_code);
/// @brief Returns true if any of the specified keys travelled downwards during the last frame
/// @param keys A c-string specifying one or more key character codes to check
bool IsKeyDownward(const char *keys);
/// @brief Returns true if the key specified travelled downwards during the last frame
/// @param key The key character code
bool IsKeyDownward(const char key);
/// @brief Returns true if any of the specified keys travelled downwards during the last frame
/// @param keys A std::string specifying one or more key character codes to check
bool IsKeyDownward(const std::string &keys);

// true is key is currently down
/// @brief Returns true if the key with the specified key_code was pressed during the last frame
/// @param key_code The key code
bool IsKeyDown(const KeyCode key_code);
/// @brief Returns true if any of the specified keys was pressed during the last frame
/// @param keys A c-string specifying one or more key character codes to check
bool IsKeyDown(const char *keys);
/// @brief Returns true if the key specified was pressed during the last frame
/// @param key The key character code
bool IsKeyDown(const char key);
/// @brief Returns true if any of the specified keys was pressed during the last frame
/// @param keys A std::string specifying one or more key character codes to check
bool IsKeyDown(const std::string &keys);

// true if key transitioned from down to up since last frame
/// @brief Returns true if the key with the specified key_code was released during the last frame
/// @param key_code The key code
bool IsKeyUpward(const KeyCode key_code);
/// @brief Returns true if any of the specified keys was released during the last frame
/// @param keys A c-string specifying one or more key character codes to check
bool IsKeyUpward(const char *keys);
/// @brief Returns true if the key specified was released during the last frame
/// @param key The key character code
bool IsKeyUpward(const char key);
/// @brief Returns true if any of the specified keys was released during the last frame
/// @param keys A std::string specifying one or more key character codes to check
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

Si32 InputMessageCount();
const InputMessage& GetInputMessage(Si32 idx);

Vec2Si32 WindowSize();
Vec2Si32 ScreenSize();
void ResizeScreen(const Si32 width, const Si32 height);
void ResizeScreen(const Vec2Si32 size);
void SetInverseY(bool is_inverse);

void Clear();
void Clear(Rgba color);

double Time();
Si64 Random(Si64 min, Si64 max);
Si32 Random32(Si32 min, Si32 max);
void Sleep(double duration_seconds);

std::vector<Ui8> ReadFile(const char *file_name, bool is_bulletproof = false);
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
