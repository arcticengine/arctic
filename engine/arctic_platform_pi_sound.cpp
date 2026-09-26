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

#ifdef ARCTIC_PLATFORM_PI
#ifdef ARCTIC_NO_ALSA

// A build without the ALSA headers (see the find_package(ALSA) block in the
// CMakeLists.txt files) has no way to reach a sound device, so the player is here
// only to keep the rest of the engine compiling and running. It says once, in the
// log, that this build is mute, and SoundPlayer::IsOk() keeps saying it
// afterwards through GetErrorDescription(), so an application can pass the word
// on to whoever is looking at the screen.

#include <deque>
#include <string>

#include "engine/arctic_mixer.h"
#include "engine/arctic_platform_sound.h"
#include "engine/log.h"

namespace arctic {

extern SoundMixerState g_sound_mixer_state;

namespace {

const char *kNoAlsaMessage =
  "This build has no sound: it was compiled without ALSA (ARCTIC_NO_ALSA)."
  " Install the ALSA headers (apt install libasound2-dev) and build again to"
  " hear anything.";

}  // namespace

class SoundPlayerImpl {
 public:
  bool is_initialized = false;
};

void SoundPlayer::Initialize() {
  Initialize(nullptr, nullptr);
}

void SoundPlayer::Initialize(const char *input_device_system_name,
    const char *output_device_system_name) {
  (void)input_device_system_name;
  (void)output_device_system_name;
  if (!impl) {
    impl = new SoundPlayerImpl;
  }
  if (impl->is_initialized) {
    return;
  }
  impl->is_initialized = true;
  *Log() << kNoAlsaMessage;
  g_sound_mixer_state.SetError(kNoAlsaMessage);
  g_sound_mixer_state.do_quit.store(true);
}

std::deque<AudioDeviceInfo> SoundPlayer::GetDeviceList() {
  return std::deque<AudioDeviceInfo>();
}

void SoundPlayer::Deinitialize() {
  if (impl) {
    impl->is_initialized = false;
  }
}

bool SoundPlayer::IsOk() {
  return false;
}

std::string SoundPlayer::GetErrorDescription() {
  return std::string(kNoAlsaMessage);
}

SoundPlayer::~SoundPlayer() {
  if (impl) {
    delete impl;
    impl = nullptr;
  }
}

bool SoundMixerShouldUseDedicatedThread() {
  return true;
}

bool SoundMixerIsDedicatedThreadRunning() {
  return false;
}

bool SoundMixerHasAsyncPcmHandler() {
  return false;
}

void UpdateSoundEngine() {
  g_sound_mixer_state.CloseRetiredStreams();
}

}  // namespace arctic

#else  // ARCTIC_NO_ALSA

#include <alsa/asoundlib.h>
#include <alsa/control.h>

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>
#include <thread>  // NOLINT
#include <unistd.h>
#include <vector>

#include "engine/arctic_mixer.h"
#include "engine/arctic_platform_sound.h"
#include "engine/log.h"
#include "engine/arctic_platform_fatal.h"
#include "engine/scalar_math.h"


namespace arctic {

extern SoundMixerState g_sound_mixer_state;

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

void UpdateSoundEngine();

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
  UpdateSoundEngine();
  return g_sound_mixer_state.IsOk();
}

std::string SoundPlayer::GetErrorDescription() {
  UpdateSoundEngine();
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
  free(full_message);
  return false;
}

static unsigned int g_buffer_time_us = 50000;
static unsigned int g_period_time_us = 10000;

struct async_private_data {
  std::vector<Si16> samples;
  std::vector<float> mix;
  std::vector<Si16> tmp;
  snd_async_handler_t *ahandler = nullptr;
  snd_pcm_t *handle = nullptr;
  snd_output_t *output = nullptr;
  snd_pcm_sframes_t buffer_size = 0;
  snd_pcm_sframes_t period_size = 0;
  // Matches sw start_threshold (set to period_size after hw negotiate).
  snd_pcm_sframes_t start_threshold = 0;
  // Preallocated before async start. The SIGIO handler writes a detailed
  // NUL-terminated message here, then publishes via g_async_mixer_errno
  // (release). One buffer is enough: an error ends the sound path.
  static constexpr size_t kErrorMessageBytes = 1000;
  char error_message[kErrorMessageBytes];
};

