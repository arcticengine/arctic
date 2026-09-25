// Sound loading and resampling.
#define TEST_NO_MAIN
#include "test_helpers.h"

#include "engine/arctic_platform_sound.h"
#include "engine/arctic_mixer.h"
#include <cerrno>
#include <string>

namespace arctic {
extern SoundMixerState g_sound_mixer_state;
}  // namespace arctic

// ============================================================================
// easy_sound_instance bug reproduction tests
// ============================================================================

// Helper: build a minimal valid WAV file in memory.
// Returns the byte buffer. samples is interleaved raw PCM data.
std::vector<Ui8> build_wav(Ui16 channels, Ui32 sample_rate,
    Ui16 bits_per_sample, const std::vector<Ui8> &samples) {
  Ui16 block_align = channels * (bits_per_sample / 8);
  Ui32 byte_rate = sample_rate * block_align;
  Ui32 data_size = static_cast<Ui32>(samples.size());
  // total = 12 (RIFF header) + 24 (fmt subchunk) + 8 (data header) + data_size
  Ui32 chunk_size = 4 + 24 + 8 + data_size;

  std::vector<Ui8> buf;
  buf.reserve(12 + 24 + 8 + data_size);

  auto push_u32_le = [&](Ui32 v) {
    buf.push_back(static_cast<Ui8>(v & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 8) & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 16) & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 24) & 0xFF));
  };
  auto push_u16_le = [&](Ui16 v) {
    buf.push_back(static_cast<Ui8>(v & 0xFF));
    buf.push_back(static_cast<Ui8>((v >> 8) & 0xFF));
  };
  auto push_tag = [&](const char *tag) {
    buf.push_back(static_cast<Ui8>(tag[0]));
    buf.push_back(static_cast<Ui8>(tag[1]));
    buf.push_back(static_cast<Ui8>(tag[2]));
    buf.push_back(static_cast<Ui8>(tag[3]));
  };

  // RIFF header
  push_tag("RIFF");
  push_u32_le(chunk_size);
  push_tag("WAVE");

  // fmt subchunk
  push_tag("fmt ");
  push_u32_le(16);          // subchunk size
  push_u16_le(1);           // audio_format = PCM
  push_u16_le(channels);
  push_u32_le(sample_rate);
  push_u32_le(byte_rate);
  push_u16_le(block_align);
  push_u16_le(bits_per_sample);

  // data subchunk
  push_tag("data");
  push_u32_le(data_size);
  buf.insert(buf.end(), samples.begin(), samples.end());

  return buf;
}

// Bug 1: LoadWav returns nullptr for any sample_rate != 44100.
//
// The resampling path (line 265) has an inverted bounds check on line 294:
//   if (idx*2*sizeof(Si16) < sample_count * 2 * sizeof(Si16))
// This simplifies to "if (idx < sample_count)", which is always true inside
// the loop "for (idx = 0; idx < sample_count; ++idx)", so LoadWav always
// returns nullptr when resampling is needed.
void test_sound_resample_returns_nullptr() {
  // 4 samples of 16-bit mono silence at 22050 Hz
  std::vector<Ui8> pcm(4 * 2, 0);  // 4 samples * 2 bytes
  std::vector<Ui8> wav = build_wav(1, 22050, 16, pcm);

  std::shared_ptr<SoundInstance> sound = LoadWav(wav.data(),
      static_cast<Si64>(wav.size()));

  // A valid WAV at 22050 Hz should load successfully after resampling to
  // 44100 Hz. The bug causes it to return nullptr instead.
  TEST_CHECK_(sound != nullptr,
      "LoadWav must not return nullptr for a valid WAV at 22050 Hz; "
      "the inverted bounds check on line 294 causes resampling to always fail");
}

// Bug 2: 8-bit stereo WAV reads wrong channel offset.
//
// For 8-bit audio each sample is 1 byte, so the right channel should be
// at in_data + 1 (sizeof(Ui8)). But lines 234-235 and 277-278 use
// sizeof(Ui16) = 2, reading the left channel of the NEXT sample instead.
void test_sound_8bit_stereo_wrong_offset() {
  // 2 stereo samples at 44100 Hz, 8-bit:
  //   sample 0: L=200, R=50
  //   sample 1: L=100, R=150
  // block_align = 2 (1 byte per channel * 2 channels)
  std::vector<Ui8> pcm = {200, 50, 100, 150};
  std::vector<Ui8> wav = build_wav(2, 44100, 8, pcm);

  std::shared_ptr<SoundInstance> sound = LoadWav(wav.data(),
      static_cast<Si64>(wav.size()));
  if (!TEST_CHECK(sound != nullptr)) {
    return;
  }

  Si16 *out = sound->GetWavData();
  if (!TEST_CHECK(out != nullptr)) {
    return;
  }

  // 8-bit WAV PCM is unsigned. Correct conversion: (byte - 128) * 256.
  // For sample 0:
  //   L = (200 - 128) * 256 = 18432
  //   R = (50  - 128) * 256 = -19968
  //
  // The bug reads the right channel from in_data + sizeof(Ui16) (offset 2)
  // instead of in_data + sizeof(Ui8) (offset 1), so it picks up byte 100
  // (left channel of the next sample) instead of byte 50.
  Si16 left_ch_sample0 = out[0];
  Si16 right_ch_sample0 = out[1];

  Si16 expected_left  = (200 - 128) * 256;  // = 18432
  Si16 expected_right = (50  - 128) * 256;  // = -19968

  TEST_CHECK_(left_ch_sample0 == expected_left,
      "Sample 0 left channel: expected %d, got %d",
      (int)expected_left, (int)left_ch_sample0);
  TEST_CHECK_(right_ch_sample0 == expected_right,
      "Sample 0 right channel: expected %d (from byte 50 at offset 1), "
      "got %d (bug reads from wrong offset)",
      (int)expected_right, (int)right_ch_sample0);
}

