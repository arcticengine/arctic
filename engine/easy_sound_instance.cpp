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

#include "engine/easy_sound_instance.h"

#include <cstring>

#include "engine/arctic_platform.h"

namespace arctic {
namespace easy {

#pragma pack(1)
struct WaveHeader {
  union {
    Ui32 raw;  // 0x52494646
    char ascii[4];  // "RIFF"
  } chunk_id;
  Ui32 chunk_size;  // WaveHeader size - 8 for chunk_id and chunk_size (36)
  union {
    Ui32 raw;  // 0x57415645
    char ascii[4];  // "WAVE"
  } format;
  union {
    Ui32 raw;  // 0x666d7420
    char ascii[4];  // "fmt "
  } subchunk_1_id;
  Ui32 subchunk_1_size;  // rest of the subchunk (16)
  Ui16 audio_format;  // 1 == PCM
  Ui16 channels;
  Ui32 sample_rate;
  Ui32 byte_rate;
  Ui16 block_align;  // bytes per sample for all channels
  Ui16 bits_per_sample;
  // Ui16 extra_param_size;  // not for PCM
  // extra_params go here

  union {
    Ui32 raw;  // 0x64617461
    char ascii[4];  // "data"
  } subchunk_2_id;
  Ui32 subchunk_2_size;
  // data goes here
};
#pragma pack()

SoundInstance::SoundInstance(Ui32 wav_samples) {
  format_ = kSoundDataWav;
  playing_count_ = 0;
  data_.resize(wav_samples * 2 * sizeof(Si16));
}

SoundInstance::SoundInstance(std::vector<Ui8> vorbis_file) {
  format_ = kSoundDataVorbis;
  playing_count_ = 0;
  data_.resize(vorbis_file.size());
  std::memcpy(data_.data(), vorbis_file.data(), vorbis_file.size());
}

Si16* SoundInstance::GetWavData() {
  if (format_ == kSoundDataWav) {
    return static_cast<Si16*>(static_cast<void*>(data_.data()));
  } else {
    return nullptr;
  }
}

Ui8* SoundInstance::GetVorbisData() const {
  return const_cast<Ui8*>(data_.data());
}

Si32 SoundInstance::GetVorbisSize() const {
  return static_cast<Ui32>(data_.size());
}

SoundDataFormat SoundInstance::GetFormat() const {
  return format_;
}

Si32 SoundInstance::GetDurationSamples() {
  if (format_ == kSoundDataWav) {
    return static_cast<Si32>(data_.size() / 4);
  } else {
    return 0;
  }
}

std::shared_ptr<easy::SoundInstance> LoadWav(const Ui8 *data,
    const Si64 size) {
  Check(size >= sizeof(WaveHeader), "Error in LoadWav, size is too small.");
  const WaveHeader *wav = static_cast<const WaveHeader*>(
      static_cast<const void*>(data));
  Check(FromBe(wav->chunk_id.raw) == 0x52494646,
      "Error in LoadWav, chunk_id is not RIFF.");
  Check(wav->chunk_size == size - 8,
      "Error in LoadWav, chunk_size is not 36.");
  Check(FromBe(wav->format.raw) == 0x57415645,
      "Error in LoadWav, format is not WAVE.");
  Check(FromBe(wav->subchunk_1_id.raw) == 0x666d7420,
      "Error in LoadWav, subchunk_1_id is not fmt.");
  Check(wav->subchunk_1_size == 16,
      "Error in LoadWav, subchunk_1_size is not 16.");
  Check(wav->audio_format == 1,
      "Error in LoadWav, audio_format is not 1 (PCM).");
  Check(wav->channels > 0,
      "Error in LoadWav, channels <= 0.");
  Check(FromBe(wav->subchunk_2_id.raw) == 0x64617461,
      "Error in LoadWav, subchunk_2_id is not data.");
  Check(wav->subchunk_2_size <= size + 44,
      "Error in LoadWav, subchunk_2_size is too small.");

  Check((wav->bits_per_sample == 8) || (wav->bits_per_sample == 16),
      "Error in LoadWav, unsupported bits_per_sample.");

  std::shared_ptr<easy::SoundInstance> sound;
  Ui32 in_sample_count = wav->subchunk_2_size / wav->block_align;
  Ui32 sample_count = (Ui32)(44100ull * (Ui64)in_sample_count
      / (Ui64)wav->sample_rate);
  sound.reset(new easy::SoundInstance(sample_count));
  const Ui8 *in_data = data + 44;
  Si16 *out_data = sound->GetWavData();
  Ui16 block_align = wav->block_align;

  if (wav->sample_rate == 44100) {
    if (wav->bits_per_sample == 8) {
      if (wav->channels == 1) {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = static_cast<Si16>(*static_cast<const Si8*>(
                static_cast<const void*>(in_data))) * 256;
          out_data[idx * 2] = value;
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      } else {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = static_cast<Si16>(*static_cast<const Si8*>(
                static_cast<const void*>(in_data))) * 256;
          out_data[idx * 2] = value;
          value = static_cast<Si16>(*static_cast<const Si8*>(
                static_cast<const void*>(in_data + sizeof(Ui16)))) * 256;
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      }
    } else if (wav->bits_per_sample == 16) {
      if (wav->channels == 1) {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = *static_cast<const Si16*>(
              static_cast<const void*>(in_data));
          out_data[idx * 2] = value;
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      } else {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = *static_cast<const Ui16*>(
              static_cast<const void*>(in_data));
          out_data[idx * 2] = value;
          value = *static_cast<const Si16*>(
              static_cast<const void*>(in_data + sizeof(Si16)));
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      }
    } else {
      Fatal("Error in LoadWav, unsupported bits_per_sample");
    }
  } else {
    for (Ui32 idx = 0; idx < sample_count; ++idx) {
      Ui64 in_sample_idx = (Ui64)wav->sample_rate * (Ui64)idx / 44100ull;
      const Ui8 *in_block = in_data + in_sample_idx * block_align;
      Si16 value1 = 0;
      Si16 value2 = 0;

      if (wav->bits_per_sample == 8) {
        value1 = static_cast<Si16>(*static_cast<const Si8*>(
              static_cast<const void*>(in_block))) * 256;
        if (wav->channels == 1) {
          value2 = value1;
        } else {
          value2 = static_cast<Si16>(*static_cast<const Si8*>(
                static_cast<const void*>(in_block + sizeof(Ui16)))) * 256;
        }
      } else if (wav->bits_per_sample == 16) {
        value1 = *static_cast<const Si16*>(static_cast<const void*>(
              in_block));
        if (wav->channels == 1) {
          value2 = value1;
        } else {
          value2 = *static_cast<const Si16*>(static_cast<const void*>(
                in_block + sizeof(Si16)));
        }
      } else {
        Fatal("Error in LoadWav, unsupported bits_per_sample");
      }

      out_data[idx * 2] = value1;
      out_data[idx * 2 + 1] = value2;
    }
  }
  return sound;
}

bool SoundInstance::IsPlaying() {
  return (playing_count_.load() != 0);
}

void SoundInstance::IncPlaying() {
  playing_count_.fetch_add(1);
}

void SoundInstance::DecPlaying() {
  playing_count_.fetch_add(-1);
}

}  // namespace easy
}  // namespace arctic