static async_private_data g_data;
static std::atomic<int> g_async_mixer_errno{0};
// SIGIO stores -EPIPE / -ESTRPIPE here; UpdateSoundEngine recovers off-signal.
static std::atomic<int> g_async_pcm_recover_code{0};
// Covers prepare+priming after recover_code is exchanged to 0 (SIGIO early-out).
static std::atomic<bool> g_async_pcm_recovering{false};
static bool is_sound_thread = false;
static std::thread sound_thread;

// Signal-safe helpers to fill g_data.error_message without snprintf/malloc.
static void SoundErrorAppendStr(char *dst, size_t cap, size_t *pos,
    const char *src) {
  if (!src || *pos + 1 >= cap) {
    if (cap > 0) {
      dst[cap - 1 < *pos ? cap - 1 : *pos] = '\0';
    }
    return;
  }
  while (*src && *pos + 1 < cap) {
    dst[(*pos)++] = *src++;
  }
  dst[*pos] = '\0';
}

static void SoundErrorAppendInt(char *dst, size_t cap, size_t *pos, int value) {
  char tmp[16];
  size_t n = 0;
  unsigned int u;
  if (value < 0) {
    if (*pos + 1 < cap) {
      dst[(*pos)++] = '-';
      dst[*pos] = '\0';
    }
    // INT_MIN-safe: convert via unsigned.
    u = static_cast<unsigned int>(-(value + 1)) + 1u;
  } else {
    u = static_cast<unsigned int>(value);
  }
  if (u == 0) {
    tmp[n++] = '0';
  } else {
    while (u > 0 && n < sizeof(tmp)) {
      tmp[n++] = static_cast<char>('0' + (u % 10u));
      u /= 10u;
    }
  }
  while (n > 0 && *pos + 1 < cap) {
    dst[(*pos)++] = tmp[--n];
  }
  if (*pos < cap) {
    dst[*pos] = '\0';
  }
}

void MixSound(bool async_signal_safe);
static void SoundReportErrorFromSignal(int err_code, const char *context);

// Period-sized writes needed so queued frames meet start_threshold.
// start_threshold is set to period_size at open, so this is typically 1.
// Shared by StartSoundMixer (silence) and UpdateSoundEngine recover (MixSound).
static int SoundMixerPrimingPeriodWrites() {
  const snd_pcm_sframes_t period = g_data.period_size;
  if (period <= 0) {
    return 1;
  }
  snd_pcm_sframes_t threshold = g_data.start_threshold;
  if (threshold <= 0) {
    threshold = period;
  }
  int n = static_cast<int>((threshold + period - 1) / period);
  if (n < 1) {
    n = 1;
  }
  if (g_data.buffer_size > 0) {
    const int max_in_buffer = static_cast<int>(g_data.buffer_size / period);
    if (max_in_buffer >= 1 && n > max_in_buffer) {
      n = max_in_buffer;
    }
  }
  return n;
}