// Bug 3: 8-bit WAV samples are treated as signed (Si8) instead of
// unsigned (Ui8).
//
// The WAV spec says 8-bit PCM is unsigned: 0=min, 128=silence, 255=max.
// Correct conversion: (Ui8_value - 128) * 256.
// The code casts to Si8* and multiplies by 256, so:
//   128 (silence) -> Si8(-128) * 256 = -32768 (should be 0)
//   0   (min)     -> Si8(0)    * 256 =  0     (should be -32768)
//   255 (max)     -> Si8(-1)   * 256 = -256   (should be +32512)
void test_sound_8bit_signed_vs_unsigned() {
  // 3 mono samples at 44100 Hz, 8-bit:
  //   sample 0: 128 (silence in unsigned 8-bit WAV)
  //   sample 1: 0   (minimum)
  //   sample 2: 255 (maximum)
  std::vector<Ui8> pcm = {128, 0, 255};
  std::vector<Ui8> wav = build_wav(1, 44100, 8, pcm);

  std::shared_ptr<SoundInstance> sound = LoadWav(wav.data(),
      static_cast<Si64>(wav.size()));
  if (!TEST_CHECK(sound != nullptr)) {
    return;
  }

  Si16 *out = sound->GetWavData();
  if (!TEST_CHECK(out != nullptr)) {
    return;
  }

  Si16 silence_sample = out[0];  // sample 0, left channel
  Si16 min_sample = out[2];      // sample 1, left channel
  Si16 max_sample = out[4];      // sample 2, left channel

  // Correct values (unsigned interpretation per WAV spec):
  //   (128 - 128) * 256 =  0
  //   (0   - 128) * 256 = -32768
  //   (255 - 128) * 256 =  32512
  TEST_CHECK_(silence_sample == 0,
      "8-bit silence (128) should convert to 0, got %d",
      (int)silence_sample);
  TEST_CHECK_(min_sample == -32768,
      "8-bit minimum (0) should convert to -32768, got %d",
      (int)min_sample);
  TEST_CHECK_(max_sample == 32512,
      "8-bit maximum (255) should convert to 32512, got %d",
      (int)max_sample);
}

// Path 2: Linux keeps snd_async_add_pcm_handler (SIGIO) when ALSA supports it.
// The handler must mix using only preallocated buffers / atomics — never
// SoundCheck-style malloc. Dedicated thread is only the -ENOSYS fallback.
// The old heap race is reproduced by tests_sigio_repro --legacy-unsafe; the
// fixed signal path is exercised by tests_sigio_repro --safe.
void test_sound_mixer_alsa_async_sigio_is_signal_safe() {
#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
  // The suite uses kHiddenWindow, which already starts the process-wide mixer.
  // Do not Initialize/Deinitialize a second SoundPlayer: that would reopen PCM
  // under a live thread and/or stop the global mixer for later tests.
  const bool async_on = SoundMixerHasAsyncPcmHandler();
  const bool thread_on = SoundMixerIsDedicatedThreadRunning();
  if (g_sound_mixer_state.IsOk() && (async_on || thread_on)) {
    TEST_CHECK_(!(async_on && thread_on),
        "async handler and dedicated thread must not both be active");
    TEST_CHECK_(SoundMixerShouldUseDedicatedThread() == thread_on,
        "ShouldUseDedicatedThread must reflect the ENOSYS thread fallback");
    if (async_on) {
      TEST_MSG("async PCM handler registered (path 2 SIGIO mix)");
    } else {
      TEST_MSG("dedicated thread fallback (snd_async_add_pcm_handler ENOSYS)");
    }
  } else if (!g_sound_mixer_state.IsOk()) {
    TEST_MSG("skipped runtime mixer check: mixer not ok");
  } else {
    TEST_MSG("skipped runtime mixer check: mixer not started");
  }
#else
  TEST_CHECK_(!SoundMixerHasAsyncPcmHandler(),
      "non-ALSA platforms do not register an async PCM handler");
#endif
}

