// The MIT License(MIT)
//
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#include "engine/easy_sound.h"

#include <algorithm>
#include <vector>

#include "engine/arctic_platform.h"
#include "engine/easy.h"
#include "engine/easy_sound_instance.h"

#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_MAX_CHANNELS    2
#include "engine/stb_vorbis.inc"

namespace arctic {
namespace easy {

void Sound::Load(const char *file_name) {
    Load(file_name, true);
}

void Sound::Load(const char *file_name, bool do_unpack) {
    Clear();
    Check(!!file_name, "Error in Sound::Load, file_name is nullptr.");
    const char *last_dot = strchr(file_name, '.');
    Check(!!last_dot, "Error in Sound::Load, file_name has no extension.");
    if (strcmp(last_dot, ".wav") == 0) {
        std::vector<Ui8> data = ReadFile(file_name);
        sound_instance_ = LoadWav(data.data(), data.size());
    } else if (strcmp(last_dot, ".ogg") == 0) {
        std::vector<Ui8> data = ReadFile(file_name);
        if (do_unpack) {
            int error = 0;
            vorbis_codec_ = stb_vorbis_open_memory(data.data(),
                static_cast<int>(data.size()), &error, nullptr);
            Si32 size = stb_vorbis_stream_length_in_samples(vorbis_codec_);
            sound_instance_.reset(new SoundInstance(size));
            int res = stb_vorbis_get_samples_short_interleaved(
                vorbis_codec_, 2, sound_instance_->GetWavData(), size * 2);
            stb_vorbis_close(vorbis_codec_);
            vorbis_codec_ = nullptr;
        } else {
            sound_instance_.reset(new SoundInstance(data));
        }
    } else {
        Fatal("Error in Sprite::Load, unknown file extension.");
    }
}

void Sound::Load(const std::string &file_name) {
    Load(file_name.c_str());
}

void Sound::Load(const std::string &file_name, bool do_unpack) {
    Load(file_name.c_str(), do_unpack);
}

void Sound::Create(double duration) {
    Clear();
    double samples = duration * 44100.f + 0.5f;
    // TODO(Huldra): Handle overflows
    sound_instance_.reset(new SoundInstance(static_cast<Si32>(samples)));
    Si16 *data = sound_instance_->GetWavData();
    Si32 size = sound_instance_->GetDurationSamples() * 2;
    for (Si32 idx = 0; idx < size; ++idx) {
        data[idx] = 0;
    }
}

void Sound::Clear() {
    if (vorbis_codec_) {
        stb_vorbis_close(vorbis_codec_);
        vorbis_codec_ = nullptr;
    }
    sound_instance_.reset();
}

void Sound::Play() {
    Play(1.0f);
}

void Sound::Play(float volume) {
    arctic::StartSoundBuffer(*this, volume);
}

void Sound::Stop() {
    arctic::StopSoundBuffer(*this);
}

double Sound::Duration() const {
    Si32 duration_samples = 0;
    switch (sound_instance_->GetFormat()) {
    case kSoundDataWav: {
        duration_samples = sound_instance_->GetDurationSamples();
        break;
    }
    case kSoundDataVorbis: {
        if (vorbis_codec_) {
            duration_samples =
                stb_vorbis_stream_length_in_samples(vorbis_codec_);
        } else {
            int error = 0;
            stb_vorbis *vorbis_codec = stb_vorbis_open_memory(
                sound_instance_->GetVorbisData(),
                sound_instance_->GetVorbisSize(), &error, nullptr);
            duration_samples = stb_vorbis_stream_length_in_samples(
                vorbis_codec_);
            stb_vorbis_close(vorbis_codec);
        }
        break;
    }
    }
    return static_cast<double>(duration_samples) * (1.0 / 44100.0);
}

Si16 *Sound::RawData() {
    return sound_instance_->GetWavData();
}

Si32 Sound::DurationSamples() {
    return sound_instance_->GetDurationSamples();
}

Si32 Sound::StreamOut(Si32 offset, Si32 size,
        Si16 *out_buffer, Si32 out_buffer_samples) {
    switch (sound_instance_->GetFormat()) {
    case kSoundDataWav: {
        Si16 *data = sound_instance_->GetWavData();
        if (!data) {
            return 0;
        }
        if (offset + size > sound_instance_->GetDurationSamples()) {
            return 0;
        }
        Si32 to_copy = std::min(size, out_buffer_samples / 2);
        memcpy(out_buffer, data + offset * 2, to_copy * 4);
        return to_copy;
    }
    case kSoundDataVorbis: {
        int error = 0;
        if (!vorbis_codec_) {
            vorbis_codec_ = stb_vorbis_open_memory(
                sound_instance_->GetVorbisData(),
                sound_instance_->GetVorbisSize(), &error, nullptr);
        }
        stb_vorbis_seek(vorbis_codec_, offset);
        int res = stb_vorbis_get_samples_short_interleaved(
            vorbis_codec_, 2, out_buffer, out_buffer_samples);
        if (res < out_buffer_samples / 2) {
            stb_vorbis_close(vorbis_codec_);
            vorbis_codec_ = nullptr;
        }
        return res;
    }
    }
    Fatal("StreamOut encountered an unknown SoundDataFormat");
    return 0;
}

std::shared_ptr<SoundInstance> Sound::GetInstance() {
    return sound_instance_;
}

bool Sound::IsPlaying() {
    return sound_instance_->IsPlaying();
}

}  // namespace easy
}  // namespace arctic