void UpdateSoundEngine() {
  g_sound_mixer_state.CloseRetiredStreams();
  // 1) Deferred underrun/suspend recover requested from SIGIO (no prepare there).
  const int recover = g_async_pcm_recover_code.exchange(
      0, std::memory_order_acq_rel);
  if (recover != 0
      && g_data.handle
      && g_data.ahandler
      && !g_sound_mixer_state.do_quit.load(std::memory_order_acquire)) {
    // recover_code is already 0; hold SIGIO out for the whole prepare+prime window.
    struct AsyncPcmRecoveringGuard {
      AsyncPcmRecoveringGuard() {
        g_async_pcm_recovering.store(true, std::memory_order_release);
      }
      ~AsyncPcmRecoveringGuard() {
        g_async_pcm_recovering.store(false, std::memory_order_release);
      }
    } recovering_guard;

    int err = 0;
    if (recover == -ESTRPIPE) {
      err = snd_pcm_resume(g_data.handle);
      // Game thread: do not sleep on -EAGAIN; fall through to prepare.
      if (err == -EAGAIN || err < 0) {
        err = snd_pcm_prepare(g_data.handle);
      }
    } else {
      err = snd_pcm_prepare(g_data.handle);
    }
    if (err < 0) {
      SoundReportErrorFromSignal(err,
          recover == -ESTRPIPE
              ? "Can't recover sound from suspend"
              : "Can't recover sound from underrun");
    } else {
      // Restart async: mix + write enough periods to meet start_threshold.
      const int prime_writes = SoundMixerPrimingPeriodWrites();
      bool primed_ok = true;
      for (int count = 0; count < prime_writes; ++count) {
        MixSound(false);
        err = snd_pcm_writei(g_data.handle, g_data.samples.data(),
            g_data.period_size);
        if (err == -EPIPE || err == -ESTRPIPE) {
          g_async_pcm_recover_code.store(err, std::memory_order_release);
          primed_ok = false;
          break;
        }
        if (err < 0 || err != g_data.period_size) {
          SoundReportErrorFromSignal(err < 0 ? err : -1,
              err < 0 ? "Can't write sound data after recover"
                      : "async pcm short write after recover");
          primed_ok = false;
          break;
        }
      }
      if (primed_ok
          && snd_pcm_state(g_data.handle) == SND_PCM_STATE_PREPARED) {
        err = snd_pcm_start(g_data.handle);
        if (err < 0) {
          SoundReportErrorFromSignal(err, "Can't start sound after recover");
        }
      }
    }
  }

  // 2) Lift a SIGIO-deferred hard error into SetError + log once.
  // exchange clears the pending code, so later PumpMessages / IsOk stay quiet
  // until a new report. Acquire pairs with SoundReportErrorFromSignal release.
  int err = g_async_mixer_errno.exchange(0, std::memory_order_acq_rel);
  if (err == 0) {
    return;
  }
  g_sound_mixer_state.SetError(g_data.error_message);
  Log(g_data.error_message);
}

// SIGIO-safe: no malloc, iostream, or std::string/mutex SetError.
// Fill order: complete NUL-terminated message in the preallocated buffer,
// then release-store the atomic so UpdateSoundEngine sees a full string.
static void SoundReportErrorFromSignal(int err_code, const char *context) {
  constexpr size_t kCap = async_private_data::kErrorMessageBytes;
  char *buf = g_data.error_message;
  size_t pos = 0;
  buf[0] = '\0';
  SoundErrorAppendStr(buf, kCap, &pos, "Arctic Engine Sound ERROR: ");
  SoundErrorAppendStr(buf, kCap, &pos,
      context ? context : "async pcm failure");
  SoundErrorAppendStr(buf, kCap, &pos, " (code ");
  SoundErrorAppendInt(buf, kCap, &pos, err_code == 0 ? -1 : err_code);
  SoundErrorAppendStr(buf, kCap, &pos, ")");
  if (err_code < 0) {
    // snd_strerror typically indexes a static table; copy the text only.
    SoundErrorAppendStr(buf, kCap, &pos, ": ");
    SoundErrorAppendStr(buf, kCap, &pos, snd_strerror(err_code));
  }
  SoundErrorAppendStr(buf, kCap, &pos, "\n");

  const int published = (err_code == 0 ? -1 : err_code);
  g_async_mixer_errno.store(published, std::memory_order_release);
  g_sound_mixer_state.do_quit.store(true, std::memory_order_release);
  (void)!write(STDERR_FILENO, buf, pos);
}

// Test helper: exercise the SIGIO error-buffer path without raising a signal.
void SoundMixerTestReportAsyncError(int err_code, const char *context) {
  SoundReportErrorFromSignal(err_code, context);
}

const char *SoundMixerTestPeekAsyncErrorMessage() {
  return g_data.error_message;
}

void SoundMixerTestClearAsyncError() {
  g_async_mixer_errno.store(0, std::memory_order_release);
  g_async_pcm_recover_code.store(0, std::memory_order_release);
  g_async_pcm_recovering.store(false, std::memory_order_release);
}

