// The MIT License (MIT)
//
// Copyright (c) 2016 - 2019 Huldra
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

#ifndef ENGINE_ARCTIC_PLATFORM_SOUND_H_
#define ENGINE_ARCTIC_PLATFORM_SOUND_H_

#include <deque>
#include <string>

#include "engine/arctic_platform_def.h"
#include "engine/easy_sound.h"
#include "engine/mtq_mpmc_befsbfsp_allocator.h"
#include "engine/sound_handle.h"
#include "engine/transform3f.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{
class SoundPlayerImpl;

class AudioDeviceInfo {
 public:
  std::string system_name = "";
  std::string description_for_human = "";
  bool is_input = false;
  bool is_output = false;

  AudioDeviceInfo(const char *in_system_name,
    const char *in_description_for_human,
    bool in_is_input,
    bool in_is_output)
      : system_name(in_system_name)
      , description_for_human(in_description_for_human)
      , is_input(in_is_input)
      , is_output(in_is_output) {
  }
};

class SoundPlayer {
 public:
  std::deque<AudioDeviceInfo> GetDeviceList();
  void Initialize();
  void Initialize(const char *input_device_system_name,
    const char *output_device_system_name);
  void Deinitialize();
  bool IsOk();
  std::string GetErrorDescription();
  ~SoundPlayer();
 protected:
  SoundPlayerImpl *impl = nullptr;
};


/// @brief Per-frame sound engine maintenance, called by Swap on every
/// platform; game thread only.
/// Closes the Vorbis decoders the mixer has parked (the mixer must not free).
/// On Linux ALSA async also recovers SIGIO-deferred underrun/suspend (prepare/
/// resume), then lifts a hard deferred mixer error into IsOk and logs it once.
void UpdateSoundEngine();


/// @brief Starts playback of a sound
/// @param sound Sound to play
/// @param volume Volume to play the sound at.
/// 0.f is silent, 1.f is the original record level.
SoundHandle StartSound(Sound sound, float volume);

/// @brief Starts looping playback of a sound (ring-buffer mode)
/// @param sound Sound to play in a loop
/// @param volume Volume to play the sound at.
/// 0.f is silent, 1.f is the original record level.
SoundHandle StartSoundLooping(Sound sound, float volume);

/// @brief Stops playback of a sound
/// @param sound Sound to play
void StopSound(Sound sound);
void StopSound(const SoundHandle &handle);

void SetSoundListenerLocation(Transform3F location);
void SetSoundSourcePosition(Sound sound, Vec3F position);
void SetSoundSourcePosition(const SoundHandle &handle, Vec3F position);
SoundHandle StartSoundAtPosition(Sound sound, float volume, Vec3F position);

/// @}
/// @addtogroup global_sound
/// @{

/// @brief Sets the master volume level
/// @param volume Volume to set.
void SetMasterVolume(float volume);

/// @brief Gets the master volume level
/// @return The master volume level
float GetMasterVolume();

/// @brief Sets the volume used by built-in GUI click sounds.
/// @param volume Volume to set.
void SetGuiSoundVolume(float volume);

/// @brief Gets the volume used by built-in GUI click sounds.
/// @return The GUI sound volume level.
float GetGuiSoundVolume();

/// @brief Plays asynchronously the note specified for the duration specified.
/// @param duration_seconds Sound duration in seconds.
/// @param note Index of the note to play, index of C4 is 0, index of C#4 is 1, etc.
/// @return The Sound being played.
Sound BeepAsync(float duration_seconds, Si32 note);

/// @brief Plays the note specified for the duration specified.
/// @param duration_seconds Sound duration in seconds.
/// @param note Index of the note to play, index of C4 is 0, index of C#4 is 1, etc.
void Beep(float duration_seconds, Si32 note);

extern template class MpmcNoFallbackFixedSizeBufferFixedSizePool<32, 4080>;
extern template class MpmcBestEffortFixedSizeBufferFixedSizePool<8, 4080>;

/// @}

/// @brief True when the Linux mixer fell back to a dedicated thread because
/// snd_async_add_pcm_handler returned -ENOSYS. False when the async/SIGIO
/// handler (SIGIO) is registered or on platforms without ALSA async.
bool SoundMixerShouldUseDedicatedThread();

/// @brief True while the dedicated Linux mixer thread is running (ENOSYS fallback).
bool SoundMixerIsDedicatedThreadRunning();

/// @brief True if an ALSA async PCM handler is currently registered (SIGIO path).
bool SoundMixerHasAsyncPcmHandler();

#if defined(ARCTIC_PLATFORM_PI) && !defined(ARCTIC_NO_ALSA)
/// Test helper: fill the preallocated SIGIO error buffer and publish it the
/// same way the async handler would (no signal raised).
void SoundMixerTestReportAsyncError(int err_code, const char *context);
/// Test helper: read the preallocated SIGIO error buffer (no promote / Fatal).
const char *SoundMixerTestPeekAsyncErrorMessage();
/// Test helper: drop a pending async errno without UpdateSoundEngine/Fatal.
void SoundMixerTestClearAsyncError();
/// Test helper: run the SIGIO handler's decision on a snd_pcm_avail_update
/// result (may publish a recover code or an async error like the handler).
bool SoundMixerTestAsyncAvailAllowsWrite(Si64 avail, Si64 period_size);
/// Test helper: read the pending underrun/suspend recover code (0 if none).
int SoundMixerTestPeekAsyncRecoverCode();
/// Process-wide mixer control (also used by the platform window startup).
void StartSoundMixer(const char *output_device_name);
void StopSoundMixer();
#endif

#ifdef ARCTIC_TEST_SIGIO_REPRO
/// Test-only: mark whether the main thread is inside malloc/free (repro harness).
void SoundMixerSigioReproSetMainThreadInHeap(bool in_heap);
/// Test-only: size MixSound buffers without opening PCM.
bool SoundMixerSigioReproPrepareBuffers();
/// Test-only: old unsafe path (MixSound + malloc) from a signal handler.
void SoundMixerSigioReproInvokeLegacyUnsafeFromSignal();
/// Test-only: signal-safe MixSound(async_signal_safe) from a signal handler.
void SoundMixerSigioReproInvokeSafeMixFromSignal();
/// Test-only: MixSound from a normal thread.
void SoundMixerSigioReproInvokeMixFromThread();
/// Deprecated alias for InvokeLegacyUnsafeFromSignal.
void SoundMixerSigioReproInvokeMixFromSignal();
#endif  // ARCTIC_TEST_SIGIO_REPRO

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_SOUND_H_
