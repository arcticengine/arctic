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

namespace arctic {
namespace easy {

void Sound::Load(const char *file_name) {
    Check(!!file_name, "Error in Sound::Load, file_name is nullptr.");
    const char *last_dot = strchr(file_name, '.');
    Check(!!last_dot, "Error in Sound::Load, file_name has no extension.");
    if (strcmp(last_dot, ".wav") == 0) {
        std::vector<Ui8> data = ReadFile(file_name);
        sound_instance_ = LoadWav(data.data(), data.size());
    } else if(strcmp(last_dot, ".ogg") == 0) {
        std::vector<Ui8> data = ReadFile(file_name);
        sound_instance_.reset(new SoundInstance(data));
    } else {
        Fatal("Error in Sprite::Load, unknown file extension.");
    }
}

void Sound::Load(const std::string &file_name) {
    Load(file_name.c_str());
}

void Sound::Create(double duration) {
    // TODO(Huldra): Implement this
}

void Sound::Clear() {
    // TODO(Huldra): Implement this
}

void Sound::Play() {
    Play(1.0f);
}

void Sound::Play(float volume) {
    arctic::StartSoundBuffer(*this, volume);
}

void Sound::Stop() {
    // TODO(Huldra): Implement this
}

double Sound::duration() const {
    // TODO(Huldra): Implement this
    return 0.0f;
}

Si16 *Sound::RawData() {
    return sound_instance_->GetWavData();
}

Si32 Sound::DurationSamples() {
    return sound_instance_->GetDurationSamples();
}

Si32 Sound::StreamOut(Si32 offset, Si32 size, Si16 *out_buffer, Si32 out_buffer_samples) {
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

}  // namespace easy
}  // namespace arctic