void MixSound(bool async_signal_safe) {
  async_private_data *data = &g_data;
  float *mix_l = &data->mix[0];
  float *mix_r = &data->mix[1];
  Si32 mix_stride = 2;
  Si32 buffer_samples_per_channel = static_cast<Si32>(data->period_size);

  g_sound_mixer_state.MixSound(mix_l, mix_r, mix_stride,
      buffer_samples_per_channel, data->tmp.data(), async_signal_safe);

  // Convert to 16-bit integer format.
  unsigned char *out_buffer = (unsigned char *)data->samples.data();
  Si32 buffer_samples_total = static_cast<Si32>(data->period_size) * 2;
  for (Si32 i = 0; i < buffer_samples_total; ++i) {
    Si16 res = static_cast<Si16>(Clamp(
      data->mix[i] * 32767.f, -32767.f, 32767.f));
    out_buffer[i * 2 + 0] = res & 0xff;
    out_buffer[i * 2 + 1] = (res >> 8) & 0xff;
  }
}

void MixSound() {
  MixSound(false);
}

// SIGIO-safe. True when a full period can be mixed and written. In the XRUN
// state avail_update itself returns -EPIPE (-ESTRPIPE when suspended) and no
// write is attempted, so the recovery request must be raised here.
static bool SoundMixerAsyncAvailAllowsWrite(snd_pcm_sframes_t avail,
    snd_pcm_sframes_t period_size) {
  if (avail == -EPIPE || avail == -ESTRPIPE) {
    g_async_pcm_recover_code.store(static_cast<int>(avail),
        std::memory_order_release);
    return false;
  }
  if (avail < 0) {
    SoundReportErrorFromSignal(static_cast<int>(avail),
        "async pcm avail update failed");
    return false;
  }
  return avail >= period_size;
}

bool SoundMixerTestAsyncAvailAllowsWrite(Si64 avail, Si64 period_size) {
  return SoundMixerAsyncAvailAllowsWrite(
      static_cast<snd_pcm_sframes_t>(avail),
      static_cast<snd_pcm_sframes_t>(period_size));
}

int SoundMixerTestPeekAsyncRecoverCode() {
  return g_async_pcm_recover_code.load(std::memory_order_acquire);
}

// Delivered from SIGIO. Only preallocated buffers and atomics.
// Underrun/suspend: set g_async_pcm_recover_code; UpdateSoundEngine prepares.
static void SoundMixerCallback(snd_async_handler_t *ahandler) {
  snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
  async_private_data *data = static_cast<async_private_data*>(
      snd_async_handler_get_callback_private(ahandler));

  // Wait for off-signal recover (pending code or in-progress prepare/prime).
  if (g_async_pcm_recovering.load(std::memory_order_acquire)
      || g_async_pcm_recover_code.load(std::memory_order_acquire) != 0) {
    return;
  }

  while (!g_sound_mixer_state.do_quit.load(std::memory_order_relaxed)) {
    snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
    if (!SoundMixerAsyncAvailAllowsWrite(avail, data->period_size)) {
      return;
    }

    MixSound(true);

    unsigned char *out_buffer = (unsigned char *)data->samples.data();
    int err = snd_pcm_writei(handle, out_buffer, data->period_size);
    if (err == -EPIPE || err == -ESTRPIPE) {
      g_async_pcm_recover_code.store(err, std::memory_order_release);
      return;
    }
    if (err < 0 || err != data->period_size) {
      SoundReportErrorFromSignal(err < 0 ? err : -1,
          err < 0 ? "async pcm write failed"
                  : "async pcm short write (written != period)");
      return;
    }
  }
}