// Path 2: SpmcArray capacity (kPoolCapacity) is larger than the number of
// allocated SoundTasks (kPoolSize), so returning every task via pool.enqueue
// succeeds without a deferred return queue.
void test_sound_mixer_pool_capacity_accepts_full_return() {
  // Pause the process-wide mixer so MixSound cannot steal tasks while this
  // test drains and refills the pool. (Linux ALSA only.)
#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
  const bool mixer_was_running =
      SoundMixerIsDedicatedThreadRunning() || SoundMixerHasAsyncPcmHandler();
  if (mixer_was_running) {
    StopSoundMixer();
  }
#else
  const bool mixer_was_running = false;
  (void)mixer_was_running;
#endif

  SoundTask *taken[SoundMixerState::kPoolSize];
  Si32 taken_n = 0;
  for (; taken_n < SoundMixerState::kPoolSize; ++taken_n) {
    SoundTask *t = g_sound_mixer_state.AllocateSoundTask();
    if (!t) {
      break;
    }
    taken[taken_n] = t;
  }
  TEST_CHECK_(taken_n == SoundMixerState::kPoolSize,
      "must take the entire SoundTask pool (%d), got %d",
      (int)SoundMixerState::kPoolSize, (int)taken_n);
  TEST_CHECK_(g_sound_mixer_state.AllocateSoundTask() == nullptr,
      "pool must be empty after taking all tasks");

  // Enqueue all tasks back into the oversized SpmcArray.
  for (Si32 i = 0; i < taken_n; ++i) {
    TEST_CHECK_(g_sound_mixer_state.pool.enqueue(taken[i]),
        "pool.enqueue must succeed for every returned SoundTask "
        "(kPoolCapacity=%d > kPoolSize=%d)",
        (int)SoundMixerState::kPoolCapacity, (int)SoundMixerState::kPoolSize);
  }

  SoundTask *reclaimed[SoundMixerState::kPoolSize];
  Si32 reclaimed_n = 0;
  for (; reclaimed_n < SoundMixerState::kPoolSize; ++reclaimed_n) {
    SoundTask *t = g_sound_mixer_state.AllocateSoundTask();
    if (!t) {
      break;
    }
    reclaimed[reclaimed_n] = t;
  }
  TEST_CHECK_(reclaimed_n == SoundMixerState::kPoolSize,
      "after full return the full pool must be allocatable again (%d), got %d",
      (int)SoundMixerState::kPoolSize, (int)reclaimed_n);

  // Restore pool for later tests.
  for (Si32 i = 0; i < reclaimed_n; ++i) {
    TEST_CHECK_(g_sound_mixer_state.pool.enqueue(reclaimed[i]),
        "cleanup pool.enqueue must succeed");
  }

#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
  if (mixer_was_running) {
    StartSoundMixer(nullptr);
    g_sound_mixer_state.do_quit.store(false);
    g_sound_mixer_state.is_ok.store(true);
  }
#endif
}

#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
// Path 2: SIGIO errors must fill a preallocated ~1000-byte buffer with detail
// before publishing the atomic; UpdateSoundEngine (via IsOk) promotes + logs once.
void test_sound_mixer_async_error_message_is_detailed() {
  SoundPlayer player;
  // Simulate a SIGIO failure (no signal raised): fill the preallocated buffer,
  // then publish via the atomic. IsOk promotes that buffer into SetError.
  SoundMixerTestReportAsyncError(-EPIPE, "async pcm write failed");
  TEST_CHECK_(!player.IsOk(),
      "IsOk must be false after a reported async mixer error");
  const std::string desc = player.GetErrorDescription();
  TEST_CHECK_(desc.find("Arctic Engine Sound ERROR:") != std::string::npos,
      "error description must include the fixed prefix, got: %s", desc.c_str());
  TEST_CHECK_(desc.find("async pcm write failed") != std::string::npos,
      "error description must include the context phrase, got: %s", desc.c_str());
  TEST_CHECK_(desc.find("code ") != std::string::npos,
      "error description must include the numeric code, got: %s", desc.c_str());
  // -EPIPE should bring in snd_strerror text (e.g. "Broken pipe").
  TEST_CHECK_(desc.find("Broken pipe") != std::string::npos
          || desc.find("Pipe") != std::string::npos
          || desc.find("-32") != std::string::npos,
      "error description should include ALSA strerror or the code, got: %s",
      desc.c_str());
  // Second promote must stay quiet (errno already consumed) and keep IsOk false.
  TEST_CHECK_(!player.IsOk(),
      "IsOk must stay false after the error was promoted");

  // Report sets do_quit; the dedicated-thread loop may have exited. Fully
  // restart the process-wide mixer so later tests still have sound.
  StopSoundMixer();
  StartSoundMixer(nullptr);
  g_sound_mixer_state.do_quit.store(false);
  g_sound_mixer_state.is_ok.store(true);
}
#else
void test_sound_mixer_async_error_message_is_detailed() {
  TEST_MSG("skipped: ALSA async error buffer is Linux-only");
}
#endif
