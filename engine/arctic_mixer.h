// The MIT License (MIT)
//
// Copyright (c) 2017 - 2021 Huldra
// Copyright (c) 2013 - 2021 Mikle
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

#define _USE_MATH_DEFINES
#include <math.h> 
#include <deque>  // NOLINT
#include <mutex>  // NOLINT
#include <string>
#include <vector>
#include <array>

#include "engine/arctic_types.h"
#include "engine/easy_sound.h"
#include "engine/mtq_mpsc_vinfarr.h"
#include "engine/mtq_spmc_array.h"
#include "engine/mtq_mpmc_befsbfsp_allocator.h"
#include "engine/sound_handle.h"
#include "engine/sound_task.h"
#include "engine/vec3f.h"
#include "engine/quaternion.h"
#include "engine/transform3f.h"

namespace arctic {

/// @addtogroup global_sound
/// @{

struct SoundListenerHead {
  struct Ear {
    Vec3F pos;
    Vec3F max_vec;
  };

  Transform3F loc;
  std::array<Ear, 2> ears;

  const float radius = 0.09f;
  const float ear_angle_degrees = 90.f;
  const float ear_max_angle_degrees = 60.f;

  void UpdateEars() {
    Vec3F dir(0.0f, 0.f, radius);
    Vec3F unit(0.0f, 0.f, 1.f);
    for (Si32 ear_idx = 0; ear_idx < 2; ++ear_idx) {
      float sign = static_cast<float>(1 - ear_idx * 2);
      QuaternionF rot(Vec3F(0.0f, 1.0f, 0.0f), sign * ear_angle_degrees * (float)M_PI / 180.f);
      QuaternionF rot_max(Vec3F(0.0f, 1.0f, 0.0f), sign * ear_max_angle_degrees * (float)M_PI / 180.f);
      Vec3F toEar = loc.rotation.Rotate(rot.Rotate(dir));
      Ear &ear = ears[ear_idx];
      ear.pos = loc.displacement + toEar;
      ear.max_vec = loc.rotation.Rotate(rot_max.Rotate(unit));
    }
  }
};


struct SoundSource {
  Transform3F loc;
  ChannelPlaybackState channel_playback_state[2];

  void Clear() {
    loc.Clear();
    channel_playback_state[0].Clear();
    channel_playback_state[1].Clear();
  }
};


template <class T>
void RenderSound(SoundTask *sound, const SoundListenerHead &head, Si32 channel_idx,
                 T *dst_buffer, Si32 dst_stride, Si32 dst_size_samples, double dst_sample_rate,
                 float master_volume) {
  const double sonic_speed = 343.0;
  const double safe_dst_sample_rate = std::max(512.0, dst_sample_rate);
  const double inv_dst_sample_rate = 1.0 / safe_dst_sample_rate;
  const double delay_change_speed = (100.0 / sonic_speed) / safe_dst_sample_rate;
  const float vol_mul_at_zero = 0.43f; // -7.22 db
  const float vol_mul_at_min = 0.43f / 1.122f;//0.19; // -7.22 -7.22 db
  const double src_sample_rate = 44100.0;
  const Si32 src_stride = 2;

  ChannelPlaybackState *channel = &sound->channel_playback_state[channel_idx];

  Vec3F to_src = sound->location.displacement - head.ears[channel_idx].pos;
  float max_dot = Dot(head.ears[channel_idx].max_vec, NormalizeSafe(to_src));
  float ear_distance = std::max(0.0f, Length(to_src));
  float nearest_distance = std::max(0.0f, Length(head.loc.displacement - sound->location.displacement) - head.radius);
  double ear_delay = std::max(0.0f, ear_distance - nearest_distance) / sonic_speed;
  float smooth_max_dot = copysign((abs(max_dot)), max_dot); // sqrtf?
  float vol_mul = vol_mul_at_zero + smooth_max_dot *
    (max_dot > 0.f ? (1.f - vol_mul_at_zero) : (vol_mul_at_zero - vol_mul_at_min));
  float signal_k = (smooth_max_dot > 0.f ? 1.f : (1.f + smooth_max_dot * 0.9f));
  float acc_k = 1.f - signal_k;
  float volume = sound->volume * master_volume * vol_mul * ((1.f / std::max(ear_distance, 0.1f)));

  Si16 *src_buffer = sound->sound.GetInstance()->GetWavData();
  Si32 src_size_samples = sound->sound.DurationSamples();

  double src_start_pos = channel->play_position;
  double cur_delay = channel->delay;
  float acc = channel->acc;
  Si32 dst_sample_idx = 0;
  for (dst_sample_idx = 0; dst_sample_idx < dst_size_samples; ++dst_sample_idx) {
    if (cur_delay != ear_delay) {
      double delay_diff = ear_delay - cur_delay;
      if (std::abs(delay_diff) > delay_change_speed) {
        cur_delay += copysign(delay_change_speed, delay_diff);
      } else {
        cur_delay = ear_delay;
      }
    }
    double dst_time = dst_sample_idx * inv_dst_sample_rate;
    double time_offset_in_src = dst_time + src_start_pos - cur_delay;
    Si32 src_sample_idx = (Si32)(time_offset_in_src * src_sample_rate);
    if (src_sample_idx >= 0 && src_sample_idx < src_size_samples) {
      acc = acc * acc_k + src_buffer[src_stride * src_sample_idx] * signal_k;
      dst_buffer[dst_stride * dst_sample_idx] += (T)(acc * volume);
    }
  }
  channel->play_position = dst_sample_idx * inv_dst_sample_rate + src_start_pos;
  channel->delay = cur_delay;
  channel->acc = acc;
}

struct SoundMixerState {
  std::atomic<bool> do_quit = ATOMIC_VAR_INIT(false);
  std::atomic<bool> is_ok = ATOMIC_VAR_INIT(true);
  std::mutex error_mutex;
  MpmcBestEffortFixedSizeBufferFixedSizePool<8, 4080> page_pool;
  MpscVirtInfArray<SoundTask*, TuneDeletePayloadFlag<true>, TuneMemoryPoolFlag<true>> tasks;
  SpmcArray<SoundTask, true> pool;
  static constexpr Si32 kPoolSize = 1024;

