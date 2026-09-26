// Sound loading and resampling.
#define TEST_NO_MAIN
#include "test_helpers.h"

#include "engine/arctic_platform_sound.h"
#include "engine/arctic_mixer.h"
#include <atomic>
#include <cerrno>
#include <functional>
#include <string>
#include <thread>

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

// Linux keeps snd_async_add_pcm_handler (SIGIO) when ALSA supports it.
// The handler must mix using only preallocated buffers / atomics, never
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
      TEST_MSG("async PCM handler registered (SIGIO mix)");
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

// SpmcArray capacity (kPoolCapacity) is larger than the number of
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
// SIGIO errors must fill a preallocated ~1000-byte buffer with detail
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

#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
// In the XRUN state snd_pcm_avail_update returns -EPIPE (-ESTRPIPE when
// suspended). The SIGIO handler never reaches snd_pcm_writei then, so unless
// it requests recovery on that result the sound stays silent forever while
// IsOk still reports success.
void test_sound_mixer_async_underrun_from_avail_requests_recovery() {
  const bool mixer_was_running =
      SoundMixerIsDedicatedThreadRunning() || SoundMixerHasAsyncPcmHandler();
  if (mixer_was_running) {
    StopSoundMixer();
  }
  const bool was_ok = g_sound_mixer_state.IsOk();
  const bool was_quit = g_sound_mixer_state.do_quit.load();
  g_sound_mixer_state.is_ok.store(true);
  SoundMixerTestClearAsyncError();

  SoundPlayer player;
  const Si64 kPeriod = 441;

  const int recoverable[] = {-EPIPE, -ESTRPIPE};
  for (int code : recoverable) {
    TEST_CHECK_(!SoundMixerTestAsyncAvailAllowsWrite(code, kPeriod),
        "avail=%d must not let the handler mix and write", code);
    TEST_CHECK_(SoundMixerTestPeekAsyncRecoverCode() == code,
        "avail=%d must request recovery with the same code, got %d",
        code, SoundMixerTestPeekAsyncRecoverCode());
    TEST_CHECK_(player.IsOk(),
        "avail=%d is recoverable and must not become a mixer error", code);
    SoundMixerTestClearAsyncError();
  }

  const Si64 not_enough[] = {0, 1, kPeriod - 1};
  for (Si64 avail : not_enough) {
    TEST_CHECK_(!SoundMixerTestAsyncAvailAllowsWrite(avail, kPeriod),
        "avail=%lld is less than a period and must wait", (long long)avail);
    TEST_CHECK_(SoundMixerTestPeekAsyncRecoverCode() == 0,
        "avail=%lld must not request recovery", (long long)avail);
  }

  const Si64 enough[] = {kPeriod, kPeriod + 1, kPeriod * 4};
  for (Si64 avail : enough) {
    TEST_CHECK_(SoundMixerTestAsyncAvailAllowsWrite(avail, kPeriod),
        "avail=%lld holds a full period and must be written", (long long)avail);
    TEST_CHECK_(SoundMixerTestPeekAsyncRecoverCode() == 0,
        "avail=%lld must not request recovery", (long long)avail);
  }
  TEST_CHECK_(player.IsOk(), "waiting and writing must not raise an error");

  // Any other negative result is a hard failure, not something prepare fixes.
  TEST_CHECK(!SoundMixerTestAsyncAvailAllowsWrite(-EIO, kPeriod));
  TEST_CHECK_(SoundMixerTestPeekAsyncRecoverCode() == 0,
      "-EIO must not be treated as an underrun, got recover code %d",
      SoundMixerTestPeekAsyncRecoverCode());
  TEST_CHECK_(!player.IsOk(), "-EIO from avail_update must become an error");
  const std::string desc = player.GetErrorDescription();
  TEST_CHECK_(desc.find("avail update failed") != std::string::npos,
      "error description must name avail update, got: %s", desc.c_str());

  SoundMixerTestClearAsyncError();
  g_sound_mixer_state.is_ok.store(was_ok);
  g_sound_mixer_state.do_quit.store(was_quit);
  if (mixer_was_running) {
    StartSoundMixer(nullptr);
  }
}
#else
void test_sound_mixer_async_underrun_from_avail_requests_recovery() {
  TEST_MSG("skipped: ALSA async handler is Linux-only");
}
#endif

