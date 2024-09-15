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

#ifndef ENGINE_EASY_SOUND_INSTANCE_H_
#define ENGINE_EASY_SOUND_INSTANCE_H_

#include <atomic>
#include <memory>
#include <vector>

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

/// @brief Enum representing the format of sound data
enum SoundDataFormat {
  kSoundDataWav,    ///< WAV format
  kSoundDataVorbis  ///< Vorbis format
};

/// @brief Class representing a sound instance
class SoundInstance {
  SoundDataFormat format_;
  std::vector<Ui8> data_;
  std::atomic<Si32> playing_count_;
 public:
  /// @brief Constructor for WAV sound instance
  /// @param wav_samples Number of WAV samples
  explicit SoundInstance(Ui32 wav_samples);

  /// @brief Constructor for Vorbis sound instance
  /// @param vorbis_file Vector containing Vorbis file data
  explicit SoundInstance(std::vector<Ui8> vorbis_file);

  /// @brief Get pointer to WAV data
  /// @return Pointer to Si16 WAV data, nullptr if format is not WAV
  Si16* GetWavData();

  /// @brief Get pointer to Vorbis data
  /// @return Pointer to Ui8 Vorbis data, nullptr if format is not Vorbis
  Ui8* GetVorbisData() const;

  /// @brief Get size of Vorbis data
  /// @return Size of Vorbis data in bytes, 0 if format is not Vorbis
  Si32 GetVorbisSize() const;

  /// @brief Get format of sound data
  /// @return SoundDataFormat enum value
  SoundDataFormat GetFormat() const;

  /// @brief Get duration of sound in samples
  /// @return Duration in samples, 0 if format is not WAV
  Si32 GetDurationSamples();

  /// @brief Check if sound is currently playing
  /// @return True if sound is playing, false otherwise
  bool IsPlaying();

  /// @brief Increment playing count, used internally by EasySound
  void IncPlaying();

  /// @brief Decrement playing count, used internally by EasySound
  void DecPlaying();
};


/// @brief Creates a sound instance from WAV data
/// @param data Pointer to WAV data
/// @param size Size of WAV data in bytes
/// @return Shared pointer to created SoundInstance
std::shared_ptr<SoundInstance> LoadWav(const Ui8 *data,
    const Si64 size);

/// @}

}  // namespace arctic


#endif  // ENGINE_EASY_SOUND_INSTANCE_H_