  // Mutex-protected state begin
  std::string error_description = "Error description is not set.";
  // Mixer-only state begin
  std::atomic<Ui64> next_uid = ATOMIC_VAR_INIT(2);
  std::atomic<float> master_volume = ATOMIC_VAR_INIT(0.7f);
  std::vector<SoundTask*> buffers;
  SoundListenerHead head;
  float compressor_level = 1.f;

  SoundMixerState()
      : tasks(&page_pool)
      , pool(kPoolSize) {
    for (Si32 i = 0; i < kPoolSize; ++i) {
      pool.enqueue(new SoundTask);
    }
  }

  void ReleaseBufferAt(Si32 idx) {
    SoundTask *buffer = buffers[idx];
    buffers[idx] = buffers[buffers.size() - 1];
    buffers.pop_back();
    buffer->uid = SoundTask::kInvalidSoundTaskUid;
    pool.enqueue(buffer);
  }

  void SetError(std::string description) {  //-V813
    std::lock_guard<std::mutex> lock(error_mutex);
    error_description = description;  //-V820
    is_ok = false;
  }

  bool IsOk() {
    return is_ok;
  }

  std::string GetErrorDescription() {
    std::lock_guard<std::mutex> lock(error_mutex);
    return error_description;
  }

  SoundTask *AllocateSoundTask() {
    SoundTask *p = pool.dequeue();
    if (p) {
      p->Clear(next_uid.fetch_add(1));
    }
    return p;
  }
  void AddSoundTask(SoundTask *buffer) {
    if (buffer) {
      tasks.enqueue(buffer);
    }
  }

  void InputTasksToMixerThread() {
    for (Si32 i = 0; i < 512; ++i) {
      SoundTask *task = tasks.dequeue();
      if (task == nullptr) {
        return;
      }
      switch (task->action) {
      case SoundTaskAction::kStart:
        buffers.push_back(task);
        task = nullptr;
        break;
      case SoundTaskAction::kStop:
        {
          Ui64 task_uid = task->target_uid;
          for (Si32 idx = 0; idx < (Si32)buffers.size(); ++idx) {
            SoundTask *buffer = buffers[idx];
            if (task_uid == SoundTask::kInvalidSoundTaskUid
                ? buffer->sound.GetInstance() == task->sound.GetInstance()
                : task_uid == buffer->uid) {
              buffer->sound.GetInstance()->DecPlaying();
              ReleaseBufferAt(idx);
              idx--;
            }
          }
        }
        break;
      case SoundTaskAction::kSetHeadLocation:
        head.loc = task->location;
        head.UpdateEars();
        break;
      case SoundTaskAction::kSetLocation:
        {
          Ui64 task_uid = task->target_uid;
          for (size_t idx = 0; idx < buffers.size(); ++idx) {
            SoundTask *buffer = buffers[idx];
            if (task_uid == SoundTask::kInvalidSoundTaskUid
                ? buffer->sound.GetInstance() == task->sound.GetInstance()
                : task_uid == buffer->uid) {

              buffer->location = task->location;
            }
          }
        }
        break;
      case SoundTaskAction::kStart3d:
        task->is_3d = true;
        task->next_position = 0;
        for (Si32 i = 0; i < 2; ++i) {
          task->channel_playback_state[i].delay = 0.f;
          task->channel_playback_state[i].play_position = 0.f;
          task->channel_playback_state[i].acc = 0.f;
        }
        buffers.push_back(task);
        task = nullptr;
        break;
      }
      if (task && !pool.enqueue(task)) {
        delete task;
      }
    }
  }