#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
// A SoundHandle must capture the task uid before the task is published to the
// mixer. Once enqueued, the mixer may finish a zero-length sound and reset the
// uid (or the task may be reused) before a handle built afterwards reads it.
// A test thread takes the mixer role so the race window is hit many times;
// the platform mixer is stopped because the task queue has a single consumer.
void test_sound_handle_takes_uid_before_publish() {
  const bool mixer_was_running =
      SoundMixerIsDedicatedThreadRunning() || SoundMixerHasAsyncPcmHandler();
  if (mixer_was_running) {
    StopSoundMixer();
  }

  Sound empty;
  empty.Create(0.0);
  TEST_CHECK(empty.GetInstance() != nullptr);
  TEST_CHECK(empty.DurationSamples() == 0);

  std::atomic<bool> stop{false};
  std::thread mixer([&stop]() {
    float mix_l = 0.f;
    float mix_r = 0.f;
    Si16 tmp[2] = {0, 0};
    while (!stop.load(std::memory_order_relaxed)) {
      g_sound_mixer_state.MixSound(&mix_l, &mix_r, 1, 1, tmp);
    }
  });

  struct Case {
    const char *name;
    Si32 iterations;
    std::function<SoundHandle()> start;
  };
  // The 3d branch of the mixer is longer, so it needs more tries to land in
  // the window between publish and a late uid read.
  const Case cases[] = {
    {"StartSound", 4000000, [&empty]() { return StartSound(empty, 1.f); }},
    {"StartSoundLooping", 4000000, [&empty]() { return StartSoundLooping(empty, 1.f); }},
    {"StartSoundAtPosition", 12000000, [&empty]() {
      return StartSoundAtPosition(empty, 1.f, Vec3F(0.f, 0.f, 0.f));
    }},
  };
  for (const Case &c : cases) {
    Si32 started = 0;
    Si32 wrong_uid = 0;
    Ui64 first_wrong = 0;
    Ui64 first_expected = 0;
    for (Si32 i = 0; i < c.iterations; ++i) {
      const Ui64 expected = g_sound_mixer_state.next_uid.load();
      SoundHandle handle = c.start();
      if (g_sound_mixer_state.next_uid.load() == expected) {
        // The task pool was empty for a moment; no task, nothing to check.
        continue;
      }
      ++started;
      if (handle.GetUid() != expected) {
        if (wrong_uid == 0) {
          first_wrong = handle.GetUid();
          first_expected = expected;
        }
        ++wrong_uid;
      }
    }
    TEST_CHECK_(started > c.iterations / 2,
        "%s: too few starts reached the mixer (%d of %d)",
        c.name, (int)started, (int)c.iterations);
    TEST_CHECK_(wrong_uid == 0,
        "%s: %d of %d handles got a uid other than their task's; first got "
        "%llu, expected %llu",
        c.name, (int)wrong_uid, (int)started,
        (unsigned long long)first_wrong, (unsigned long long)first_expected);
  }

  stop.store(true);
  mixer.join();
  float mix_l = 0.f;
  float mix_r = 0.f;
  Si16 tmp[2] = {0, 0};
  for (Si32 i = 0; i < 4; ++i) {
    g_sound_mixer_state.MixSound(&mix_l, &mix_r, 1, 1, tmp);
  }
  TEST_CHECK_(g_sound_mixer_state.buffers.empty(),
      "every zero-length sound must be released, %d left",
      (int)g_sound_mixer_state.buffers.size());
  TEST_CHECK_(!empty.IsPlaying(),
      "the playing count must return to zero after all releases");

  if (mixer_was_running) {
    StartSoundMixer(nullptr);
    g_sound_mixer_state.do_quit.store(false);
    g_sound_mixer_state.is_ok.store(true);
  }
}
#else
void test_sound_handle_takes_uid_before_publish() {
  TEST_MSG("skipped: needs a stoppable platform mixer (Linux ALSA only)");
}
#endif

