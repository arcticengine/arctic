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


#include "engine/sound_handle.h"
#include "engine/sound_task.h"

namespace arctic {

  SoundHandle::SoundHandle(SoundTask *sound_task)
      : uid_(sound_task ? (Ui64)sound_task->uid : SoundTask::kInvalidSoundTaskUid)
      , sound_task_(sound_task) {
  }

  SoundHandle::SoundHandle()
      : uid_(SoundTask::kInvalidSoundTaskUid)
      , sound_task_(nullptr) {
  }

  SoundHandle::SoundHandle(const SoundHandle &h)
      : uid_(h.uid_)
      , sound_task_(h.sound_task_) {

  }

  bool SoundHandle::IsPlaying() const {
    if (IsValid()) {
      bool is_playing = sound_task_->is_playing;
      if (IsValid()) {
        return is_playing;
      }
    }
    return false;
  }

  bool SoundHandle::IsValid() const {
    return (sound_task_ != nullptr
            && uid_ != SoundTask::kInvalidSoundTaskUid
            && sound_task_->uid == uid_);
  }

}  // namespace arctic


