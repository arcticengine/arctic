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

#ifndef ENGINE_SOUND_TASK_H_
#define ENGINE_SOUND_TASK_H_

#include <atomic>

#include "engine/arctic_types.h"
#include "engine/transform3f.h"
#include "engine/easy_sound.h"

namespace arctic {

/// @addtogroup global_sound
/// @{

enum class SoundTaskAction {
  kStart = 0,
  kStop = 1,
  kSetHeadLocation = 2,
  kSetLocation = 3,
  kStart3d = 4
};

struct ChannelPlaybackState {
  double play_position = 0.0;
  double delay = 0.0;
  float acc = 0.f;

  void Clear() {
    play_position = 0.0;
    delay = 0.0;
    acc = 0.f;
  }
};

struct SoundTask {
  static constexpr Ui64 kInvalidSoundTaskUid = 0;
  std::atomic<Ui64> uid = ATOMIC_VAR_INIT(kInvalidSoundTaskUid);
  Ui64 target_uid = kInvalidSoundTaskUid;
  Sound sound;
  float volume = 1.0f;
  Si32 next_position = 0;
  Transform3F location;
  ChannelPlaybackState channel_playback_state[2];
  SoundTaskAction action = SoundTaskAction::kStart;
  bool is_3d = false;
  std::atomic<bool> is_playing = ATOMIC_VAR_INIT(false);

  void Clear(Ui64 in_uid) {
    uid = in_uid;
    target_uid = kInvalidSoundTaskUid;
    sound.Clear();
    volume = 1.0f;
    next_position = 0;
    action = SoundTaskAction::kStart;
    is_3d = false;
    is_playing = false;
  }
};

/// @}

}  // namespace arctic

#endif  // ENGINE_SOUND_TASK_H_


