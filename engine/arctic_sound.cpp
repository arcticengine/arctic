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
#include "engine/easy.h"

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
      buffer->action = SoundTaskAction::kStart;  //-V1048
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
      buffer->action = SoundTaskAction::kStop;
      g_sound_mixer_state.AddSoundTask(buffer);
    }
  }
}

void StopSound(const SoundHandle &handle) {
  if (handle.IsValid()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      buffer->volume = 0.f;
      buffer->action = SoundTaskAction::kStop;
      buffer->target_uid = handle.GetUid();
      g_sound_mixer_state.AddSoundTask(buffer);
    }
  }
}

void SetSoundListenerLocation(Transform3F location) {
  SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
  if (buffer) {
    buffer->location = location;
    buffer->action = SoundTaskAction::kSetHeadLocation;
    g_sound_mixer_state.AddSoundTask(buffer);
  }
}

void SetSoundSourcePosition(Sound sound, Vec3F position) {
  if (sound.GetInstance()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      buffer->sound = sound;
      buffer->location.displacement = position;
      buffer->action = SoundTaskAction::kSetLocation;
      g_sound_mixer_state.AddSoundTask(buffer);
    }
  }
}

void SetSoundSourcePosition(const SoundHandle &handle, Vec3F position) {
  if (handle.IsValid()) {
    SoundTask *buffer = g_sound_mixer_state.AllocateSoundTask();
    if (buffer) {
      buffer->location.displacement = position;
      buffer->action = SoundTaskAction::kSetLocation;
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
      buffer->action = SoundTaskAction::kStart3d;
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

Sound BeepAsync(float duration_seconds, Si32 note) {
  if (duration_seconds < 0.01f) {
    duration_seconds = 0.01f;
  }
  Sound s;
  s.Create(duration_seconds);
  Si32 end = s.DurationSamples();
  Si16 *p = s.RawData();
  float freq = std::pow(2.f, (note - 9.f)/12.f) * 440.f;
  freq = std::min(std::max(freq, 100.f), 22050.f);
  for (Si32 i = 0; i < end; ++i) {
    float t = i * duration_seconds / end;
    float v = std::sin(t * 2.f * float(M_PI) * freq)*32767.f;
    p[i * 2] = (Si16)v;
    p[i * 2 + 1] = (Si16)v;
  }
  float mul = 0.98f;
  for (Si32 i = std::max(0, end - 441); i < end; ++i) {
    p[i * 2] = (Si16)(p[i * 2] * mul);
    p[i * 2 + 1] = (Si16)(p[i * 2 + 1] * mul);
    mul = mul * 0.98f;
  }
  s.Play();
  return s;
}

void Beep(float duration_seconds, Si32 note) {
  ShowFrame();
  Sound s = BeepAsync(duration_seconds, note);
  while (s.IsPlaying()) {
    ;
  }
}

}  // namespace arctic