// The mixer (SIGIO on Linux) calls StreamOut and must not touch the heap.
// Opening a Vorbis decoder allocates, so StreamOut must never open one: a
// stream that was not opened on a game thread stays silent.
void test_sound_vorbis_stream_out_does_not_open_decoder() {
  const std::string path = find_test_data_dir() + "/tone.ogg";
  Sound sound;
  sound.Load(path.c_str(), false);
  TEST_CHECK_(sound.GetInstance() != nullptr, "tone.ogg must load: %s",
      path.c_str());
  TEST_CHECK_(std::fabs(sound.Duration() - 1.0) < 1e-3,
      "tone.ogg is one second long, got %f", sound.Duration());
  std::vector<Si16> out(512 * 2, 0);
  const Si32 written = sound.StreamOut(0, 512, out.data(), 512 * 2);
  TEST_CHECK_(written == 0,
      "StreamOut on an unopened Vorbis stream must return 0, got %d", written);
}

namespace {

std::string ToneOggPath() {
  return find_test_data_dir() + "/tone.ogg";
}

// Streams `frames` frames starting at `offset` and compares them with the
// same range of the sound decoded whole at load time.
bool StreamMatchesUnpacked(Sound *streamed, Sound *unpacked, Si32 offset,
    Si32 frames, Si32 *out_written) {
  std::vector<Si16> out(static_cast<size_t>(frames) * 2, 0);
  const Si32 written = streamed->StreamOut(offset, frames, out.data(),
      frames * 2);
  *out_written = written;
  if (written != frames) {
    return false;
  }
  const Si16 *expected = unpacked->RawData() + static_cast<size_t>(offset) * 2;
  return std::memcmp(out.data(), expected,
      static_cast<size_t>(frames) * 2 * sizeof(Si16)) == 0;
}

}  // namespace

void test_sound_vorbis_stream_matches_unpacked_decode() {
  Sound unpacked;
  unpacked.Load(ToneOggPath().c_str(), true);
  TEST_CHECK_(unpacked.DurationSamples() == 44100,
      "unpacked tone.ogg must have 44100 frames, got %d",
      unpacked.DurationSamples());
  Sound streamed;
  streamed.Load(ToneOggPath().c_str(), false);
  TEST_CHECK(!streamed.IsStreamOpen());
  streamed.OpenStream();
  TEST_CHECK_(streamed.IsStreamOpen(), "OpenStream must open the decoder");

  // Sequential periods, the way the mixer asks for them.
  const Si32 kPeriod = 441;
  Si32 offset = 0;
  bool all_match = true;
  while (offset + kPeriod <= 44100) {
    Si32 written = 0;
    if (!StreamMatchesUnpacked(&streamed, &unpacked, offset, kPeriod,
        &written)) {
      TEST_CHECK_(false, "period at %d differs from the unpacked decode "
          "(written %d)", (int)offset, (int)written);
      all_match = false;
      break;
    }
    offset += written;
  }
  TEST_CHECK(all_match);
  TEST_CHECK_(offset == 44100, "sequential stream must cover the whole "
      "file, got %d frames", (int)offset);

  std::vector<Si16> out(kPeriod * 2, 0);
  TEST_CHECK_(streamed.StreamOut(offset, kPeriod, out.data(), kPeriod * 2) == 0,
      "past the end StreamOut must return 0");
  TEST_CHECK_(streamed.IsStreamOpen(),
      "reaching the end must not close the decoder (closing frees memory)");

  // A jump back (loop restart) and a jump into the middle must seek.
  Si32 written = 0;
  TEST_CHECK_(StreamMatchesUnpacked(&streamed, &unpacked, 0, kPeriod,
      &written), "restart from 0 differs (written %d)", (int)written);
  TEST_CHECK_(StreamMatchesUnpacked(&streamed, &unpacked, 20000, kPeriod,
      &written), "jump to 20000 differs (written %d)", (int)written);
  TEST_CHECK_(StreamMatchesUnpacked(&streamed, &unpacked, 20000 + kPeriod,
      kPeriod, &written), "continuation after the jump differs (written %d)",
      (int)written);

  // Asking for fewer frames than the buffer holds must decode only those.
  Sound partial;
  partial.Load(ToneOggPath().c_str(), false);
  partial.OpenStream();
  std::vector<Si16> big(kPeriod * 2 * 4, 0);
  TEST_CHECK_(partial.StreamOut(0, kPeriod, big.data(), kPeriod * 2 * 4)
      == kPeriod, "StreamOut must honor the requested frame count");
  TEST_CHECK_(StreamMatchesUnpacked(&partial, &unpacked, kPeriod, kPeriod,
      &written), "the next period must follow the requested count "
      "(written %d)", (int)written);
}

