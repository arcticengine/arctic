// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
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

#ifdef ARCTIC_PLATFORM_WINDOWS

#define IDI_ICON1 129

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <winsock2.h>
#include <Mmsystem.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>  // NOLINT
#include <utility>
#include <vector>

#include "engine/engine.h"
#include "engine/easy.h"
#include "engine/arctic_input.h"
#include "engine/arctic_platform.h"
#include "engine/arctic_mixer.h"
#include "engine/log.h"
#include "engine/rgb.h"
#include "engine/vec3f.h"

#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Ws2_32.lib")


namespace arctic {

extern SoundMixerState g_sound_mixer_state;

class SoundPlayerImpl {
 public:
  bool is_initialized = false;
  std::thread sound_thread;

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


void SoundMixerThreadFunction() {
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
  Si32 bytes_per_sample = 2;

  WAVEFORMATEX format;
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = 2;
  format.nSamplesPerSec = 44100;
  format.nAvgBytesPerSec =
    bytes_per_sample * format.nChannels * format.nSamplesPerSec;
  format.nBlockAlign = bytes_per_sample * format.nChannels;
  format.wBitsPerSample = 8 * bytes_per_sample;
  format.cbSize = 0;

  HANDLE hEvent = CreateEvent(NULL, false, false, nullptr);
  Check(hEvent != 0, "Can't create event");

  HWAVEOUT wave_out_handle;
  MMRESULT result = waveOutOpen(&wave_out_handle, WAVE_MAPPER,
    &format, reinterpret_cast<DWORD_PTR>(hEvent), 0,
    CALLBACK_EVENT | WAVE_FORMAT_DIRECT);

  const char *postfix = nullptr;
  switch (result) {
  case MMSYSERR_ALLOCATED:
      postfix = "Specified resource is already allocated.";
      break;
  case MMSYSERR_BADDEVICEID:
      postfix = "Specified device identifier is out of range.";
      break;
  case MMSYSERR_NODRIVER:
      postfix = "No device driver is present.";
      break;
  case MMSYSERR_NOMEM:
      postfix = "Unable to allocate or lock memory.";
      break;
  case WAVERR_BADFORMAT:
      postfix = "Attempted to open with an unsupported waveform-audio format.";
      break;
  case WAVERR_SYNC:
      postfix = "The device is synchronous but waveOutOpen was called"
          " without using the WAVE_ALLOWSYNC flag.";
      break;
  }
  Check(result == MMSYSERR_NOERROR,
      "Error in SoundMixerThreadFunction during waweOutOpen:", postfix);

  Ui32 buffer_count = 10ull;
  Ui64 buffer_duration_us = 5000ull;
  Si32 buffer_samples_per_channel =
    static_cast<Si32>(
      static_cast<Ui64>(format.nSamplesPerSec) *
      buffer_duration_us / 1000000ull);
  Si32 buffer_samples_total = format.nChannels * buffer_samples_per_channel;

  std::vector<WAVEHDR> wave_headers(buffer_count);
  std::vector<std::vector<Si16>> wave_buffers(buffer_count);
  std::vector<Si16> tmp(buffer_samples_total);
  std::vector<float> mix(buffer_samples_total);
  float *mix_l = &mix[0];
  float *mix_r = &mix[1];
  Si32 mix_stride = 2;
  for (Ui32 i = 0; i < wave_headers.size(); ++i) {
    wave_buffers[i].resize(buffer_samples_total);

    memset(reinterpret_cast<char*>(&wave_headers[i]), 0, sizeof(WAVEHDR));
    Ui32 buffer_bytes = bytes_per_sample * buffer_samples_total;
    wave_headers[i].dwBufferLength = buffer_bytes;
    wave_headers[i].lpData = reinterpret_cast<char*>(&(wave_buffers[i][0]));
    waveOutPrepareHeader(wave_out_handle,
      &wave_headers[i], sizeof(WAVEHDR));
    wave_headers[i].dwFlags |= WHDR_DONE;
  }
  Check(result == MMSYSERR_NOERROR, "Error in SoundMixerThreadFunction");

  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

  size_t cur_buffer_idx = 0;
  while (!g_sound_mixer_state.do_quit) {
    MMTIME mmt;
    waveOutGetPosition(wave_out_handle, &mmt, sizeof(mmt));
    while (!(*(volatile DWORD*)&wave_headers[cur_buffer_idx].dwFlags
          & WHDR_DONE)) {
      WaitForSingleObject(hEvent, INFINITE);
      waveOutGetPosition(wave_out_handle, &mmt, sizeof(mmt));
    }
    (*(volatile DWORD*)&wave_headers[cur_buffer_idx].dwFlags) &= ~WHDR_DONE;

    g_sound_mixer_state.MixSound(mix_l, mix_r, mix_stride, buffer_samples_per_channel, tmp.data());

    // Convert to 16-bit integer format.
    Si16* out_data = &(wave_buffers[cur_buffer_idx][0]);
    for (Si32 i = 0; i < buffer_samples_total; ++i) {
      out_data[i] = static_cast<Si16>(Clamp(
        mix[i] * 32767.f, -32767.f, 32767.f));
    }

    waveOutWrite(wave_out_handle,
      &wave_headers[cur_buffer_idx], sizeof(WAVEHDR));
    cur_buffer_idx = (cur_buffer_idx + 1) % wave_headers.size();
  }
  timeEndPeriod(1);

  for (Ui32 i = 0; i < wave_headers.size(); ++i) {
    do {
      result = waveOutUnprepareHeader(wave_out_handle,
        &wave_headers[i], sizeof(WAVEHDR));
    } while (result == WAVERR_STILLPLAYING);
  }
  waveOutClose(wave_out_handle);
  CloseHandle(hEvent);
  return;
}

void SoundPlayerImpl::Initialize() {
  if (is_initialized) {
    return;
  }
  is_initialized = true;
  std::thread st(arctic::SoundMixerThreadFunction);
  sound_thread = std::move(st);
  sound_thread.detach();
}

void SoundPlayerImpl::Deinitialize() {
  if (is_initialized) {
    is_initialized = false;
    arctic::g_sound_mixer_state.do_quit = true;
  }
}


}  // namespace arctic

#endif  // ARCTIC_PLATFORM_WINDOWS
