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

class HwSprite;

/// @addtogroup global_drawing
/// @{

// Every point here is in backbuffer pixels with (0, 0) at the bottom-left
// corner and y growing upward, and rectangle corners are inclusive. See the
// "Where Zero Is and Which Way Is Up" section of the documentation for the
// whole convention and for FromTopLeft(), which converts a layout measured
// from the top of the screen.

/// @brief Draws a solid color line from point a to point b
/// @param [in] a Starting point of the line.
/// @param [in] b Ending point of the line.
/// @param [in] color Color of the line.
void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color);

/// @brief Draws a gradient color line from point a to point b
/// @param [in] a Starting point of the line.
/// @param [in] b Ending point of the line.
/// @param [in] color_a Color at the starting point.
/// @param [in] color_b Color at the ending point.
void DrawLine(Vec2Si32 a, Vec2Si32 b, Rgba color_a, Rgba color_b);

/// @brief Draws a solid color line from point a to point b to a sprite
/// @param [in] to_sprite Sprite to draw the line on.
/// @param [in] a Starting point of the line.
/// @param [in] b Ending point of the line.
/// @param [in] color Color of the line.
void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Rgba color);

/// @brief Draws a gradient color line from point a to point b to a sprite
/// @param [in] to_sprite Sprite to draw the line on.
/// @param [in] a Starting point of the line.
/// @param [in] b Ending point of the line.
/// @param [in] color_a Color at the starting point.
/// @param [in] color_b Color at the ending point.
void DrawLine(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b,
  Rgba color_a, Rgba color_b);

/// @brief Draws a solid color filled triangle
/// @param [in] a First vertex of the triangle.
/// @param [in] b Second vertex of the triangle.
/// @param [in] c Third vertex of the triangle.
/// @param [in] color Color of the triangle.
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c, Rgba color);

/// @brief Draws a solid color filled triangle to a sprite
/// @param [in] to_sprite Sprite to draw the triangle on.
/// @param [in] a First vertex of the triangle.
/// @param [in] b Second vertex of the triangle.
/// @param [in] c Third vertex of the triangle.
/// @param [in] color Color of the triangle.
void DrawTriangle(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
  Rgba color);

/// @brief Draws a gradient color filled triangle
/// @param [in] a First vertex of the triangle.
/// @param [in] b Second vertex of the triangle.
/// @param [in] c Third vertex of the triangle.
/// @param [in] color_a Color at the first vertex.
/// @param [in] color_b Color at the second vertex.
/// @param [in] color_c Color at the third vertex.
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
  Rgba color_a, Rgba color_b, Rgba color_c);

/// @brief Draws a gradient color filled triangle to a sprite
/// @param [in] to_sprite Sprite to draw the triangle on.
/// @param [in] a First vertex of the triangle.
/// @param [in] b Second vertex of the triangle.
/// @param [in] c Third vertex of the triangle.
/// @param [in] color_a Color at the first vertex.
/// @param [in] color_b Color at the second vertex.
/// @param [in] color_c Color at the third vertex.
void DrawTriangle(Sprite to_sprite, Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
  Rgba color_a, Rgba color_b, Rgba color_c);

/// @brief Draws a textured triangle to a sprite
/// @param [in] to_sprite Sprite to draw the triangle on.
/// @param [in] a First vertex of the triangle.
/// @param [in] b Second vertex of the triangle.
/// @param [in] c Third vertex of the triangle.
/// @param [in] ta Texture coordinate for the first vertex.
/// @param [in] tb Texture coordinate for the second vertex.
/// @param [in] tc Texture coordinate for the third vertex.
/// @param [in] texture Texture to apply to the triangle.
/// @param [in] blending_mode Blending mode for drawing the triangle.
/// @param [in] filter_mode Filter mode for the texture.
/// @param [in] in_color Color to multiply the texture with.
void DrawTriangle(Sprite to_sprite,
  Vec2F a, Vec2F b, Vec2F c,
  Vec2F ta, Vec2F tb, Vec2F tc,
  Sprite texture,
  DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color);

/// @brief Draws a solid color filled rectangle
/// @param [in] ll Lower-left corner of the rectangle, inclusive.
/// @param [in] ur Upper-right corner of the rectangle, inclusive.
/// @param [in] color Color of the rectangle.
/// @details Both corners belong to the rectangle, so equal corners paint one
/// pixel and a box of size s at p ends at p + s - (1, 1). The corners may come
/// in any order, the rectangle is clipped to the target, and the color is alpha
/// blended, with a fully transparent one painting nothing.
///
/// This is the software path: it writes pixels into the backbuffer, which is
/// composed above every hardware sprite. DrawRectangleHw() is the hardware
/// counterpart.
void DrawRectangle(Vec2Si32 ll, Vec2Si32 ur, Rgba color);

/// @brief Draws a solid color filled rectangle to a sprite
/// @param [in] to_sprite Sprite to draw the rectangle on.
/// @param [in] ll Lower-left corner of the rectangle, inclusive.
/// @param [in] ur Upper-right corner of the rectangle, inclusive.
/// @param [in] color Color of the rectangle.
void DrawRectangle(Sprite to_sprite, Vec2Si32 ll, Vec2Si32 ur, Rgba color);

/// @brief Fills a rectangle with a color through the hardware path
/// @param [in] ll Lower-left corner of the rectangle, inclusive.
/// @param [in] ur Upper-right corner of the rectangle, inclusive.
/// @param [in] color Color of the rectangle, alpha blended.
/// @details A filled rectangle without a sprite for it. A wall, a panel, a
/// health bar or a dimmed background is one call rather than a texture created,
/// cleared to a color and drawn, which is what the absence of this function
/// used to cost.
///
/// The rectangle joins the queue of hardware drawings, so it obeys the same
/// order as HwSprite::Draw: a later call is above an earlier one, and the
/// software backbuffer is composed above all of them. The engine keeps the one
/// pixel texture such a fill needs, see Engine::SolidHwSprite.
///
/// Corners are inclusive and may come in any order, exactly as in
/// DrawRectangle. A run without a GL context draws nothing, since there is no
/// hardware path in it at all.
void DrawRectangleHw(Vec2Si32 ll, Vec2Si32 ur, Rgba color);

