// The MIT License (MIT)
//
// Copyright (c) 2019 Huldra
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

#ifndef ENGINE_ARCTIC_MIXER_H_
#define ENGINE_ARCTIC_MIXER_H_

#include <deque>  // NOLINT
#include <mutex>  // NOLINT
#include <vector>

#include "engine/arctic_types.h"
#include "engine/easy_sound.h"

namespace arctic {

struct SoundBuffer {
  enum Action {
    kStart = 0,
    kStop = 1
  };
  easy::Sound sound;
  float volume = 1.0f;
  Si32 next_position = 0;
  Action action = kStart;
};

struct SoundMixerState {
  std::atomic<bool> do_quit = ATOMIC_VAR_INIT(false);

  std::atomic<bool> is_ok = ATOMIC_VAR_INIT(true);
  std::mutex error_mutex;
  // Mutex-protected state begin
  std::string error_description = "Error description is not set.";
  // Mutex-protected state end

  // Mixer-only state begin
  std::atomic<float> master_volume = ATOMIC_VAR_INIT(0.7f);
  std::vector<SoundBuffer> buffers;
  // Mixer-only state end

  std::atomic<Si64> task_pushers = ATOMIC_VAR_INIT(0);
  std::mutex task_mutex;
  // Mutex-protected state begin
  std::atomic<Ui64> task_count;
  std::deque<SoundBuffer> tasks;
  // Mutex-protected state end

  void SetError(std::string description) {
    std::lock_guard<std::mutex> lock(error_mutex);
    error_description = description;
    is_ok = false;
  }

  bool IsOk() {
    return is_ok;
  }

  std::string GetErrorDescription() {
    std::lock_guard<std::mutex> lock(error_mutex);
    return error_description;
  }

  void AddSoundTask(const SoundBuffer &buffer) {
    while (true) {
      Si64 starts = task_pushers.load();
      if (starts >= 0) {
        if (task_pushers.compare_exchange_strong(starts, starts + 1)) {
          break;
        }
      }
    }
    {
      std::lock_guard<std::mutex> lock(task_mutex);
      tasks.push_back(buffer);
    }
    task_pushers.fetch_add(-1);
    task_count.fetch_add(1);
  }

  void InputTasksToMixerThread() {
    if (!task_count.load()) {
      return;
    }
    Si64 exp = 0;
    if (!task_pushers.compare_exchange_strong(exp, -1)) {
      return;
    }
    std::lock_guard<std::mutex> lock(task_mutex);
    for (size_t i = 0; i < tasks.size(); ++i) {
      SoundBuffer &task = tasks[i];
      switch (task.action) {
      case SoundBuffer::kStart:
        buffers.push_back(task);
        break;
      case SoundBuffer::kStop:
        for (size_t idx = 0; idx < buffers.size(); ++idx) {
          SoundBuffer &buffer = buffers[idx];
          if (buffer.sound.GetInstance() == task.sound.GetInstance()) {
            buffer.sound.GetInstance()->DecPlaying();
            if (idx != buffers.size() - 1) {
              buffers[idx] = buffers[buffers.size() - 1];
            }
            buffers.pop_back();
            idx--;
          }
        }
        break;
      }
    }
    tasks.clear();
    task_count.store(0);
    task_pushers.store(0);
  }
};

}  // namespace arctic

#endif  // ENGINE_ARCTIC_MIXER_H_
