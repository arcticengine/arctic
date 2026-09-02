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

#ifndef ANTARCTICA_PYRAMIDS_SFX_H_
#define ANTARCTICA_PYRAMIDS_SFX_H_

// Sound: the music track from an ogg file and the effects, which are not
// files at all but samples filled in at startup (Sound::Create plus RawData).
// With ARCTIC_DISABLE_AUDIO in the environment every call here is a no-op
// inside the engine, so the game runs the same, only silently.

#include "engine/easy.h"

namespace pyramids {

using namespace arctic;  // NOLINT

enum SfxKind {
  kSfxStep = 0,
  kSfxShot,
  kSfxKick,
  kSfxHit,
  kSfxKill,
  kSfxPickup,
  kSfxStairs,
  kSfxRest,
  kSfxCount
};

// Synthesizes every effect; call once after the engine is up.
void InitSfx();
void PlaySfx(SfxKind kind);

void LoadMusic(const char *ogg_path);
void SetMusicEnabled(bool is_enabled);
bool IsMusicEnabled();
// Keeps the track looping while it is enabled; call every frame.
void UpdateMusic();

}  // namespace pyramids

#endif  // ANTARCTICA_PYRAMIDS_SFX_H_
