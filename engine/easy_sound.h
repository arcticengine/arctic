// The MIT License (MIT)
//
// Copyright (c) 2017 Huldra
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

#ifndef ENGINE_EASY_SOUND_H_
#define ENGINE_EASY_SOUND_H_

#include <memory>
#include <string>

#include "engine/easy_sound_instance.h"
#include "engine/sound_handle.h"

struct stb_vorbis;

namespace arctic {

/// @addtogroup global_sound
/// @{
class Sound {
 private:
  std::shared_ptr<SoundInstance> sound_instance_;
  stb_vorbis *vorbis_codec_ = nullptr;
  std::shared_ptr<std::string> file_name_ = std::make_shared<std::string>("CLEAR");
 public:
  /// @brief Loads a sound file with the option to unpack it
  /// @param file_name The name of the file to load
  /// @param do_unpack Whether to unpack the sound data
  void Load(const std::string &file_name, bool do_unpack);

  /// @brief Loads a sound file with the option to unpack it and provide input data
  /// @param file_name The name of the file (used for logging only)
  /// @param do_unpack Whether to unpack the sound data
  /// @param in_data Pointer to input data vector
  void Load(const char *file_name, bool do_unpack, std::vector<Ui8> *in_data);

  /// @brief Loads a sound file with the option to unpack it
  /// @param file_name The name of the file to load
  /// @param do_unpack Whether to unpack the sound data
  void Load(const char *file_name, bool do_unpack);

  /// @brief Loads a sound file
  /// @param file_name The name of the file to load
  void Load(const char *file_name);

  /// @brief Loads a sound file
  /// @param file_name The name of the file to load
  void Load(const std::string &file_name);

  /// @brief Creates a sound with a specified duration
  /// @param duration The duration of the sound in seconds
  void Create(double duration);

  /// @brief Clears the sound data
  void Clear();

  /// @brief Plays the sound
  /// @return A handle to the played sound
  SoundHandle Play();

  /// @brief Plays the sound with a specified volume
  /// @param volume The volume to play the sound at, 1.0f is the default
  /// @return A handle to the played sound
  SoundHandle Play(float volume);

  /// @brief Stops the sound
  void Stop();

  /// @brief Gets the duration of the sound in seconds, 0.0 if not loaded
  /// @return The duration of the sound in seconds
  double Duration() const;

  /// @brief Gets the duration of the sound in samples, 0 if not loaded
  /// @return The duration of the sound in samples
  Si32 DurationSamples();

  /// @brief Gets the raw sound data
  /// @return Pointer to the raw sound data
  Si16 *RawData();

  /// @brief Streams out a portion of the sound data
  /// @param offset The starting offset in samples
  /// @param size The number of samples to stream
  /// @param out_buffer The output buffer to write the samples to
  /// @param out_buffer_samples The size of the output buffer in samples
  /// @return The number of samples actually written
  Si32 StreamOut(Si32 offset, Si32 size,
      Si16 *out_buffer, Si32 out_buffer_samples);

  /// @brief Gets the sound instance
  /// @return A shared pointer to the sound instance
  std::shared_ptr<SoundInstance> GetInstance();

  /// @brief Checks if the sound is currently playing
  /// @return True if the sound is playing, false otherwise
  bool IsPlaying();
};
/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_SOUND_H_
