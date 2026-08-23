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

/// @brief Enables/disables Y-coordinate inversion. By default Y-axis is directed upward.
/// @param is_inverse If true, inverts the Y-axis; if false, Y-axis is directed upward (default)
void SetInverseY(bool is_inverse);

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
