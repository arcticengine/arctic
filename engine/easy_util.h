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

#ifndef ENGINE_EASY_UTIL_H_
#define ENGINE_EASY_UTIL_H_

#include <random>
#include <string>

#include "engine/arctic_types.h"
#include "engine/vec2si32.h"

namespace arctic {

class Sprite;
class HwSprite;

/// @addtogroup global_utility
/// @{

/// @brief Get the window size in actual pixels of the OS
/// @details These are real pixels of the display, not points: on a HiDPI or
/// Retina screen the number is twice what the window manager calls the window
/// size. It follows the window, so it changes whenever the window is resized or
/// goes full screen, while ScreenSize() does not.
Vec2Si32 WindowSize();

/// @brief Get the backbuffer resolution in pixels
/// @details This is what ResizeScreen last set, which the engine never changes
/// on its own, so it is the size drawing coordinates are measured in and it
/// stays put while the window is resized. The backbuffer is scaled into the
/// window keeping its aspect ratio, with bars on the sides when the ratios
/// differ. Code that renders in the resolution of the window, a 3d pass for
/// instance, follows WindowSize() and calls ResizeScreen() when it changes.
Vec2Si32 ScreenSize();

/// @brief Set the backbuffer resolution in pixels
/// @param width The desired width of the backbuffer in pixels
/// @param height The desired height of the backbuffer in pixels
/// @details The backbuffer is recreated and its contents are lost. Pass
/// WindowSize() to draw in real pixels of the display, and do it only when the
/// window size actually changes, as recreating textures every frame is not free.
void ResizeScreen(const Si32 width, const Si32 height);

/// @brief Set the backbuffer resolution in pixels
/// @param size The desired size of the backbuffer in pixels
/// @details The backbuffer is recreated and its contents are lost. Pass
/// WindowSize() to draw in real pixels of the display, and do it only when the
/// window size actually changes, as recreating textures every frame is not free.
void ResizeScreen(const Vec2Si32 size);

/// @brief Flips the finished frame upside down when it is composed
/// @param is_inverse true to flip the frame, false for the normal picture (default)
/// @details This is not a coordinate mode and it will not turn the engine into a
/// "zero at the top" one. Nothing about drawing changes: coordinates still count
/// from the bottom-left corner with y upward, the mouse still answers in those
/// same coordinates, and the GUI still sits where it sat. What changes is the
/// texture mapping of the composition step, so the whole software backbuffer is
/// mirrored vertically and every HwSprite is mirrored along with it: text comes
/// out written backwards and a click lands nowhere near the thing under the
/// cursor. It exists for a frame that has to be presented flipped, a video
/// encoder or a projector setup for example, and it acts only where the frame is
/// composed on the GPU.
///
/// To place a layout measured from the top of the screen, convert the
/// coordinates instead: see FromTopLeft() and ToTopLeft() below.
void SetInverseY(bool is_inverse);

/// @brief Converts a point measured from the top-left corner of the screen
/// @param pos_from_top_left A point whose y counts pixels downward from the top
/// @return The same point in engine coordinates, y counted upward from the bottom
/// @details Engine coordinates count from the bottom-left corner of the
/// backbuffer with y growing upward, while a mock-up, a window toolkit or a web
/// page counts from the top-left corner downward. Convert once, at the boundary
/// between the two, instead of writing ScreenSize().y - y at every call site.
///
/// The conversion is its own inverse, so FromTopLeft(FromTopLeft(p)) == p, and
/// ToTopLeft() is the same arithmetic under a name that reads better going the
/// other way. Row 0 from the top is the topmost row of pixels, ScreenSize().y-1
/// in engine coordinates.
///
/// This overload moves a *point*. Use the overload taking a size for a picture
/// or a caption whose *top* edge is the measured one.
Vec2Si32 FromTopLeft(Vec2Si32 pos_from_top_left);

/// @brief Converts the top-left corner of a box into the engine position of the box
/// @param pos_from_top_left The top-left corner of the box, y counted downward from the top
/// @param size The size of the box in pixels
/// @return The bottom-left corner of the box in engine coordinates
/// @details This is the overload a layout actually needs, and the one people
/// forget: "40 pixels below the top and 120 pixels tall" is the *top* edge, so
/// the position a Draw call wants is size.y lower. The result equals
/// ScreenSize().y - pos_from_top_left.y - size.y, and the box then occupies rows
/// from the result up to the result plus size minus one pixel.
///
/// The conversion is its own inverse for the same size, so a position converted
/// twice comes back.
Vec2Si32 FromTopLeft(Vec2Si32 pos_from_top_left, Vec2Si32 size);

/// @brief Converts a point measured from the top-left corner of a sprite
/// @param to_sprite The sprite the point is measured in, normally a draw target
/// @param pos_from_top_left A point whose y counts pixels downward from the top of the sprite
/// @return The same point in the coordinates the sprite is drawn to
/// @details The same conversion as the screen one, done inside a sprite: a
/// sprite is its own little backbuffer with zero at its bottom-left corner. An
/// empty sprite gives the point back unchanged.
Vec2Si32 FromTopLeft(const Sprite &to_sprite, Vec2Si32 pos_from_top_left);

/// @brief Converts the top-left corner of a box measured inside a sprite
/// @param to_sprite The sprite the box is measured in, normally a draw target
/// @param pos_from_top_left The top-left corner of the box, y counted downward from the top
/// @param size The size of the box in pixels
/// @return The bottom-left corner of the box in the coordinates of the sprite
Vec2Si32 FromTopLeft(const Sprite &to_sprite, Vec2Si32 pos_from_top_left,
  Vec2Si32 size);

/// @brief Converts an engine point into one measured from the top-left corner
/// @param engine_pos A point in engine coordinates, y counted upward
/// @return The same point with y counting pixels downward from the top
/// @details For reporting a position the way a mock-up or a tool counts it. The
/// arithmetic is the one FromTopLeft() does; the two names exist so that the
/// direction of the conversion is readable at the call site.
Vec2Si32 ToTopLeft(Vec2Si32 engine_pos);

/// @brief Converts a box position into the position of its top-left corner
/// @param engine_pos The bottom-left corner of the box in engine coordinates
/// @param size The size of the box in pixels
/// @return The top-left corner of the box, y counting downward from the top
Vec2Si32 ToTopLeft(Vec2Si32 engine_pos, Vec2Si32 size);

/// @brief Tells whether a point is inside a sprite drawn at a given position
/// @param sprite The sprite as it was drawn
/// @param drawn_at The position passed to Draw, where the pivot of the sprite landed
/// @param point The point to test, MousePos() for instance
/// @return true when the point is within the rectangle the sprite covered
/// @details A sprite is placed by its pivot, so the rectangle it covers starts
/// at drawn_at minus the pivot, and a hand-written test that treats drawn_at as
/// the bottom-left corner misses by the pivot -- which is how a click stops
/// hitting a character loaded from a tga. This function accounts for the pivot
/// and for the reference rectangle of the sprite, so the answer matches what
/// Draw did.
///
/// The test is the bounding rectangle, not the shape: a transparent pixel of the
/// sprite still counts as a hit. Look at the alpha of the pixel afterwards when
/// that matters. An empty sprite covers nothing and answers false. A sprite
/// drawn scaled, rotated or into another sprite is a different rectangle than
/// the one this function assumes.
bool IsPointInSprite(const Sprite &sprite, Vec2Si32 drawn_at, Vec2Si32 point);

/// @brief Tells whether a point is inside a hardware sprite drawn at a position
/// @param sprite The sprite as it was drawn
/// @param drawn_at The position passed to Draw, where the pivot of the sprite landed
/// @param point The point to test, MousePos() for instance
/// @return true when the point is within the rectangle the sprite covered
/// @details The HwSprite counterpart of the Sprite overload, with the same
/// rules. Note that HwSprite::Load leaves the pivot at zero even for a tga,
/// while Sprite::Load takes the pivot from the file.
bool IsPointInSprite(const HwSprite &sprite, Vec2Si32 drawn_at, Vec2Si32 point);

/// @brief Returns time in seconds since the game start
/// @return Time in seconds as a double
double Time();

/// @brief Seeds the random number generators of the calling thread
/// @param seed The seed value, any value will do
/// @details Every Random function below draws from thread-local generators, so
/// a seed set here holds for the calling thread only, and a thread that was
/// never seeded starts from the clock. Seeding the same value again replays the
/// same sequence of numbers, which is what a reproducible level generator or a
/// test needs. A seed starts a sequence from its beginning; to leave a sequence
/// and come back to the very number it was about to give, use GetRandomState and
/// SetRandomState instead.
void SetRandomSeed(Ui64 seed);

/// @brief Everything the random number generators of one thread are about to give
/// @details The Random functions draw from four generators, one per width, and a
/// state holds all four of them at once, so a state put back restores every
/// Random function to the number it was about to return. The state is a value:
/// copy it, keep several of them side by side and switch between them to run
/// independent sequences on one thread, or hand one to another thread.
///
/// A state is about ten kilobytes of numbers and copying it is a copy of those
/// numbers and nothing else, so switching sequences per chunk or per entity is
/// cheap. For a save file there is ToString, some twenty five kilobytes of text,
/// and a state read back with FromString continues where it was taken, in
/// another run of the program as well.
class RandomState {
 public:
  /// @brief Writes the state as text
  /// @return The state as a line of numbers, suitable for a save file
  std::string ToString() const;

