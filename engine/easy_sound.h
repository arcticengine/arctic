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

#include <string>
#include <memory>

#include "engine/easy_sound_instance.h"

struct stb_vorbis;

namespace arctic {
namespace easy {

class Sound {
 private:
  std::shared_ptr<SoundInstance> sound_instance_;
  stb_vorbis *vorbis_codec_ = nullptr;
 public:
  void Load(const std::string &file_name, bool do_unpack);
  void Load(const char *file_name, bool do_unpack);
  void Load(const char *file_name);
  void Load(const std::string &file_name);
  void Create(double duration);
  void Clear();
  void Play();
  void Play(float volume);
  void Stop();
  double Duration() const;
  Si32 DurationSamples();
  Si16 *RawData();
  Si32 StreamOut(Si32 offset, Si32 size,
      Si16 *out_buffer, Si32 out_buffer_samples);
  std::shared_ptr<SoundInstance> GetInstance();
  bool IsPlaying();
};

}  // namespace easy
}  // namespace arctic

#endif  // ENGINE_EASY_SOUND_H_
