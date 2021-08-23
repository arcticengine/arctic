// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2021 The Lasting Curator
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

#include "engine/arctic_mixer.h"
#include "engine/arctic_platform_sound.h"

namespace arctic {

SoundMixerState g_sound_mixer_state;

SoundHandle StartSound(Sound sound, float volume) {
  if (sound.GetInstance()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      SoundHandle handle(buffer);
      buffer->sound = sound;
      buffer->volume = volume;
      buffer->sound.GetInstance()->IncPlaying();
      buffer->action = SoundTask::kStart;  //-V1048
      buffer->is_playing = true;
      g_sound_mixer_state.AddSoundTask(buffer);
      return handle;
    }
  }
  return SoundHandle::Invalid();
}

void StopSound(Sound sound) {
  if (sound.GetInstance()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      buffer->sound = sound;
      buffer->volume = 0.f;
      buffer->action = SoundTask::kStop;
      g_sound_mixer_state.AddSoundTask(buffer);
    }
  }
}

void StopSound(const SoundHandle &handle) {
  if (handle.IsValid()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      buffer->volume = 0.f;
      buffer->action = SoundTask::kStop;
      buffer->target_uid = handle.GetUid();
      g_sound_mixer_state.AddSoundTask(buffer);
    }
  }
}

void SetSoundListenerLocation(Transform3F location) {
  SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
  if (buffer) {
    buffer->location = location;
    buffer->action = SoundTask::kSetHeadLocation;
    g_sound_mixer_state.AddSoundTask(buffer);
  }
}

void SetSoundSourcePosition(Sound sound, Vec3F position) {
  if (sound.GetInstance()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      buffer->sound = sound;
      buffer->location.displacement = position;
      buffer->action = SoundTask::kSetLocation;
      g_sound_mixer_state.AddSoundTask(buffer);
    }
  }
}

void SetSoundSourcePosition(const SoundHandle &handle, Vec3F position) {
  if (handle.IsValid()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      buffer->location.displacement = position;
      buffer->action = SoundTask::kSetLocation;
      buffer->target_uid = handle.GetUid();
      g_sound_mixer_state.AddSoundTask(buffer);
    }
  }
}


SoundHandle StartSoundAtPosition(Sound sound, float volume, Vec3F position) {
  if (sound.GetInstance()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      SoundHandle handle(buffer);
      buffer->sound = sound;
      buffer->volume = volume;
      buffer->next_position = 0;
      buffer->sound.GetInstance()->IncPlaying();
      buffer->is_3d = true;
      buffer->location.displacement = position;
      buffer->action = SoundTask::kStart3d;
      buffer->is_playing = true;
      g_sound_mixer_state.AddSoundTask(buffer);
      return handle;
    }
  }
  return SoundHandle::Invalid();
}


void SetMasterVolume(float volume) {
  g_sound_mixer_state.master_volume.store(volume);
}

float GetMasterVolume() {
  return g_sound_mixer_state.master_volume.load();
}

}  // namespace arctic