  /// @brief Reads a state written by ToString
  /// @param text The text to read
  /// @return true if the text was a state; the state is left as it was if not
  bool FromString(const std::string &text);

 private:
  friend class Engine;

  std::independent_bits_engine<std::mt19937_64, 64, Ui64> rnd_64_;
  std::independent_bits_engine<std::mt19937_64, 32, Ui64> rnd_32_;
  std::independent_bits_engine<std::mt19937_64, 16, Ui64> rnd_16_;
  std::independent_bits_engine<std::mt19937_64, 8, Ui64> rnd_8_;
};

/// @brief Returns the state of the random number generators of the calling thread
/// @return The state, to be put back with SetRandomState
/// @details A thread that never drew a number and was never seeded is seeded
/// from the clock here, so the state returned is always a usable one.
RandomState GetRandomState();

/// @brief Puts a state returned by GetRandomState back
/// @param state The state to continue from
/// @details The generators of the calling thread continue from the state, which
/// need not come from this thread or even from this run of the program.
void SetRandomState(const RandomState &state);

/// @brief Returns a random number in range [min,max]
/// @param min The minimum value of the range (inclusive)
/// @param max The maximum value of the range (inclusive)
/// @return A random Si64 number within the specified range
Si64 Random(Si64 min, Si64 max);

/// @brief Returns a random number in range [min,max]
/// @param min The minimum value of the range (inclusive)
/// @param max The maximum value of the range (inclusive)
/// @return A random Si32 number within the specified range
Si32 Random32(Si32 min, Si32 max);

/// @brief Returns a random Ui64
/// @return A random Ui64 number
Ui64 Random64();

/// @brief Returns a random Ui32
/// @return A random Ui32 number
Ui32 Random32();

/// @brief Returns a random Ui16
/// @return A random Ui16 number
Ui16 Random16();

/// @brief Returns a random Ui8
/// @return A random Ui8 number
Ui8 Random8();

/// @brief Returns a random float
/// @return A random float number in range [0.f, 1.f)
float RandomF();

/// @brief Returns a random signed float
/// @return A random signed float number in range [-1.f, 1.f)
float RandomSF();

/// @brief Returns a random double
/// @return A random double number in range [0.0, 1.0)
double RandomD();

/// @brief Returns a random signed double
/// @return A random signed double number in range [-1.0, 1.0)
double RandomSD();

/// @brief Waits for the time specified before returning
/// @param duration_seconds The duration to wait in seconds
void Sleep(double duration_seconds);

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_UTIL_H_
