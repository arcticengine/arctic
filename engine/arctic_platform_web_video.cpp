// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2026 Huldra
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

#include "engine/arctic_platform_def.h"

#ifdef ARCTIC_PLATFORM_WEB

#include <emscripten.h>
#include <string>

#include "engine/easy_video.h"
#include "engine/log.h"

EM_ASYNC_JS(int, web_play_fullscreen_video, (const char *fname_ptr), {
  var fname = UTF8ToString(fname_ptr);

  var overlay = document.createElement('div');
  overlay.style.position = 'fixed';
  overlay.style.top = '0';
  overlay.style.left = '0';
  overlay.style.width = '100vw';
  overlay.style.height = '100vh';
  overlay.style.backgroundColor = 'black';
  overlay.style.zIndex = '9999';
  overlay.style.display = 'flex';
  overlay.style.alignItems = 'center';
  overlay.style.justifyContent = 'center';
  overlay.style.cursor = 'pointer';
  var label = document.createElement('div');
  label.textContent = 'Click to play';
  label.style.color = 'white';
  label.style.fontSize = '2em';
  label.style.fontFamily = 'sans-serif';
  overlay.appendChild(label);
  document.body.appendChild(overlay);

  await new Promise(function(waitResolve) {
    function onGesture() {
      overlay.removeEventListener('click', onGesture);
      document.removeEventListener('keydown', onGestureKey);
      waitResolve();
    }
    function onGestureKey(e) {
      if (e.key === ' ' || e.key === 'Enter') {
        onGesture();
      }
    }
    overlay.addEventListener('click', onGesture);
    document.addEventListener('keydown', onGestureKey);
  });

  overlay.remove();

  return await new Promise(function(resolve) {
    var video = document.createElement('video');
    video.src = fname;
    video.style.position = 'fixed';
    video.style.top = '0';
    video.style.left = '0';
    video.style.width = '100vw';
    video.style.height = '100vh';
    video.style.objectFit = 'contain';
    video.style.backgroundColor = 'black';
    video.style.zIndex = '9999';
    video.playsInline = true;

    var done = false;
    function cleanup(skipped) {
      if (done) {
        return;
      }
      done = true;
      video.pause();
      video.remove();
      resolve(skipped ? 0 : 1);
    }

    video.addEventListener('ended', function() { cleanup(false); });
    video.addEventListener('error', function() { cleanup(false); });

    video.addEventListener('click', function() { cleanup(true); });
    document.addEventListener('keydown', function onKey(e) {
      if (e.key === 'Escape' || e.key === ' ') {
        document.removeEventListener('keydown', onKey);
        cleanup(true);
      }
    });

    document.body.appendChild(video);

    var playPromise = video.play();
    if (playPromise !== undefined) {
      playPromise.catch(function() {
        cleanup(false);
      });
    }
  });
});

namespace arctic {

bool PlayFullscreenVideo(const char *file_name) {
  int result = web_play_fullscreen_video(file_name);
  return result != 0;
}

}  // namespace arctic

#endif  // ARCTIC_PLATFORM_WEB