/// @brief Fills a rectangle of a hardware sprite with a color
/// @param [in] to_sprite Hardware sprite to fill a part of.
/// @param [in] ll Lower-left corner of the rectangle, inclusive.
/// @param [in] ur Upper-right corner of the rectangle, inclusive.
/// @param [in] color Color to write.
/// @details For preparing a texture rather than drawing a frame, so it differs
/// from the screen overload in two ways worth knowing. It takes effect at once
/// instead of waiting for the frame to be composed, because the queue of
/// hardware drawings knows only one destination, the frame itself. And the
/// color is written rather than blended, so a color with alpha lands in the
/// sprite as it is, which is what a texture with holes in it needs.
void DrawRectangleHw(const HwSprite &to_sprite, Vec2Si32 ll, Vec2Si32 ur,
  Rgba color);

/// @brief Returns color of a pixel at coordinates specified
/// @param [in] x X-coordinate of the pixel.
/// @param [in] y Y-coordinate of the pixel.
/// @return Color of the pixel at the specified coordinates.
Rgba GetPixel(Si32 x, Si32 y);

/// @brief Returns color of a pixel of a sprite at coordinates specified
/// @param [in] from_sprite Sprite to get the pixel from.
/// @param [in] x X-coordinate of the pixel.
/// @param [in] y Y-coordinate of the pixel.
/// @return Color of the pixel at the specified coordinates in the sprite.
Rgba GetPixel(const Sprite &from_sprite, Si32 x, Si32 y);

/// @brief Sets color of a pixel at coordinates specified
/// @param [in] x X-coordinate of the pixel.
/// @param [in] y Y-coordinate of the pixel.
/// @param [in] color Color to set the pixel to.
void SetPixel(Si32 x, Si32 y, Rgba color);

/// @brief Sets color of a pixel of a sprite at coordinates specified
/// @param [in] to_sprite Sprite to set the pixel in.
/// @param [in] x X-coordinate of the pixel.
/// @param [in] y Y-coordinate of the pixel.
/// @param [in] color Color to set the pixel to.
void SetPixel(const Sprite &to_sprite, Si32 x, Si32 y, Rgba color);

/// @brief Sets the new color to all the pixels of the old color
/// @param [in] to_sprite Sprite to replace colors in.
/// @param [in] old_color Color to be replaced.
/// @param [in] new_color Color to replace with.
void ReplaceColor(Sprite to_sprite, Rgba old_color, Rgba new_color);

/// @brief Draws a solid color filled circle
/// @param [in] c Center of the circle.
/// @param [in] r Radius of the circle.
/// @param [in] color Color of the circle.
void DrawCircle(Vec2Si32 c, Si32 r, Rgba color);

/// @brief Draws a solid color filled circle to a sprite
/// @param [in] to_sprite Sprite to draw the circle on.
/// @param [in] c Center of the circle.
/// @param [in] r Radius of the circle.
/// @param [in] color Color of the circle.
void DrawCircle(Sprite to_sprite, Vec2Si32 c, Si32 r, Rgba color);

/// @brief Draws a solid color filled oval
/// @param [in] c Center of the oval.
/// @param [in] r Radii of the oval (x and y).
/// @param [in] color Color of the oval.
void DrawOval(Vec2Si32 c, Vec2Si32 r, Rgba color);

/// @brief Draws a solid color filled oval to a sprite
/// @param [in] to_sprite Sprite to draw the oval on.
/// @param [in] c Center of the oval.
/// @param [in] r Radii of the oval (x and y).
/// @param [in] color Color of the oval.
void DrawOval(Sprite to_sprite, Vec2Si32 c, Vec2Si32 r, Rgba color);

/// @brief Draws a solid color filled oval to a sprite
/// @param [in] to_sprite Sprite to draw the oval on.
/// @param [in] color Color of the oval.
/// @param [in] ll Lower-left corner of the bounding box.
/// @param [in] ur Upper-right corner of the bounding box.
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

/// @brief Calculate the position of a point on the edge of a block
/// @param [in] lower_left_pos Lower-left block corner position.
/// @param [in] size Block size.
/// @param [in] corner_radius External radius of block corners.
/// @param [in] direction Direction vector from the center of the block to the edge.
/// @return Position of the point on the edge of the block.
Vec2F BlockEdgePos(Vec2F lower_left_pos, Vec2F size, float corner_radius, Vec2F direction);

/// @brief Show the current backbuffer and update the input state
void ShowFrame();

/// @brief Takes a picture of the frame that is about to be shown
/// @return A software sprite holding the frame, empty if there is nothing to
/// read, the reason for which goes to the log
/// @details Call it before ShowFrame(), while the frame is still being built.
/// In software mode the picture is a copy of the backbuffer, and its size is
/// the backbuffer size. In hardware mode the frame is assembled a second time
/// into a texture, so the picture has the size of the window and shows the
/// hardware sprites, letterbox bars and all, exactly as the window would.
/// Nothing is written to disk: save the sprite with Sprite::Save if a file is
/// what is wanted.
Sprite Screenshot();

/// @brief Clear the backbuffer with black color
void Clear();

/// @brief Clear the backbuffer with the color specified
/// @param [in] color Color to clear the backbuffer with.
void Clear(Rgba color);

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_DRAWING_H_
