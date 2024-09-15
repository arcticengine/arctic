// The MIT License (MIT)
//
// Copyright (c) 2017 - 2021 Huldra
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

#ifndef ENGINE_SOUND_HANDLE_H_
#define ENGINE_SOUND_HANDLE_H_

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_sound
/// @{

struct SoundTask;

/// @brief A handle to a sound task.
class SoundHandle {
  Ui64 uid_;
  SoundTask* sound_task_;
 public:
  /// @brief Constructs a SoundHandle with a given SoundTask.
  /// @param sound_task Pointer to the SoundTask.
  SoundHandle(SoundTask *sound_task);

  /// @brief Default constructor.
  SoundHandle();

  /// @brief Copy constructor.
  /// @param h The SoundHandle to copy from.
  SoundHandle(const SoundHandle &h);

  /// @brief Assignment operator.
  /// @param other The SoundHandle to assign from.
  /// @return Reference to this SoundHandle.
  SoundHandle& operator=(const SoundHandle& other);

  /// @brief Checks if the sound is currently playing.
  /// @return True if the sound is playing, false otherwise.
  bool IsPlaying() const;

  /// @brief Checks if the SoundHandle is valid.
  /// @return True if the SoundHandle is valid, false otherwise.
  bool IsValid() const;

  /// @brief Gets the unique identifier of the SoundHandle.
  /// @return The unique identifier.
  Ui64 GetUid() const {
    return uid_;
  }

  /// @brief Creates an invalid SoundHandle.
  /// @return An invalid SoundHandle.
  inline static SoundHandle Invalid() {
    return SoundHandle(nullptr);
  }
};

/// @}

}  // namespace arctic

#endif  // ENGINE_SOUND_HANDLE_H_


