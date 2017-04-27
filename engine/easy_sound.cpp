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

namespace arctic {
namespace easy {

void Sound::Load(const char *file_name) {
    // TODO(Huldra): Implement this
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
    // TODO(Huldra): Implement this
}

void Sound::Stop() {
    // TODO(Huldra): Implement this
}

double Sound::duration() const {
    // TODO(Huldra): Implement this
    return 0.0f;
}

float *Sound::RawData() {
    // TODO(Huldra): Implement this
    return nullptr;
}

}  // namespace easy
}  // namespace arctic
