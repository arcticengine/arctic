// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2019 Huldra
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

#ifdef ARCTIC_PLATFORM_PI

#include <alsa/asoundlib.h>
#include <alsa/control.h>

#include <algorithm>
#include <deque>
#include <iostream>
#include <mutex>  // NOLINT
#include <thread>  // NOLINT
#include <vector>

#include "engine/arctic_mixer.h"
#include "engine/arctic_platform_sound.h"
#include "engine/arctic_platform_fatal.h"


namespace arctic {

SoundMixerState g_sound_mixer_state;

class SoundPlayerImpl {
 public:
  bool is_initialized = false;

  std::deque<AudioDeviceInfo> GetDeviceList();
  void Initialize();
  void Initialize(const char *input_device_system_name,
    const char *output_device_system_name);
  void Deinitialize();
  ~SoundPlayerImpl() {
    Deinitialize();
  }
};

void SoundPlayer::Initialize() {
  Initialize(nullptr, nullptr);
}

void SoundPlayer::Initialize(const char *input_device_system_name,
    const char *output_device_system_name) {
  if (!impl) {
    impl = new SoundPlayerImpl;
  }
  impl->Initialize(input_device_system_name, output_device_system_name);
}

std::deque<AudioDeviceInfo> SoundPlayer::GetDeviceList() {
  if (!impl) {
    impl = new SoundPlayerImpl;
  }
  return impl->GetDeviceList();
}

void SoundPlayer::Deinitialize() {
  if (impl) {
    impl->Deinitialize();
  }
}

bool SoundPlayer::IsOk() {
  return g_sound_mixer_state.IsOk();
}

std::string SoundPlayer::GetErrorDescription() {
  return g_sound_mixer_state.GetErrorDescription();
}

SoundPlayer::~SoundPlayer() {
  if (impl) {
    delete impl;
    impl = nullptr;
  }
}

bool SoundCheck(bool condition, const char *error_message,
    const char *error_message_postfix = nullptr) {
  if (condition) {
    return true;
  }
  size_t size = 1 +
    strlen(error_message) +
    (error_message_postfix ? strlen(error_message_postfix) : 0);
  char *full_message = static_cast<char *>(malloc(size));
  memset(full_message, 0, size);
  snprintf(full_message, size, "%s%s", error_message,
      (error_message_postfix ? error_message_postfix : ""));
  std::cerr << "Arctic Engine Sound ERROR: " << full_message << std::endl;
  // Signal error and stop the mixer
  g_sound_mixer_state.SetError(full_message);
  g_sound_mixer_state.do_quit.store(true);
  return false;
}

void StartSoundBuffer(easy::Sound sound, float volume) {
  if (sound.GetInstance()) {
    SoundBuffer buffer;
    buffer.sound = sound;
    buffer.volume = volume;
    buffer.next_position = 0;
    buffer.sound.GetInstance()->IncPlaying();
    buffer.action = SoundBuffer::kStart;
    g_sound_mixer_state.AddSoundTask(buffer);
  }
}

void StopSoundBuffer(easy::Sound sound) {
  if (sound.GetInstance()) {
    SoundBuffer buffer;
    buffer.sound = sound;
    buffer.volume = 0.f;
    buffer.next_position = 0;
    buffer.action = SoundBuffer::kStop;
    g_sound_mixer_state.AddSoundTask(buffer);
  }
}

void SetMasterVolume(float volume) {
  g_sound_mixer_state.master_volume.store(volume);
}

float GetMasterVolume() {
  return g_sound_mixer_state.master_volume.load();
}

static unsigned int g_buffer_time_us = 50000;
static unsigned int g_period_time_us = 10000;

struct async_private_data {
  std::vector<Si16> samples;
  std::vector<Si32> mix;
  std::vector<Si16> tmp;
  snd_async_handler_t *ahandler = nullptr;
  snd_pcm_t *handle = nullptr;
  snd_output_t *output = nullptr;
  snd_pcm_sframes_t buffer_size;
  snd_pcm_sframes_t period_size;
};

static async_private_data g_data;

void MixSound() {
  async_private_data *data = &g_data;

  Si32 buffer_samples_total = data->period_size * 2;
  Si32 buffer_bytes = data->period_size * 4;

  float master_volume = 1.0f;
  {
    memset(data->mix.data(), 0, 2 * buffer_bytes);

    g_sound_mixer_state.InputTasksToMixerThread();

    master_volume = g_sound_mixer_state.master_volume.load();

    for (Ui32 idx = 0;
        idx < g_sound_mixer_state.buffers.size(); ++idx) {
      SoundBuffer &sound = g_sound_mixer_state.buffers[idx];

      Ui32 size = data->period_size;
      size = sound.sound.StreamOut(sound.next_position, size,
          data->tmp.data(), buffer_samples_total);
      Si16 *in_data = data->tmp.data();
      for (Ui32 i = 0; i < size; ++i) {
        data->mix[i * 2] += static_cast<Si32>(
            static_cast<float>(in_data[i * 2]) * sound.volume);
        data->mix[i * 2 + 1] += static_cast<Si32>(
            static_cast<float>(in_data[i * 2 + 1]) * sound.volume);
        ++sound.next_position;
      }

      if (sound.next_position == sound.sound.DurationSamples()
          || size == 0) {
        sound.sound.GetInstance()->DecPlaying();
        g_sound_mixer_state.buffers[idx] =
          g_sound_mixer_state.buffers[
          g_sound_mixer_state.buffers.size() - 1];
        g_sound_mixer_state.buffers.pop_back();
        --idx;
      }
    }
  }

  unsigned char *out_buffer = (unsigned char *)data->samples.data();
  for (Ui32 i = 0; i < buffer_samples_total; ++i) {
    float val = static_cast<float>(data->mix[i]) * master_volume;
    Si16 res = static_cast<Si16>(std::min(std::max(val, -32767.0f), 32767.0f));
    out_buffer[i * 2 + 0] = res & 0xff;
    out_buffer[i * 2 + 1] = (res >> 8) & 0xff;
  }
}

static void SoundMixerCallback(snd_async_handler_t *ahandler) {
  snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
  async_private_data *data = static_cast<async_private_data*>(
      snd_async_handler_get_callback_private(ahandler));

  bool is_ok = true;
  while (is_ok) {
    snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
    if (avail < data->period_size) {
      return;
    }

    MixSound();

    unsigned char *out_buffer = (unsigned char *)data->samples.data();
    int err = snd_pcm_writei(handle, out_buffer, data->period_size);
    is_ok = is_ok && SoundCheck(err >= 0, "Sound write error: ", snd_strerror(err));
    is_ok = is_ok && SoundCheck(err == data->period_size,
        "Sound write error: written != expected.");
  }
}

void SoundMixerThreadFunction() {
  bool is_ok = true;
  while (!g_sound_mixer_state.do_quit.load()) {
    MixSound();

    Si16 *out_buffer = g_data.samples.data();
    Si32 size_left = g_data.period_size;
    while (size_left > 0 && is_ok) {
      int err = snd_pcm_writei(g_data.handle, out_buffer, size_left);
      if (err == -EAGAIN) {
        continue;
      } else if (err == -EPIPE) {
        err = snd_pcm_prepare(g_data.handle);
        is_ok = is_ok && SoundCheck(err >= 0, "Can't recover sound from underrun: ",
            snd_strerror(err));
      } else if (err == -ESTRPIPE) {
        while (true) {
          err = snd_pcm_resume(g_data.handle);
          if (err != -EAGAIN) {
            break;
          }
          sleep(1);
        }
        if (err < 0) {
          err = snd_pcm_prepare(g_data.handle);
          is_ok = is_ok && SoundCheck(err >= 0, "Can't recover sound from suspend: ",
              snd_strerror(err));
        }
      } else {
        is_ok = is_ok && SoundCheck(err >= 0, "Can't write sound data: ",
            snd_strerror(err));
      }
      out_buffer += err * 2;
      size_left -= err;
    }
  }
}

std::thread sound_thread;

void StartSoundMixer(const char* output_device_name) {
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_sw_params_t *swparams;
  snd_pcm_sw_params_alloca(&swparams);
  int err = snd_output_stdio_attach(&g_data.output, stdout, 0);
  bool is_ok = SoundCheck(err >= 0, "Sound error output setup failed: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }

  if (!output_device_name) {
    // default device
    err = snd_pcm_open(&g_data.handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err == -ENOENT) {
      err = snd_pcm_open(&g_data.handle, "plughw:0,0",
          SND_PCM_STREAM_PLAYBACK, 0);
      is_ok = is_ok && SoundCheck(err >= 0, "Can't open 'plughw:0,0' sound device: ",
          snd_strerror(err));
    } else {
      is_ok = is_ok && SoundCheck(err >= 0, "Can't open 'default' sound device: ",
          snd_strerror(err));
    }
  } else {
    err = snd_pcm_open(&g_data.handle, output_device_name,
        SND_PCM_STREAM_PLAYBACK, 0);
    is_ok = is_ok && SoundCheck(err >= 0, "Can't open the specified sound device: ",
        snd_strerror(err));
  }
  if (!is_ok) {
    return;
  }

  err = snd_pcm_hw_params_any(g_data.handle, hwparams);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't get sound configuration space: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_hw_params_set_rate_resample(g_data.handle, hwparams, 1);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set sound resampling: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_hw_params_set_access(g_data.handle, hwparams,
      SND_PCM_ACCESS_RW_INTERLEAVED);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set access type for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_hw_params_set_format(g_data.handle, hwparams,
      SND_PCM_FORMAT_S16);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set sample format for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_hw_params_set_channels(g_data.handle, hwparams, 2);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set 2 channels for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  unsigned int rate = 44100;
  err = snd_pcm_hw_params_set_rate_near(g_data.handle, hwparams, &rate, 0);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set 44100 Hz rate for sound: ",
      snd_strerror(err));
  is_ok = is_ok && SoundCheck(rate == 44100,
      "Sound output rate doesn't match requested 44100 Hz.");
  if (!is_ok) {
    return;
  }
  int dir;
  err = snd_pcm_hw_params_set_buffer_time_near(g_data.handle, hwparams,
      &g_buffer_time_us, &dir);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set buffer time for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  snd_pcm_uframes_t size;
  err = snd_pcm_hw_params_get_buffer_size(hwparams, &size);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't get buffer size for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  g_data.buffer_size = size;
  err = snd_pcm_hw_params_set_period_time_near(g_data.handle, hwparams,
      &g_period_time_us, &dir);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set period time for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't get period size for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  g_data.period_size = size;
  err = snd_pcm_hw_params(g_data.handle, hwparams);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set hw params for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }

  err = snd_pcm_sw_params_current(g_data.handle, swparams);
  is_ok = is_ok && SoundCheck(err >= 0,
      "Can't determine current sw params for sound: ", snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_sw_params_set_start_threshold(g_data.handle, swparams, 512);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set start threshold mode for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_sw_params_set_avail_min(g_data.handle, swparams,
      512);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set avail min for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }
  err = snd_pcm_sw_params(g_data.handle, swparams);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set sw params for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    return;
  }

  // start sound
  g_data.samples.resize(g_data.period_size * 2, 0);
  g_data.mix.resize(g_data.period_size * 2, 0);
  g_data.tmp.resize(g_data.period_size * 2, 0);
  err = snd_async_add_pcm_handler(&g_data.ahandler, g_data.handle,
      SoundMixerCallback, &g_data);
  if (err == -ENOSYS) {
    sound_thread = std::thread(arctic::SoundMixerThreadFunction);
    sound_thread.detach();
  } else {
    is_ok = is_ok && SoundCheck(err >= 0,
        "Can't register async pcm handler for sound:",
        snd_strerror(err));
    if (!is_ok) {
      return;
    }
    for (int count = 0; count < 3; count++) {
      err = snd_pcm_writei(g_data.handle, g_data.samples.data(),
          g_data.period_size);
      is_ok = is_ok && SoundCheck(err >= 0, "Sound pcm write error: ",
          snd_strerror(err));
      is_ok = is_ok && SoundCheck(err == g_data.period_size,
          "Sound pcm write error: written != expected");
      if (!is_ok) {
        return;
      }
    }
    if (snd_pcm_state(g_data.handle) == SND_PCM_STATE_PREPARED) {
      err = snd_pcm_start(g_data.handle);
      is_ok = is_ok && SoundCheck(err >= 0, "Sound pcm start error: ",
          snd_strerror(err));
      if (!is_ok) {
        return;
      }
    }

  }
}

