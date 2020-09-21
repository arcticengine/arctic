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

/// @addtogroup global_drawing
/// @{

/// @brief Draws a solid color line from point a to point b
void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color);

/// @brief Draws a gradient color line from point a to point b
void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b);

/// @brief Draws a solid color line from point a to point b to a sprite
void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Rgba color);

/// @brief Draws a gradient color line from point a to point b to a sprite
void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b,
    Rgba color_a, Rgba color_b);

/// @brief Draws a solid color filled triangle
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color);

/// @brief Draws a solid color filled triangle to a sprite
void DrawTriangle(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color);

/// @brief Draws a gradient color filled triangle
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color_a, Rgba color_b, Rgba color_c);

/// @brief Draws a gradient color filled triangle to a sprite
void DrawTriangle(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
    Rgba color_a, Rgba color_b, Rgba color_c);


void DrawTriangle(Sprite to_sprite,
  Vec2F a, Vec2F b, Vec2F c,
  Vec2F ta, Vec2F tb, Vec2F tc,
  Sprite texture,
  DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color);

/// @brief Draws a solid color filled rectangle
void DrawRectangle(Vec2Si32 ll, Vec2Si32 ur, Rgba color);

/// @brief Draws a solid color filled rectangle to a sprite
void DrawRectangle(Sprite to_sprite, Vec2Si32 ll, Vec2Si32 ur, Rgba color);

/// @brief Returns color of a pixel at coordinates specified
Rgba GetPixel(Si32 x, Si32 y);

/// @brief Returns color of a pixel of a sprite at coordinates specified
Rgba GetPixel(Sprite from_sprite, Si32 x, Si32 y);

/// @brief Returns color of a pixel of a sprite at coordinates specified
Rgba GetPixel(Sprite &from_sprite, Si32 x, Si32 y);

/// @brief Sets color of a pixel at coordinates specified
void SetPixel(Si32 x, Si32 y, Rgba color);

/// @brief Sets color of a pixel of a sprite at coordinates specified
void SetPixel(const Sprite &to_sprite, Si32 x, Si32 y, Rgba color);

/// @brief Draws a solid color filled circle
void DrawCircle(Vec2Si32 c, Si32 r, Rgba color);

/// @brief Draws a solid color filled circle to a sprite
void DrawCircle(Sprite to_sprite, Vec2Si32 c, Si32 r, Rgba color);

/// @brief Draws a solid color filled oval
void DrawOval(Vec2Si32 c, Vec2Si32 r, Rgba color);

/// @brief Draws a solid color filled oval to a sprite
void DrawOval(Sprite to_sprite, Vec2Si32 c, Vec2Si32 r, Rgba color);

/// @brief Show the current backbuffer and update the input state
void ShowFrame();

/// @brief Clear the backbuffer with black color
void Clear();
/// @brief Clear the backbuffer with the color specified
void Clear(Rgba color);

/// @}
/// @addtogroup global_input
/// @{

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


/// @brief true if key transitioned from up to down state last frame
bool IsAnyKeyDownward();
/// @brief true is key is currently down
bool IsAnyKeyDown();
/// @brief true if key transitioned from down to up since last frame
bool IsAnyKeyUpward();

/// @brief Changes the stored key state
/// @param key_code The key code
/// @param is_set_down Key state to set as if it was the last seen state of the key
/// @details The state is set just like it would be if the key actually transitioned
/// up or down at the end of the last frame. It may affect resuts of
/// IsKeyDownward() IsKeyDown() IsKeyUpward() IsAnyKeyDownward() IsAnyKeyDown() and
/// IsAnyKeyUpward() calls.
void SetKey(const KeyCode key_code, bool is_set_down);

/// @brief Changes the stored key state
/// @param key The key character code
/// @param is_set_down Key state to set as if it was the last seen state of the key
/// @details The state is set just like it would be if the key actually transitioned
/// up or down at the end of the last frame. It may affect resuts of
/// IsKeyDownward() IsKeyDown() IsKeyUpward() IsAnyKeyDownward() IsAnyKeyDown() and
/// IsAnyKeyUpward() calls.
void SetKey(const char key, bool is_set_down);

/// @brief Returns mouse cursor position
Vec2Si32 MousePos();
/// @brief Returns mouse movement vector
Vec2Si32 MouseMove();
/// @brief Returns mouse wheel rotation delta
Si32 MouseWheelDelta();

/// @brief Returns the number of user input messages obtained by the
/// last Swap() call
Si32 InputMessageCount();

/// @brief Returns the user input message with the index specified
const InputMessage& GetInputMessage(Si32 idx);

/// @}
/// @addtogroup global_utility
/// @{

/// @brief Get the window size in actual pixels of the OS
Vec2Si32 WindowSize();
/// @brief Get the backbuffer resolution in pixels
Vec2Si32 ScreenSize();
/// @brief Set the backbuffer resolution in pixels
void ResizeScreen(const Si32 width, const Si32 height);
/// @brief Set the backbuffer resolution in pixels
void ResizeScreen(const Vec2Si32 size);
/// @brief Enables/disables Y-coordinte inversion.
/// By default Y axis is directed upward.
void SetInverseY(bool is_inverse);

/// @brief Returns time in seconds since the game start
double Time();
/// @brief Returns a random number in range [min,max]
Si64 Random(Si64 min, Si64 max);
/// @brief Returns a random number in range [min,max]
Si32 Random32(Si32 min, Si32 max);
/// @brief Waits for the time specified before returning.
void Sleep(double duration_seconds);

/// @}
/// @addtogroup global_files
/// @{

/// @brief Loads all data from a file specified.
std::vector<Ui8> ReadFile(const char *file_name, bool is_bulletproof = false);
/// @brief Saved the data specified to a file.
void WriteFile(const char *file_name, const Ui8 *data, const Ui64 data_size);

/// @}
/// @addtogroup global_advanced
/// @{

/// @brief Returns a pointer to the Engine instance.
Engine* GetEngine();

/// @}

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


}  // namespace arctic

#endif  // ENGINE_EASY_H_
