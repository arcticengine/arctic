// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2025 Huldra
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

#ifdef ARCTIC_PLATFORM_WEB

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

#include <emscripten/em_math.h>
#include <emscripten/webaudio.h>


namespace arctic {

extern SoundMixerState g_sound_mixer_state;
static SoundPlayerImpl *g_sound_player_impl = nullptr;
static uint8_t audioThreadStack[262144];

class SoundPlayerImpl {
public:
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
    g_sound_player_impl = impl;
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
    g_sound_player_impl = nullptr;
  }
}


bool OnCanvasClick(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
  EMSCRIPTEN_WEBAUDIO_T audioContext = (EMSCRIPTEN_WEBAUDIO_T)userData;
  if (emscripten_audio_context_state(audioContext) != AUDIO_CONTEXT_STATE_RUNNING) {
    emscripten_resume_audio_context_sync(audioContext);
  }
  return false;
}


bool GenerateNoise(int numInputs, const AudioSampleFrame *inputs,
                      int numOutputs, AudioSampleFrame *outputs,
                      int numParams, const AudioParamFrame *params,
                      void *userData) {
  SoundPlayerImpl *mixer = g_sound_player_impl;
  for(int i = 0; i < numOutputs; ++i) {
    Si32 buffer_samples_per_channel = outputs[i].samplesPerChannel;
    float *mix_l = &outputs[i].data[0];
    float *mix_r = &outputs[i].data[1];
    Si32 mix_stride = outputs[i].numberOfChannels;
    if (mixer->tmp.size() < buffer_samples_per_channel * 2) {
      mixer->tmp.resize(buffer_samples_per_channel * 2);
    }
    g_sound_mixer_state.MixSound(mix_l, mix_r, mix_stride, buffer_samples_per_channel, mixer->tmp.data());
  }

  return true; // Keep the graph output going
}

void AudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData){
  if (!success) {
    return; // Check browser console in a debug build for detailed errors
  }
  int outputChannelCounts[1] = { 1 };
  EmscriptenAudioWorkletNodeCreateOptions options = {
    .numberOfInputs = 0,
    .numberOfOutputs = 1,
    .outputChannelCounts = outputChannelCounts
  };
  // Create node
  EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node(audioContext,
    "noise-generator", &options, &GenerateNoise, 0);
  // Connect it to audio context destination
  emscripten_audio_node_connect(wasmAudioWorklet, audioContext, 0, 0);
  // Resume context on mouse click
  emscripten_set_click_callback("canvas", (void*)audioContext, 0, OnCanvasClick);
}

void AudioThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData) {
  if (!success) return; // Check browser console in a debug build for detailed errors
  WebAudioWorkletProcessorCreateOptions opts = {
    .name = "noise-generator",
  };
  emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &AudioWorkletProcessorCreated, 0);
}

void SoundPlayerImpl::Initialize() {
  if (is_initialized) {
    return;
  }

  tmp.resize(2 << 20);
  g_sound_mixer_state.InputTasksToMixerThread();

  EmscriptenWebAudioCreateAttributes attributes;
  attributes.latencyHint = "interactive";
  attributes.sampleRate = 44100;
  EMSCRIPTEN_WEBAUDIO_T context = emscripten_create_audio_context(&attributes);
  emscripten_start_wasm_audio_worklet_thread_async(context, arctic::audioThreadStack, sizeof(arctic::audioThreadStack),
    &arctic::AudioThreadInitialized, 0);

  is_initialized = true;
}

void SoundPlayerImpl::Deinitialize() {
  if (is_initialized) {
    is_initialized = false;
  }
}

}  // namespace arctic

#endif
