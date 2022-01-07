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

class SoundHandle {
  Ui64 uid_;
  SoundTask* sound_task_;
 public:
  SoundHandle(SoundTask *sound_task);
  SoundHandle();
  SoundHandle(const SoundHandle &h);

  SoundHandle& operator=(const SoundHandle& other);

  bool IsPlaying() const;
  bool IsValid() const;

  Ui64 GetUid() const {
    return uid_;
  }

  inline static SoundHandle Invalid() {
    return SoundHandle(nullptr);
  }
};

/// @}

}  // namespace arctic

#endif  // ENGINE_SOUND_HANDLE_H_