  inline float SoftClipSound(float d) {
    constexpr float a = 0.97f;
    constexpr float b = 1.f - a;
    if (d > a) {
      d -= a;
      return (b * d) / (b + d) + a;
    } else if (d < -a) {
      d += a;
      return (b * d) / (b - d) - a;
    } else {
      return d;
    }
  }

  template <class T>
  void MixSound(T *mix_l, T *mix_r, Si32 mix_stride, Si32 buffer_samples_per_channel, Si16 *tmp) {
    InputTasksToMixerThread();
    float master_volume = static_cast<float>(
      this->master_volume.load() / 32767.0);
    if (buffers.empty()) {
      Si32 mix_idx = 0;
      for (Si32 i = 0; i < buffer_samples_per_channel; ++i) {
        mix_l[mix_idx] = 0.f;
        mix_r[mix_idx] = 0.f;
        mix_idx += mix_stride;
      }
    }

    for (Ui32 idx = 0; idx < buffers.size(); ++idx) {
      SoundTask &sound = *buffers[idx];
      if (sound.is_3d) {
        if (idx == 0) {
          Si32 mix_idx = 0;
          for (Si32 i = 0; i < buffer_samples_per_channel; ++i) {
            mix_l[mix_idx] = 0.f;
            mix_r[mix_idx] = 0.f;
            mix_idx += mix_stride;
          }
        }
        bool is_over = true;
        for (Si32 channel_idx = 0; channel_idx < 2; ++channel_idx) {
          RenderSound<T>(
              &sound, head, channel_idx,
              (channel_idx == 0 ? mix_l : mix_r), 1, buffer_samples_per_channel, 44100.0,
              master_volume);
          if (sound.channel_playback_state[channel_idx].play_position * 44100.0 < sound.sound.DurationSamples()) {
            is_over = false;
          }
        }
        if (is_over) {
          sound.sound.GetInstance()->DecPlaying();
          ReleaseBufferAt(idx);
          --idx;
        }
      } else {
        Si32 size = sound.sound.StreamOut(sound.next_position,
            buffer_samples_per_channel,
            tmp,
            buffer_samples_per_channel * 2);

        Si16 *in_data = tmp;
        float volume = sound.volume * master_volume;
        Si32 mix_idx = 0;
        if (idx == 0) {
          for (Si32 i = 0; i < size; ++i) {
            mix_l[mix_idx] = static_cast<float>(in_data[i * 2]) * volume;
            mix_r[mix_idx] = static_cast<float>(in_data[i * 2 + 1]) * volume;
            mix_idx += mix_stride;
          }
          mix_idx = size * mix_stride;
          for (Si32 i = size; i < buffer_samples_per_channel; ++i) {
            mix_l[mix_idx] = 0.f;
            mix_r[mix_idx] = 0.f;
            mix_idx += mix_stride;
          }
        } else {
          for (Si32 i = 0; i < size; ++i) {
            mix_l[mix_idx] += static_cast<float>(in_data[i * 2]) * volume;
            mix_r[mix_idx] += static_cast<float>(in_data[i * 2 + 1]) * volume;
            mix_idx += mix_stride;
          }
        }
        sound.next_position += size;

        if (size < buffer_samples_per_channel) {
          sound.sound.GetInstance()->DecPlaying();
          ReleaseBufferAt(idx);
          --idx;
        }
      }
    }

    const float Attack = 1.f / (44100.f * 0.005f);
    const float Release = 1.f / (44100.f * 0.2f);
    for (Si32 frame = 0; frame < buffer_samples_per_channel; ++frame) {
      float smax = std::max(abs(mix_l[frame]), abs(mix_r[frame]));
      if (smax > compressor_level) {
        compressor_level = compressor_level * (1.f - Attack) + smax * Attack;
      } else {
        compressor_level = compressor_level * (1.f - Release) + smax * Release;
      }
      float gain_k = (compressor_level > 1.f ? 1.f / compressor_level : 1.f);
      mix_l[frame] = SoftClipSound(mix_l[frame] * gain_k);
      mix_r[frame] = SoftClipSound(mix_r[frame] * gain_k);
    }
  }
};

extern template class MpmcBestEffortFixedSizeBufferFixedSizePool<8, 4080>;
extern template class MpscVirtInfArray<SoundTask*, TuneDeletePayloadFlag<true>, TuneMemoryPoolFlag<true>>;
extern template class SpmcArray<SoundTask, true>;

/// @}

}  // namespace arctic

#endif  // ENGINE_ARCTIC_MIXER_H_
