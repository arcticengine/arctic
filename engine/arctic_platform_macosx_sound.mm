// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2022 Huldra
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


#import <AppKit/AppKit.h>

#include <AudioToolbox/AudioToolbox.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "engine/engine.h"
#include "engine/log.h"
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

bool SoundPlayer::IsOk() {
  return g_sound_mixer_state.IsOk();
}

std::string SoundPlayer::GetErrorDescription() {
  return g_sound_mixer_state.GetErrorDescription();
}

/// @brief Turns an OSStatus into the four character code CoreAudio prints
static std::string DescribeStatus(OSStatus status) {
  char code[20];
  Ui32 be = static_cast<Ui32>(ToBe(status));
  memcpy(code + 1, &be, sizeof(be));
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
  return std::string(code);
}

/// @brief Records a sound failure without ending the process
/// @return false always, so that a caller can `return SoundFailure(...)`
///
/// A machine with no sound device, or one that keeps it to itself, is a machine
/// the application should still run on: the game is playable without sound and
/// a silent mixer is a far better answer than a process that dies before the
/// first frame. The reason is kept for SoundPlayer::GetErrorDescription and
/// written to the log, so that silence is never a mystery.
static bool SoundFailure(const std::string &message) {
  g_sound_mixer_state.SetError(message);
  *Log() << "Sound is not available: " << message;
  return false;
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
  Float32 *mix_l;
  Float32 *mix_r;
  Si32 mix_stride;
  if (ioData->mNumberBuffers >= 2) {
    mix_l = (Float32*)ioData->mBuffers[0].mData;
    mix_r = (Float32*)ioData->mBuffers[1].mData;
    mix_stride = 1;
  } else {
    mix_l = (Float32*)ioData->mBuffers[0].mData;
    mix_r = mix_l + 1;
    mix_stride = 2;
  }

  if (mixer->tmp.size() < buffer_samples_per_channel * 2) {
    mixer->tmp.resize(buffer_samples_per_channel * 2);
  }

  g_sound_mixer_state.MixSound(mix_l, mix_r, mix_stride, buffer_samples_per_channel, mixer->tmp.data());

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
    SoundFailure("no default output audio unit on this machine");
    return;
  }
  OSStatus status = AudioComponentInstanceNew(comp, &output_unit);
  if (status != noErr) {
    SoundFailure("can't open the output audio unit, CoreAudio says "
      + DescribeStatus(status));
    return;
  }

  AURenderCallbackStruct render;
  render.inputProc = SoundRenderProc;
  render.inputProcRefCon = this;
  status = AudioUnitSetProperty(output_unit,
      kAudioUnitProperty_SetRenderCallback,
      kAudioUnitScope_Input,
      0,
      &render,
      sizeof(render));
  if (status != noErr) {
    SoundFailure("can't set the render callback, CoreAudio says "
      + DescribeStatus(status));
    AudioComponentInstanceDispose(output_unit);
    return;
  }

  status = AudioUnitInitialize(output_unit);
  if (status != noErr) {
    SoundFailure("can't initialize the output audio unit, CoreAudio says "
      + DescribeStatus(status));
    AudioComponentInstanceDispose(output_unit);
    return;
  }

  status = AudioOutputUnitStart(output_unit);
  if (status != noErr) {
    SoundFailure("can't start the output audio unit, CoreAudio says "
      + DescribeStatus(status));
    AudioUnitUninitialize(output_unit);
    AudioComponentInstanceDispose(output_unit);
    return;
  }

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
