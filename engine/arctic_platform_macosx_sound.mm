// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "engine/arctic_platform_def.h"

#ifdef ARCTIC_PLATFORM_MACOSX

#import <Cocoa/Cocoa.h>

#include <AudioToolbox/AudioToolbox.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>  // NOLINT
#include <vector>

#include "engine/engine.h"
#include "engine/scalar_math.h"
#include "engine/arctic_mixer.h"
#include "engine/arctic_platform.h"

namespace arctic {

extern SoundMixerState g_sound_mixer_state;

class SoundPlayerImpl {
public:
  AudioUnit output_unit = {0};
  std::vector<Si16> tmp;
  double starting_frame_count = 0.0;
  bool is_initialized = false;
  void Initialize();
  void Deinitialize();
  ~SoundPlayerImpl() {
    Deinitialize();
  }
};

void SoundPlayer::Initialize() {
  if (!impl) {
    impl = new SoundPlayerImpl;
  }
  impl->Initialize();
}

void SoundPlayer::Deinitialize() {
  if (impl) {
    impl->Deinitialize();
  }
}

SoundPlayer::~SoundPlayer() {
  if (impl) {
    delete impl;
    impl = nullptr;
  }
}

void CheckStatus(OSStatus status, const char *message) {
  if (status == noErr) {
    return;
  }
  char code[20];
  *(UInt32 *)(code + 1) = static_cast<Ui32>(ToBe(status));
  if (isprint(code[1])
      && isprint(code[2])
      && isprint(code[3])
      && isprint(code[4])) {
    code[0] = '\'';
    code[5] = '\'';
    code[6] = '\0';
  } else {
    snprintf(code, 20, "%d", (int)status);
  }
  Fatal(message, code);
}

OSStatus SoundRenderProc(void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData);

OSStatus SoundRenderProc(void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData) {
  SoundPlayerImpl *mixer = (SoundPlayerImpl*)inRefCon;
  Si32 buffer_samples_per_channel = static_cast<Si32>(inNumberFrames);
  Float32 *mix_l = (Float32*)ioData->mBuffers[0].mData;
  Float32 *mix_r = (Float32*)ioData->mBuffers[1].mData;
  Si32 mix_stride = 1;
  g_sound_mixer_state.InputTasksToMixerThread();
  float master_volume = static_cast<float>(
    g_sound_mixer_state.master_volume.load() / 32767.0);

  if (mixer->tmp.size() < buffer_samples_per_channel * 2) {
    mixer->tmp.resize(buffer_samples_per_channel * 2);
  }

  if (g_sound_mixer_state.buffers.empty()) {
    Si32 mix_idx = 0;
    for (Si32 i = 0; i < buffer_samples_per_channel; ++i) {
      mix_l[mix_idx] = 0.f;
      mix_r[mix_idx] = 0.f;
      mix_idx += mix_stride;
    }
  }

  for (Ui32 idx = 0; idx < g_sound_mixer_state.buffers.size(); ++idx) {
    SoundTask &sound = *g_sound_mixer_state.buffers[idx];
    if (sound.is_3d) {
      if (idx == 0) {
        for (Si32 i = 0; i < buffer_samples_per_channel; ++i) {
          mix_l[i] = 0.f;
          mix_r[i] = 0.f;
        }
      }
      bool is_over = true;
      for (Si32 channel_idx = 0; channel_idx < 2; ++channel_idx) {
        RenderSound<Float32>(
            &sound, g_sound_mixer_state.head, channel_idx,
            (channel_idx == 0 ? mix_l : mix_r), 1, buffer_samples_per_channel, 44100.0,
            master_volume);
        if (sound.channel_playback_state[channel_idx].play_position * 44100.0 < sound.sound.DurationSamples()) {
          is_over = false;
        }
      }
      if (is_over) {
        sound.sound.GetInstance()->DecPlaying();
        g_sound_mixer_state.ReleaseBufferAt(idx);
        --idx;
      }
    } else {
      Si32 size = sound.sound.StreamOut(sound.next_position,
          buffer_samples_per_channel,
          mixer->tmp.data(),
          buffer_samples_per_channel * 2);
      Si16 *in_data = mixer->tmp.data();
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

      if (sound.next_position == sound.sound.DurationSamples()
          || size == 0) {
        sound.sound.GetInstance()->DecPlaying();
        g_sound_mixer_state.ReleaseBufferAt(idx);
        --idx;
      }
    }
  }

  for (Si32 frame = 0; frame < buffer_samples_per_channel; ++frame) {
    mix_l[frame] = Clamp(mix_l[frame], -1.0f, 1.0f);
    mix_r[frame] = Clamp(mix_r[frame], -1.0f, 1.0f);
  }
  return noErr;
}

void SoundPlayerImpl::Initialize() {
  if (is_initialized) {
    return;
  }
  tmp.resize(2 << 20);
  g_sound_mixer_state.InputTasksToMixerThread();

  AudioComponentDescription outputcd = {0};
  outputcd.componentType = kAudioUnitType_Output;
  outputcd.componentSubType = kAudioUnitSubType_DefaultOutput;
  outputcd.componentManufacturer = kAudioUnitManufacturer_Apple;

  AudioComponent comp = AudioComponentFindNext(NULL, &outputcd);
  if (comp == NULL) {
    printf("can't get output unit");
    exit(-1);
  }
  OSStatus status = AudioComponentInstanceNew(comp, &output_unit);
  CheckStatus(status, "Couldn't open component for output_unit");

  AURenderCallbackStruct render;
  render.inputProc = SoundRenderProc;
  render.inputProcRefCon = this;
  status = AudioUnitSetProperty(output_unit,
      kAudioUnitProperty_SetRenderCallback,
      kAudioUnitScope_Input,
      0,
      &render,
      sizeof(render));
  CheckStatus(status, "AudioUnitSetProperty failed");

  status = AudioUnitInitialize(output_unit);
  CheckStatus(status, "Couldn't initialize output unit");

  status = AudioOutputUnitStart(output_unit);
  CheckStatus(status, "Couldn't start output unit");

  is_initialized = true;
}

void SoundPlayerImpl::Deinitialize() {
  if (is_initialized) {
    AudioOutputUnitStop(output_unit);
    AudioUnitUninitialize(output_unit);
    AudioComponentInstanceDispose(output_unit);
    is_initialized = false;
  }
}

}  // namespace arctic

#endif