// Dedicated-thread fallback (-ENOSYS async). Keep underrun/suspend recovery;
// on hard failure report the same way the SIGIO handler does (prefilled
// buffer + do_quit, then UpdateSoundEngine SetError + Log once). No Check/Fatal here.
void SoundMixerThreadFunction() {
  while (!g_sound_mixer_state.do_quit.load()) {
    MixSound(false);

    Si16 *out_buffer = g_data.samples.data();
    Si32 size_left = static_cast<Si32>(g_data.period_size);
    while (size_left > 0 &&
        !g_sound_mixer_state.do_quit.load(std::memory_order_relaxed)) {
      int err = snd_pcm_writei(g_data.handle, out_buffer, size_left);
      if (err == -EAGAIN) {
        continue;
      }
      if (err == -EPIPE) {
        err = snd_pcm_prepare(g_data.handle);
        if (err < 0) {
          SoundReportErrorFromSignal(err,
              "Can't recover sound from underrun");
          return;
        }
        break;  // remix + rewrite after successful prepare
      }
      if (err == -ESTRPIPE) {
        while (true) {
          err = snd_pcm_resume(g_data.handle);
          if (err != -EAGAIN) {
            break;
          }
          sleep(1);
        }
        if (err < 0) {
          err = snd_pcm_prepare(g_data.handle);
        }
        if (err < 0) {
          SoundReportErrorFromSignal(err,
              "Can't recover sound from suspend");
          return;
        }
        break;  // remix + rewrite after successful resume/prepare
      }
      if (err < 0) {
        SoundReportErrorFromSignal(err, "Can't write sound data");
        return;
      }
      out_buffer += err * 2;
      size_left -= err;
    }
  }
}

