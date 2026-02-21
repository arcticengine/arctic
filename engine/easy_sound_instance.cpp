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
#include <sstream>

#include "engine/arctic_platform.h"
#include "engine/log.h"

namespace arctic {


#pragma pack(1)
struct WaveSubchunkHeader {
  union {
    Ui32 raw;
    char ascii[4];
  } subchunk_id;
  // 0x666d7420 "fmt " - WaveSubchunkFmt
  // 0x64617461 "data" - raw data

  Ui32 subchunk_size;  // rest of the subchunk
};
struct WaveSubchunkFmt {
  WaveSubchunkHeader header;  // 0x666d7420 "fmt "

  Ui16 audio_format;  // 1 == PCM
  Ui16 channels;
  Ui32 sample_rate;
  Ui32 byte_rate;
  Ui16 block_align;  // bytes per sample for all channels
  Ui16 bits_per_sample;
  // Ui16 extra_param_size;  // not for PCM
  // extra_params go here
};

struct WaveHeader {
  union {
    Ui32 raw;  // 0x52494646
    char ascii[4];  // "RIFF"
  } chunk_id;
  Ui32 chunk_size;  // WaveHeader (size - 8) for chunk_id and chunk_size
  union {
    Ui32 raw;  // 0x57415645
    char ascii[4];  // "WAVE"
  } format;
  // WaveSubchunkHeader goes here
};
#pragma pack()

SoundInstance::SoundInstance(Ui32 wav_samples) {
  format_ = kSoundDataWav;
  playing_count_ = 0;
  data_.resize(Ui64(wav_samples) * 2 * sizeof(Si16));
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
  return static_cast<Si32>(data_.size());
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

std::shared_ptr<SoundInstance> LoadWav(const Ui8 *data,
    const Si64 size) {
  if (size < (Si64)sizeof(WaveHeader)) {
    *Log() << "Error in LoadWav, size is too small.";
    return nullptr;
  }
  const WaveHeader *wav = static_cast<const WaveHeader*>(
      static_cast<const void*>(data));
  if (FromBe(wav->chunk_id.raw) != 0x52494646) {
    *Log() << "Error in LoadWav, chunk_id is not RIFF.";
    return nullptr;
  }
  if (wav->chunk_size > size - 8) {
    *Log() << "Error in LoadWav, chunk_size is too large.";
    return nullptr;
  }
  if (FromBe(wav->format.raw) != 0x57415645) {
    *Log() << "Error in LoadWav, format is not WAVE.";
    return nullptr;
  }

  const WaveSubchunkFmt *fmt = nullptr;
  const Ui8 *sound_data = nullptr;
  Si64 sound_data_size = 0;

  const Ui8 *cur_subchunk = data + sizeof(WaveHeader);
  Si64 remaining_chunk_size = (Si64)wav->chunk_size - 4;
  while (remaining_chunk_size > (Si64)sizeof(WaveSubchunkHeader)) {
    const WaveSubchunkHeader *hdr = static_cast<const WaveSubchunkHeader*>(
        static_cast<const void*>(cur_subchunk));

    if (hdr->subchunk_size > remaining_chunk_size - (Si64)sizeof(WaveSubchunkHeader)) {
      *Log() << "Error in LoadWav, subchunk_size is larger than the remaining part of the chunk.";
      return nullptr;
    }
    switch (FromBe(hdr->subchunk_id.raw)) {
      case 0x666d7420: // "fmt"
        if (hdr->subchunk_size != 16) {
          *Log() << "Error in LoadWav, fmt subchunk_size is not 16.";
          return nullptr;
        }
        fmt = static_cast<const WaveSubchunkFmt*>(static_cast<const void*>(cur_subchunk));
        break;
      case 0x64617461: // data
        sound_data = cur_subchunk + sizeof(WaveSubchunkHeader);
        sound_data_size = hdr->subchunk_size;
        break;

      default:
        break;
    }
    Si64 step = hdr->subchunk_size + sizeof(WaveSubchunkHeader);
    remaining_chunk_size -= step;
    cur_subchunk += step;
  }
  if (!fmt) {
    *Log() << "Error in LoadWav, no fmt subchunk found.";
    return nullptr;
  }
  if (!sound_data) {
    *Log() << "Error in LoadWav, no data subchunk found.";
    return nullptr;
  }

  if (fmt->audio_format != 1) {
    *Log() << "Error in LoadWav, fmt audio_format is not 1 (PCM).";
    return nullptr;
  }
  if (fmt->channels <= 0) {
    *Log() << "Error in LoadWav, fmt channels <= 0.";
    return nullptr;
  }
  if ((fmt->bits_per_sample != 8) && (fmt->bits_per_sample != 16)) {
    *Log() << "Error in LoadWav, unsupported bits_per_sample.";
    return nullptr;
  }
  if (fmt->block_align == 0) {
    *Log() << "Error in LoadWav, block_align cannot be 0.";
    return nullptr;
  }
  if (fmt->sample_rate == 0) {
    *Log() << "Error in LoadWav, sample_rate cannot be 0.";
    return nullptr;
  }
  if (fmt->sample_rate < 1000) {
    *Log() << "Error in LoadWav, sample_rate of " << fmt->sample_rate
      << " is too low, use at least 1000";
    return nullptr;
  }

  std::shared_ptr<SoundInstance> sound = nullptr;
  Si64 in_sample_count = sound_data_size / fmt->block_align;
  Ui32 sample_count = (Ui32)(44100ull * (Ui64)in_sample_count
      / (Ui64)fmt->sample_rate);
  sound.reset(new SoundInstance(sample_count));
  const Ui8 *in_data = sound_data;
  Si16 *out_data = sound->GetWavData();
  if (out_data == nullptr) {
    *Log() << "Error in LoadWav, unexpected nullptr from GetWavData";
    return nullptr;
  }
  Ui16 block_align = fmt->block_align;

  if (fmt->sample_rate == 44100) {
    if (fmt->bits_per_sample == 8) {
      if (fmt->channels == 1) {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = (static_cast<Si16>(*in_data) - 128) * 256;
          out_data[idx * 2] = value;
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      } else {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = (static_cast<Si16>(*in_data) - 128) * 256;
          out_data[idx * 2] = value;
          value = (static_cast<Si16>(*(in_data + sizeof(Ui8))) - 128) * 256;
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      }
    } else if (fmt->bits_per_sample == 16) {
      if (fmt->channels == 1) {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = *static_cast<const Si16*>(
              static_cast<const void*>(in_data));
          out_data[idx * 2] = value;
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      } else {
        for (Ui32 idx = 0; idx < sample_count; ++idx) {
          Si16 value = *static_cast<const Si16*>(
              static_cast<const void*>(in_data));
          out_data[idx * 2] = value;
          value = *static_cast<const Si16*>(
              static_cast<const void*>(in_data + sizeof(Si16)));
          out_data[idx * 2 + 1] = value;
          in_data += block_align;
        }
      }
    } else {
      *Log() << "Error in LoadWav, unsupported bits_per_sample";
      return nullptr;
    }
  } else {
    for (Ui64 idx = 0; idx < sample_count; ++idx) {
      Ui64 in_sample_idx = (Ui64)fmt->sample_rate * (Ui64)idx / 44100ull;
      if (in_sample_idx * block_align + block_align >
          static_cast<Ui64>(sound_data_size)) {
        break;
      }
      const Ui8 *in_block = in_data + in_sample_idx * block_align;
      Si16 value1 = 0;
      Si16 value2 = 0;

      if (fmt->bits_per_sample == 8) {
        value1 = (static_cast<Si16>(*in_block) - 128) * 256;
        if (fmt->channels == 1) {
          value2 = value1;
        } else {
          value2 = (static_cast<Si16>(*(in_block + sizeof(Ui8))) - 128) * 256;
        }
      } else if (fmt->bits_per_sample == 16) {
        value1 = *static_cast<const Si16*>(static_cast<const void*>(
              in_block));
        if (fmt->channels == 1) {
          value2 = value1;
        } else {
          value2 = *static_cast<const Si16*>(static_cast<const void*>(
                in_block + sizeof(Si16)));
        }
      } else {
        *Log() << "Error in LoadWav, unsupported bits_per_sample";
        return nullptr;
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

}  // namespace arctic

