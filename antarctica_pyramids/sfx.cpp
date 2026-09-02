// The MIT License (MIT)
//
// Copyright (c) 2016 - 2026 Huldra
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

#include "sfx.h"

#include <algorithm>
#include <cmath>
#include "engine/arctic_pi.h"

namespace pyramids {

namespace {

Sound g_sfx[kSfxCount];
Sound g_music;
bool g_is_music_enabled = true;

// Every effect is a sum of a tone that slides from one frequency to another
// and a share of white noise, both fading out over the duration. That is
// enough for footsteps, a thrown stone and a bite.
struct Recipe {
  float seconds;
  float freq_begin;
  float freq_end;
  float tone_volume;   // 0..1
  float noise_volume;  // 0..1
  float decay;         // how fast it fades: 1 is linear, higher is sharper
};

Sound Synthesize(const Recipe &recipe) {
  Sound sound;
  sound.Create(recipe.seconds);
  Si32 samples = sound.DurationSamples();
  Si16 *data = sound.RawData();
  float phase = 0.0f;
  for (Si32 i = 0; i < samples; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(samples);
    float freq = recipe.freq_begin + (recipe.freq_end - recipe.freq_begin) * t;
    phase += freq * recipe.seconds / static_cast<float>(samples);
    float envelope = std::pow(1.0f - t, recipe.decay);
    float tone = std::sin(phase * 2.0f * static_cast<float>(kPi))
        * recipe.tone_volume;
    float noise = (static_cast<float>(Random32(0, 65535)) / 32768.0f - 1.0f)
        * recipe.noise_volume;
    float v = (tone + noise) * envelope * 0.6f * 32767.0f;
    Si16 sample = static_cast<Si16>(std::max(-32767.0f, std::min(32767.0f, v)));
    data[i * 2] = sample;
    data[i * 2 + 1] = sample;
  }
  return sound;
}

// Two effects one after another in one sound.
Sound Concatenate(const Sound &first, const Sound &second) {
  Sound a = first;
  Sound b = second;
  Sound sound;
  sound.Create(a.Duration() + b.Duration());
  Si16 *data = sound.RawData();
  Si32 a_samples = a.DurationSamples();
  Si32 b_samples = b.DurationSamples();
  Si32 total = sound.DurationSamples();
  const Si16 *a_data = a.RawData();
  const Si16 *b_data = b.RawData();
  for (Si32 i = 0; i < total; ++i) {
    Si16 left = 0;
    Si16 right = 0;
    if (i < a_samples) {
      left = a_data[i * 2];
      right = a_data[i * 2 + 1];
    } else if (i - a_samples < b_samples) {
      left = b_data[(i - a_samples) * 2];
      right = b_data[(i - a_samples) * 2 + 1];
    }
    data[i * 2] = left;
    data[i * 2 + 1] = right;
  }
  return sound;
}

}  // namespace

void InitSfx() {
  // The noise uses the random generator, and the maze must not depend on
  // whether the sounds were synthesized before it, so the generator's state is
  // put back afterwards.
  RandomState state = GetRandomState();
  const Recipe step = {0.06f, 90.0f, 60.0f, 0.3f, 0.5f, 2.0f};
  const Recipe shot = {0.18f, 400.0f, 80.0f, 0.4f, 0.8f, 3.0f};
  const Recipe kick = {0.12f, 140.0f, 50.0f, 0.9f, 0.2f, 2.0f};
  const Recipe hit = {0.25f, 320.0f, 120.0f, 0.8f, 0.3f, 1.5f};
  const Recipe kill = {0.4f, 200.0f, 30.0f, 0.7f, 0.9f, 1.2f};
  const Recipe pickup_a = {0.07f, 660.0f, 660.0f, 0.6f, 0.0f, 1.0f};
  const Recipe pickup_b = {0.10f, 880.0f, 880.0f, 0.6f, 0.0f, 1.5f};
  const Recipe stairs_a = {0.12f, 440.0f, 440.0f, 0.6f, 0.0f, 1.0f};
  const Recipe stairs_b = {0.12f, 330.0f, 330.0f, 0.6f, 0.0f, 1.0f};
  const Recipe stairs_c = {0.25f, 220.0f, 220.0f, 0.6f, 0.0f, 1.5f};
  const Recipe rest = {0.3f, 220.0f, 230.0f, 0.3f, 0.0f, 1.0f};
  g_sfx[kSfxStep] = Synthesize(step);
  g_sfx[kSfxShot] = Synthesize(shot);
  g_sfx[kSfxKick] = Synthesize(kick);
  g_sfx[kSfxHit] = Synthesize(hit);
  g_sfx[kSfxKill] = Synthesize(kill);
  g_sfx[kSfxPickup] = Concatenate(Synthesize(pickup_a), Synthesize(pickup_b));
  g_sfx[kSfxStairs] = Concatenate(
      Concatenate(Synthesize(stairs_a), Synthesize(stairs_b)),
      Synthesize(stairs_c));
  g_sfx[kSfxRest] = Synthesize(rest);
  SetRandomState(state);
}

void PlaySfx(SfxKind kind) {
  if (kind < 0 || kind >= kSfxCount) {
    return;
  }
  g_sfx[kind].Play();
}

// The track is a git-lfs asset and may be absent from a checkout; the game
// then runs without music, and the checkbox and the slider still work on the
// effects. An empty Sound plays as nothing, so UpdateMusic needs no check.
void LoadMusic(const char *ogg_path) {
  if (DoesFileExist(ogg_path) != kTrivalentTrue) {
    *Log() << "No music: " << ogg_path << " is not there";
    return;
  }
  g_music.Load(ogg_path, false);
}

void SetMusicEnabled(bool is_enabled) {
  g_is_music_enabled = is_enabled;
  if (!is_enabled && g_music.IsPlaying()) {
    g_music.Stop();
  }
}

bool IsMusicEnabled() {
  return g_is_music_enabled;
}

void UpdateMusic() {
  if (g_is_music_enabled && !g_music.IsPlaying()) {
    g_music.Play();
  }
}

}  // namespace pyramids