void StartSoundMixer(const char* output_device_name) {
  // Process-wide mixer: hidden-window startup already starts it. A second
  // open/thread assign would race the live PCM and destroy a joinable
  // std::thread ("terminate called without an active exception").
  if (g_data.handle || g_data.ahandler || is_sound_thread) {
    return;
  }
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
      is_ok = is_ok && SoundCheck(err >= 0,
          "Can't open 'plughw:0,0' sound device: ",
          snd_strerror(err));
    } else {
      is_ok = is_ok && SoundCheck(err >= 0,
          "Can't open 'default' sound device: ",
          snd_strerror(err));
    }
  } else {
    err = snd_pcm_open(&g_data.handle, output_device_name,
        SND_PCM_STREAM_PLAYBACK, 0);
    is_ok = is_ok && SoundCheck(err >= 0,
        "Can't open the specified sound device: ",
        snd_strerror(err));
  }
  if (!is_ok) {
    return;
  }

  err = snd_pcm_hw_params_any(g_data.handle, hwparams);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't get sound configuration space: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  err = snd_pcm_hw_params_set_rate_resample(g_data.handle, hwparams, 1);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set sound resampling: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  err = snd_pcm_hw_params_set_access(g_data.handle, hwparams,
      SND_PCM_ACCESS_RW_INTERLEAVED);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set access type for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  err = snd_pcm_hw_params_set_format(g_data.handle, hwparams,
      SND_PCM_FORMAT_S16);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set sample format for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  err = snd_pcm_hw_params_set_channels(g_data.handle, hwparams, 2);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set 2 channels for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  {
    unsigned int rate = 44100;
    err = snd_pcm_hw_params_set_rate_near(g_data.handle, hwparams, &rate, 0);
    is_ok = is_ok && SoundCheck(err >= 0, "Can't set 44100 Hz rate for sound: ",
        snd_strerror(err));
    is_ok = is_ok && SoundCheck(rate == 44100,
        "Sound output rate doesn't match requested 44100 Hz.");
    if (!is_ok) {
      goto cleanup;
    }
  }
  {
    int dir;
    err = snd_pcm_hw_params_set_buffer_time_near(g_data.handle, hwparams,
        &g_buffer_time_us, &dir);
    is_ok = is_ok && SoundCheck(err >= 0, "Can't set buffer time for sound: ",
        snd_strerror(err));
    if (!is_ok) {
      goto cleanup;
    }
    snd_pcm_uframes_t size;
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &size);
    is_ok = is_ok && SoundCheck(err >= 0, "Can't get buffer size for sound: ",
        snd_strerror(err));
    if (!is_ok) {
      goto cleanup;
    }
    g_data.buffer_size = static_cast<snd_pcm_sframes_t>(size);
    err = snd_pcm_hw_params_set_period_time_near(g_data.handle, hwparams,
        &g_period_time_us, &dir);
    is_ok = is_ok && SoundCheck(err >= 0, "Can't set period time for sound: ",
        snd_strerror(err));
    if (!is_ok) {
      goto cleanup;
    }
    err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
    is_ok = is_ok && SoundCheck(err >= 0, "Can't get period size for sound: ",
        snd_strerror(err));
    if (!is_ok) {
      goto cleanup;
    }
    g_data.period_size = static_cast<snd_pcm_sframes_t>(size);
  }
  err = snd_pcm_hw_params(g_data.handle, hwparams);
  is_ok = is_ok && SoundCheck(err >= 0, "Can't set hw params for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }

  err = snd_pcm_sw_params_current(g_data.handle, swparams);
  is_ok = is_ok && SoundCheck(err >= 0,
      "Can't determine current sw params for sound: ", snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  g_data.start_threshold = g_data.period_size;
  err = snd_pcm_sw_params_set_start_threshold(g_data.handle, swparams,
      static_cast<snd_pcm_uframes_t>(g_data.start_threshold));
  is_ok = is_ok && SoundCheck(err >= 0,
      "Can't set start threshold mode for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  err = snd_pcm_sw_params_set_avail_min(g_data.handle, swparams,
      static_cast<snd_pcm_uframes_t>(g_data.period_size));
  is_ok = is_ok && SoundCheck(err >= 0,
      "Can't set avail min for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }
  err = snd_pcm_sw_params(g_data.handle, swparams);
  is_ok = is_ok && SoundCheck(err >= 0,
      "Can't set sw params for sound: ",
      snd_strerror(err));
  if (!is_ok) {
    goto cleanup;
  }

  // start sound
  // Preallocate before any SIGIO delivery. MixSound from the handler must
  // not grow these vectors.
  g_data.samples.resize(static_cast<size_t>(g_data.period_size) * 2, 0);
  g_data.mix.resize(static_cast<size_t>(g_data.period_size) * 2, 0.f);
  g_data.tmp.resize(static_cast<size_t>(g_data.period_size) * 2, 0);
  g_data.error_message[0] = '\0';
  g_async_mixer_errno.store(0, std::memory_order_relaxed);
  g_async_pcm_recover_code.store(0, std::memory_order_relaxed);
  g_async_pcm_recovering.store(false, std::memory_order_relaxed);
  g_sound_mixer_state.do_quit.store(false);

  // Mix from the ALSA async handler (SIGIO) when available; it must touch
  // only preallocated buffers. The dedicated thread is the -ENOSYS fallback.
  err = snd_async_add_pcm_handler(&g_data.ahandler, g_data.handle,
      SoundMixerCallback, &g_data);
  if (err == -ENOSYS) {
    g_data.ahandler = nullptr;
    is_sound_thread = true;
    sound_thread = std::thread(arctic::SoundMixerThreadFunction);
  } else {
    is_ok = is_ok && SoundCheck(err >= 0,
        "Can't register async pcm handler for sound: ",
        snd_strerror(err));
    if (!is_ok) {
      goto cleanup;
    }
    // Silence prime: enough periods to meet start_threshold (no MixSound yet).
    const int prime_writes = SoundMixerPrimingPeriodWrites();
    for (int count = 0; count < prime_writes; count++) {
      err = snd_pcm_writei(g_data.handle, g_data.samples.data(),
          g_data.period_size);
      is_ok = is_ok && SoundCheck(err >= 0, "Sound pcm write error: ",
          snd_strerror(err));
      is_ok = is_ok && SoundCheck(err == g_data.period_size,
          "Sound pcm write error: written != expected");
      if (!is_ok) {
        goto cleanup;
      }
    }
    if (snd_pcm_state(g_data.handle) == SND_PCM_STATE_PREPARED) {
      err = snd_pcm_start(g_data.handle);
      is_ok = is_ok && SoundCheck(err >= 0, "Sound pcm start error: ",
          snd_strerror(err));
      if (!is_ok) {
        goto cleanup;
      }
    }
  }
  return;

cleanup:
  if (g_data.ahandler) {
    snd_async_del_handler(g_data.ahandler);
    g_data.ahandler = nullptr;
  }
  if (g_data.handle) {
    snd_pcm_close(g_data.handle);
    g_data.handle = nullptr;
  }
}

void StopSoundMixer() {
  g_sound_mixer_state.do_quit.store(true);
  g_async_pcm_recover_code.store(0, std::memory_order_release);
  g_async_pcm_recovering.store(false, std::memory_order_release);
  if (is_sound_thread) {
    sound_thread.join();
    is_sound_thread = false;
  }
  if (g_data.ahandler) {
    int err = snd_async_del_handler(g_data.ahandler);
    SoundCheck(err >= 0, "Can't delete async sound handler: ",
        snd_strerror(err));
    g_data.ahandler = nullptr;
  }
  if (g_data.handle) {
    snd_pcm_close(g_data.handle);
    g_data.handle = nullptr;
  }
  UpdateSoundEngine();
}

bool SoundMixerShouldUseDedicatedThread() {
  return is_sound_thread;
}

bool SoundMixerIsDedicatedThreadRunning() {
  return is_sound_thread;
}

bool SoundMixerHasAsyncPcmHandler() {
  return g_data.ahandler != nullptr;
}

void SoundPlayerImpl::Initialize(const char *input_device_system_name,
    const char *output_device_system_name) {
  (void)input_device_system_name;
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



#ifdef ARCTIC_TEST_SIGIO_REPRO
// Test-only hooks for tests_sigio_repro.
// --legacy-unsafe: MixSound + malloc from a signal (old hazard, aborts).
// --safe: MixSound(async_signal_safe=true) from a signal under heap stress
//         (the fixed SIGIO callback, must survive).

static std::atomic<bool> g_sigio_repro_main_in_heap{false};

void SoundMixerSigioReproSetMainThreadInHeap(bool in_heap) {
  g_sigio_repro_main_in_heap.store(in_heap, std::memory_order_relaxed);
}

bool SoundMixerSigioReproPrepareBuffers() {
  g_data.period_size = 441;
  g_data.buffer_size = 441 * 5;
  g_data.samples.assign(static_cast<size_t>(g_data.period_size) * 2, 0);
  g_data.mix.assign(static_cast<size_t>(g_data.period_size) * 2, 0.f);
  g_data.tmp.assign(static_cast<size_t>(g_data.period_size) * 2, 0);
  g_sound_mixer_state.do_quit.store(false);
  return true;
}

void SoundMixerSigioReproInvokeLegacyUnsafeFromSignal() {
  const bool overlap =
      g_sigio_repro_main_in_heap.load(std::memory_order_relaxed);
  // Deliberately unsafe: same shape as the old SoundCheck-from-SIGIO path.
  MixSound(false);
  const char *error_message = "SIGIO legacy-unsafe SoundCheck allocation";
  size_t size = 1 + strlen(error_message);
  char *full_message = static_cast<char *>(malloc(size));
  if (full_message) {
    memcpy(full_message, error_message, size);
    free(full_message);
  }
  if (overlap) {
    const char msg[] =
        "REPRODUCED: MixSound/SoundCheck ran from a signal handler while "
        "the main thread was in malloc/free (SIGIO heap race)\n";
    (void)!write(STDERR_FILENO, msg, sizeof(msg) - 1);
    abort();
  }
}

void SoundMixerSigioReproInvokeSafeMixFromSignal() {
  // Same call the SIGIO callback makes: MixSound with async_signal_safe=true (no heap).
  MixSound(true);
}

void SoundMixerSigioReproInvokeMixFromThread() {
  MixSound(false);
}

// Keep old name as alias used by existing harness until main is updated.
void SoundMixerSigioReproInvokeMixFromSignal() {
  SoundMixerSigioReproInvokeLegacyUnsafeFromSignal();
}
#endif  // ARCTIC_TEST_SIGIO_REPRO

}  // namespace arctic

#endif  // ARCTIC_NO_ALSA

#endif  // ARCTIC_PLATFORM_PI
