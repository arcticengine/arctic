// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "engine/arctic_platform_def.h"

#ifdef ARCTIC_PLATFORM_MACOSX

#import <Cocoa/Cocoa.h>

#include <AudioToolbox/AudioToolbox.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define _USE_MATH_DEFINES
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