void StopSoundMixer() {
  g_sound_mixer_state.do_quit.store(true);

  if (g_data.ahandler) {
    int err = snd_async_del_handler(g_data.ahandler);
    SoundCheck(err >= 0, "Can't delete async sound handler",
        snd_strerror(err));
  }
  snd_pcm_close(g_data.handle);
}

void SoundPlayerImpl::Initialize(const char *input_device_system_name,
    const char *output_device_system_name) {
  if (is_initialized) {
    return;
  }
  is_initialized = true;
  arctic::StartSoundMixer(output_device_system_name);
}

void SoundPlayerImpl::Deinitialize() {
  if (is_initialized) {
    is_initialized = false;
    arctic::StopSoundMixer();
  }
}


std::deque<AudioDeviceInfo> SoundPlayerImpl::GetDeviceList() {
  std::deque<AudioDeviceInfo> list;
  void **hints;
  int err = snd_device_name_hint(-1, "pcm", &hints);
  bool is_ok = SoundCheck(err >= 0, "Can't list sound devices: ",
      snd_strerror(err));
  if (!is_ok) {
    return list;
  }

  for (void **cur_hint = hints; *cur_hint; ++cur_hint) {
    char *name = snd_device_name_get_hint(*cur_hint, "NAME");
    char *desc = snd_device_name_get_hint(*cur_hint, "DESC");
    char *ioid = snd_device_name_get_hint(*cur_hint, "IOID");
    bool is_input = (!ioid || strcmp(ioid, "Input") == 0);
    bool is_output = (!ioid || strcmp(ioid, "Output") == 0);

    list.emplace_back(name, desc, is_input, is_output);

    free(name);
    free(desc);
    free(ioid);
  }
  snd_device_name_free_hint(hints);
  return list;
}

}  // namespace arctic



#endif  // ARCTIC_PLATFORM_PI