void test_sound_copy_does_not_share_decoder() {
  Sound unpacked;
  unpacked.Load(ToneOggPath().c_str(), true);
  Sound original;
  original.Load(ToneOggPath().c_str(), false);
  original.OpenStream();
  TEST_CHECK(original.IsStreamOpen());

  Sound copy(original);
  TEST_CHECK_(!copy.IsStreamOpen(),
      "a copied Sound must start without a decoder");
  TEST_CHECK(copy.GetInstance() == original.GetInstance());
  {
    Sound temporary(original);
    temporary.OpenStream();
    TEST_CHECK(temporary.IsStreamOpen());
  }
  Sound assigned;
  assigned.Load(ToneOggPath().c_str(), false);
  assigned.OpenStream();
  assigned = original;
  TEST_CHECK_(!assigned.IsStreamOpen(),
      "an assigned Sound must drop its own decoder and not take the source's");
  copy.Clear();
  assigned.Clear();

  // The original's decoder survived the copies being destroyed and cleared.
  TEST_CHECK(original.IsStreamOpen());
  Si32 written = 0;
  TEST_CHECK_(StreamMatchesUnpacked(&original, &unpacked, 0, 441, &written),
      "the original must still decode correctly (written %d)", (int)written);
}

// The mixer plays an opened stream, and when it releases the task it hands
// the decoder over instead of closing it; a game thread closes it later.
void test_sound_mixer_retires_vorbis_decoder_to_game_thread() {
  Sound sound;
  sound.Load(ToneOggPath().c_str(), false);
  SoundMixerState state;
  SoundTask *task = state.AllocateSoundTask();
  TEST_CHECK(task != nullptr);
  task->sound = sound;
  task->sound.OpenStream();
  task->volume = 1.f;
  task->action = SoundTaskAction::kStart;
  task->is_playing = true;
  task->sound.GetInstance()->IncPlaying();
  TEST_CHECK(state.AddSoundTask(task));

  const Si32 kPeriod = 441;
  std::vector<float> mix_l(kPeriod, 0.f);
  std::vector<float> mix_r(kPeriod, 0.f);
  std::vector<Si16> tmp(kPeriod * 2, 0);
  float peak = 0.f;
  Si32 periods = 0;
  do {
    state.MixSound(mix_l.data(), mix_r.data(), 1, kPeriod, tmp.data());
    for (Si32 i = 0; i < kPeriod; ++i) {
      peak = std::max(peak, std::max(std::fabs(mix_l[i]), std::fabs(mix_r[i])));
    }
    ++periods;
  } while (!state.buffers.empty() && periods < 1000);
  TEST_CHECK_(peak > 0.05f, "the opened stream must be audible, peak %f",
      peak);
  TEST_CHECK_(periods >= 99 && periods <= 102,
      "one second at 441 frames per period is about 100 periods, got %d",
      (int)periods);
  TEST_CHECK_(state.buffers.empty(), "the task must be released at the end");
  TEST_CHECK_(!task->sound.IsStreamOpen(),
      "the mixer must detach the decoder from the released task");
  TEST_CHECK_(state.retired_stream_count.load() == 1,
      "the detached decoder must wait for a game thread, count %d",
      (int)state.retired_stream_count.load());
  TEST_CHECK(!sound.IsPlaying());

  SoundTask *next = state.AllocateSoundTask();
  TEST_CHECK_(state.retired_stream_count.load() == 0,
      "AllocateSoundTask on a game thread must close parked decoders, "
      "count %d", (int)state.retired_stream_count.load());
  if (next) {
    TEST_CHECK(state.pool.enqueue(next));
  }
}

