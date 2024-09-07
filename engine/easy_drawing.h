// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
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

#ifndef ENGINE_EASY_DRAWING_H_
#define ENGINE_EASY_DRAWING_H_

#include "engine/arctic_types.h"
#include "engine/easy_sprite.h"
#include "engine/vec2si32.h"
#include "engine/rgba.h"

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
void DrawTriangle(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
  Rgba color);

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
Rgba GetPixel(const Sprite &from_sprite, Si32 x, Si32 y);

/// @brief Sets color of a pixel at coordinates specified
void SetPixel(Si32 x, Si32 y, Rgba color);

/// @brief Sets color of a pixel of a sprite at coordinates specified
void SetPixel(const Sprite &to_sprite, Si32 x, Si32 y, Rgba color);

/// @brief Sets the new color to all the pixels of the old color
void ReplaceColor(Sprite to_sprite, Rgba old_color, Rgba new_color);

/// @brief Draws a solid color filled circle
void DrawCircle(Vec2Si32 c, Si32 r, Rgba color);

/// @brief Draws a solid color filled circle to a sprite
void DrawCircle(Sprite to_sprite, Vec2Si32 c, Si32 r, Rgba color);

/// @brief Draws a solid color filled oval
void DrawOval(Vec2Si32 c, Vec2Si32 r, Rgba color);

/// @brief Draws a solid color filled oval to a sprite
void DrawOval(Sprite to_sprite, Vec2Si32 c, Vec2Si32 r, Rgba color);

/// @brief Draws a solid color filled oval to a sprite
void DrawOval(Sprite to_sprite, Rgba color, Vec2Si32 ll, Vec2Si32 ur);

/// @brief Draw a rounded corner rectangular block shape.
/// @param [in] to_sprite Sprite to draw the block on.
/// @param [in] lower_left_pos Lower-left block corner position (as if it was not rounded).
/// @param [in] size Block size.
/// @param [in] corner_radius External radius of block corners.
/// @param [in] color Fill color of the block.
void DrawBlock(Sprite &to_sprite, Vec2F lower_left_pos, Vec2F size, float corner_radius, Rgba color);

/// @brief Draw a rounded corner rectangular block shape. The shape has a border.
/// @param [in] to_sprite Sprite to draw the block on.
/// @param [in] lower_left_pos Lower-left block corner position (as if it was not rounded).
/// @param [in] size Block size.
/// @param [in] corner_radius External radius of block corners.
/// @param [in] color Fill color of the block.
/// @param [in] border_size Border width.
/// @param [in] border_color Border color.
void DrawBlock(Sprite &to_sprite, Vec2F lower_left_pos, Vec2F size, float corner_radius,
    Rgba color, float border_size, Rgba border_color);

/// @brief Draw an arrow shape.
/// @param [in] to_sprite Sprite to draw the arrow on.
/// @param [in] source_pos Tail position (source point).
/// @param [in] destination_pos Head position (destination point).
/// @param [in] body_width Tail width.
/// @param [in] head_width Head width.
/// @param [in] head_length Head length.
/// @param [in] color Fill color of the arrow.
void DrawArrow(Sprite &to_sprite, Vec2F source_pos, Vec2F destination_pos,
               float body_width, float head_width, float head_length, Rgba color);

/// @brief Draw an arrow shape. The shape has a border.
/// @param [in] to_sprite Sprite to draw the arrow on.
/// @param [in] source_pos Tail position (source point).
/// @param [in] destination_pos Head position (destination point).
/// @param [in] body_width Tail width.
/// @param [in] head_width Head width.
/// @param [in] head_length Head length.
/// @param [in] color Fill color of the arrow.
/// @param [in] border_size Border width.
/// @param [in] border_color Border color.
void DrawArrow(Sprite &to_sprite, Vec2F source_pos, Vec2F destination_pos,
               float body_width, float head_width, float head_length, Rgba color,
               float border_size, Rgba border_color);

Vec2F BlockEdgePos(Vec2F lower_left_pos, Vec2F size, float corner_radius, Vec2F direction);

/// @brief Show the current backbuffer and update the input state
void ShowFrame();

/// @brief Clear the backbuffer with black color
void Clear();
/// @brief Clear the backbuffer with the color specified
void Clear(Rgba color);

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_DRAWING_H_