// Parked decoders of the global mixer must be closed by the per-frame
// UpdateSoundEngine on every platform (ShowFrame calls it), so a game that
// stops starting sounds does not keep dead decoders forever.
// RetireStream is mixer-only; calling it here is safe because no Vorbis sound
// is playing, so the platform mixer never retires anything concurrently.
void test_sound_update_sound_engine_closes_retired_streams() {
  Sound sound;
  sound.Load(ToneOggPath().c_str(), false);
  SoundTask *task = g_sound_mixer_state.AllocateSoundTask();
  TEST_CHECK(task != nullptr);
  if (!task) {
    return;
  }
  TEST_CHECK_(g_sound_mixer_state.retired_stream_count.load() == 0,
      "no parked decoders at the start, count %d",
      (int)g_sound_mixer_state.retired_stream_count.load());
  task->sound = sound;
  task->sound.OpenStream();
  TEST_CHECK(task->sound.IsStreamOpen());
  g_sound_mixer_state.RetireStream(task);
  TEST_CHECK(!task->sound.IsStreamOpen());
  g_sound_mixer_state.ReturnSoundTask(task);
  TEST_CHECK_(g_sound_mixer_state.retired_stream_count.load() == 1,
      "the decoder must be parked, count %d",
      (int)g_sound_mixer_state.retired_stream_count.load());
  UpdateSoundEngine();
  TEST_CHECK_(g_sound_mixer_state.retired_stream_count.load() == 0,
      "UpdateSoundEngine must close parked decoders, count %d",
      (int)g_sound_mixer_state.retired_stream_count.load());
}

#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
// StartSound must open the decoder before publishing the task: the mixer
// no longer opens it, so without that a streamed Vorbis sound is silent.
void test_sound_start_sound_opens_vorbis_stream() {
  const bool mixer_was_running =
      SoundMixerIsDedicatedThreadRunning() || SoundMixerHasAsyncPcmHandler();
  if (mixer_was_running) {
    StopSoundMixer();
  }
  Sound sound;
  sound.Load(ToneOggPath().c_str(), false);
  const Si32 kPeriod = 441;
  std::vector<float> mix_l(kPeriod, 0.f);
  std::vector<float> mix_r(kPeriod, 0.f);
  std::vector<Si16> tmp(kPeriod * 2, 0);
  const char *names[] = {"StartSound", "StartSoundLooping"};
  for (Si32 kind = 0; kind < 2; ++kind) {
    SoundHandle handle = (kind == 0 ? StartSound(sound, 1.f)
                                    : StartSoundLooping(sound, 1.f));
    TEST_CHECK(handle.IsValid());
    float peak = 0.f;
    for (Si32 period = 0; period < 10; ++period) {
      g_sound_mixer_state.MixSound(mix_l.data(), mix_r.data(), 1, kPeriod,
          tmp.data());
      for (Si32 i = 0; i < kPeriod; ++i) {
        peak = std::max(peak,
            std::max(std::fabs(mix_l[i]), std::fabs(mix_r[i])));
      }
    }
    TEST_CHECK_(peak > 0.05f, "%s: a streamed Vorbis sound must be audible, "
        "peak %f", names[kind], peak);
    StopSound(handle);
    g_sound_mixer_state.MixSound(mix_l.data(), mix_r.data(), 1, kPeriod,
        tmp.data());
  }
  TEST_CHECK(g_sound_mixer_state.buffers.empty());
  if (mixer_was_running) {
    StartSoundMixer(nullptr);
    g_sound_mixer_state.do_quit.store(false);
    g_sound_mixer_state.is_ok.store(true);
  }
}
#else
void test_sound_start_sound_opens_vorbis_stream() {
  TEST_MSG("skipped: needs a stoppable platform mixer (Linux ALSA only)");
}
#endif
