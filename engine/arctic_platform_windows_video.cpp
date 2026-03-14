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

#ifdef ARCTIC_PLATFORM_WINDOWS

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")

#include <chrono>
#include <sstream>
#include <thread>
#include <vector>
#include <string>

#include "engine/easy_video.h"
#include "engine/easy_video_internal.h"
#include "engine/engine.h"
#include "engine/easy_advanced.h"
#include "engine/easy_sound.h"
#include "engine/arctic_platform.h"
#include "engine/gl_texture2d.h"
#include "engine/gl_program.h"
#include "engine/gl_buffer.h"
#include "engine/log.h"

namespace arctic {

static std::wstring Utf8ToWide(const std::string &utf8) {
  std::wstring wide;
  wide.reserve(utf8.size());
  for (size_t i = 0; i < utf8.size(); ++i) {
    wide.push_back(static_cast<wchar_t>(
        static_cast<unsigned char>(utf8[i])));
  }
  return wide;
}

static std::wstring ResolveVideoPath(const char *file_name) {
  std::wstring wide = Utf8ToWide(file_name);
  DWORD attr = GetFileAttributesW(wide.c_str());
  if (attr != INVALID_FILE_ATTRIBUTES) {
    return wide;
  }
  std::string full = GetEngine()->GetInitialPath() + "/" + file_name;
  return Utf8ToWide(full);
}

static bool SetupVideoStream(IMFSourceReader *reader,
    Si32 *out_width, Si32 *out_height, float *out_fps) {
  IMFMediaType *mediaType = nullptr;
  HRESULT hr = MFCreateMediaType(&mediaType);
  if (FAILED(hr)) {
    return false;
  }
  mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
  hr = reader->SetCurrentMediaType(
      (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType);
  mediaType->Release();
  if (FAILED(hr)) {
    return false;
  }

  IMFMediaType *currentType = nullptr;
  hr = reader->GetCurrentMediaType(
      (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType);
  if (FAILED(hr)) {
    return false;
  }

  UINT32 w = 0, h = 0;
  MFGetAttributeSize(currentType, MF_MT_FRAME_SIZE, &w, &h);
  *out_width = static_cast<Si32>(w);
  *out_height = static_cast<Si32>(h);

  UINT32 num = 0, den = 0;
  MFGetAttributeRatio(currentType, MF_MT_FRAME_RATE, &num, &den);
  if (den > 0) {
    *out_fps = static_cast<float>(num) / static_cast<float>(den);
  } else {
    *out_fps = 30.f;
  }

  currentType->Release();
  return true;
}

static bool SetupAudioStream(IMFSourceReader *reader) {
  IMFMediaType *audioType = nullptr;
  HRESULT hr = MFCreateMediaType(&audioType);
  if (FAILED(hr)) {
    return false;
  }
  audioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  audioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
  audioType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
  audioType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
  audioType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
  audioType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4);
  audioType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100 * 4);
  hr = reader->SetCurrentMediaType(
      (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioType);
  audioType->Release();
  return SUCCEEDED(hr);
}

static Sound DecodeAudioTrack(const std::wstring &path) {
  Sound audio_sound;
  IMFSourceReader *audio_reader = nullptr;
  HRESULT hr = MFCreateSourceReaderFromURL(
      path.c_str(), nullptr, &audio_reader);
  if (FAILED(hr) || !audio_reader) {
    return audio_sound;
  }

  if (!SetupAudioStream(audio_reader)) {
    audio_reader->Release();
    return audio_sound;
  }

  std::vector<Si16> pcm_data;
  pcm_data.reserve(44100 * 2 * 60);

  for (;;) {
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    IMFSample *sample = nullptr;
    hr = audio_reader->ReadSample(
        (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
        0, nullptr, &flags, &timestamp, &sample);
    if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
      if (sample) {
        sample->Release();
      }
      break;
    }
    if (sample) {
      IMFMediaBuffer *buf = nullptr;
      hr = sample->ConvertToContiguousBuffer(&buf);
      if (SUCCEEDED(hr)) {
        BYTE *data = nullptr;
        DWORD len = 0;
        hr = buf->Lock(&data, nullptr, &len);
        if (SUCCEEDED(hr)) {
          Si32 num_samples = static_cast<Si32>(len) /
              static_cast<Si32>(sizeof(Si16));
          size_t old_size = pcm_data.size();
          pcm_data.resize(old_size + num_samples);
          memcpy(pcm_data.data() + old_size, data,
              num_samples * sizeof(Si16));
          buf->Unlock();
        }
        buf->Release();
      }
      sample->Release();
    }
  }

  audio_reader->Release();

  if (pcm_data.size() >= 2) {
    Si32 total_stereo_samples =
        static_cast<Si32>(pcm_data.size()) / 2;
    double duration =
        static_cast<double>(total_stereo_samples) / 44100.0;
    audio_sound.Create(duration);
    Si16 *dst = audio_sound.RawData();
    if (dst) {
      memcpy(dst, pcm_data.data(),
          pcm_data.size() * sizeof(Si16));
    }
  }

  return audio_sound;
}

bool PlayFullscreenVideo(const char *file_name) {
  HRESULT hr = MFStartup(MF_VERSION);
  if (FAILED(hr)) {
    Log("PlayFullscreenVideo: MFStartup failed");
    return false;
  }

  std::wstring resolved_path = ResolveVideoPath(file_name);

  Sound audio_sound = DecodeAudioTrack(resolved_path);
  SoundHandle audio_handle = audio_sound.Play();

  IMFSourceReader *reader = nullptr;
  hr = MFCreateSourceReaderFromURL(
      resolved_path.c_str(), nullptr, &reader);
  if (FAILED(hr) || !reader) {
    audio_sound.Stop();
    MFShutdown();
    return false;
  }

  Si32 video_width = 0;
  Si32 video_height = 0;
  float video_fps = 30.f;

  if (!SetupVideoStream(reader, &video_width, &video_height, &video_fps)) {
    audio_sound.Stop();
    reader->Release();
    MFShutdown();
    return false;
  }

  GlTexture2D texture;
  texture.Create(video_width, video_height);

  GlProgram program;
  const char vs[] = R"SHADER(
#ifdef GL_ES
#endif
attribute vec3 vPosition;
attribute vec2 vTex;
varying vec2 v_texCoord;
void main() {
  gl_Position = vec4(vPosition, 1.0);
  v_texCoord = vTex;
}
)SHADER";
  const char fs[] = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texCoord;
uniform sampler2D s_texture;
void main() {
  vec4 c = texture2D(s_texture, v_texCoord);
  gl_FragColor = vec4(c.b, c.g, c.r, 1.0);
}
)SHADER";
  program.Create(vs, fs);

  GlBuffer vbo;
  vbo.Create();
  GlBuffer ebo;
  ebo.Create();

  double frame_duration = 1.0 / static_cast<double>(video_fps);
  double start_time = GetEngine()->GetTime();
  bool skipped = false;
  bool finished = false;

  std::vector<Ui8> row_buffer;

  while (!finished && !skipped) {
    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    IMFSample *sample = nullptr;

    hr = reader->ReadSample(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0, &streamIndex, &flags, &timestamp, &sample);

    if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
      finished = true;
      if (sample) {
        sample->Release();
      }
      break;
    }

    if (sample) {
      double frame_time = static_cast<double>(timestamp) / 10000000.0;
      double elapsed = GetEngine()->GetTime() - start_time;
      if (frame_time > elapsed + frame_duration) {
        double sleep_sec = frame_time - elapsed - 0.001;
        if (sleep_sec > 0.0) {
          std::this_thread::sleep_for(
              std::chrono::microseconds(
                  static_cast<int64_t>(sleep_sec * 1000000.0)));
        }
      }

      IMFMediaBuffer *buffer = nullptr;
      hr = sample->ConvertToContiguousBuffer(&buffer);
      if (SUCCEEDED(hr)) {
        BYTE *data = nullptr;
        DWORD length = 0;
        hr = buffer->Lock(&data, nullptr, &length);
        if (SUCCEEDED(hr)) {
          Si32 expected_stride = video_width * 4;
          Si32 expected_size = expected_stride * video_height;
          if (static_cast<Si32>(length) >= expected_size) {
            if (row_buffer.size() <
                static_cast<size_t>(expected_size)) {
              row_buffer.resize(static_cast<size_t>(expected_size));
            }
            for (Si32 y = 0; y < video_height; ++y) {
              memcpy(row_buffer.data() + y * expected_stride,
                  data + (video_height - 1 - y) * expected_stride,
                  static_cast<size_t>(expected_stride));
            }
            texture.UpdateData(row_buffer.data());
          }
          buffer->Unlock();
        }
        buffer->Release();
      }
      sample->Release();

      DrawVideoFrame(texture, program, vbo, ebo,
          video_width, video_height);
      Swap();

      skipped = CheckVideoSkipInput();
    }
  }

  audio_sound.Stop();

  if (reader) {
    reader->Release();
  }
  MFShutdown();

  return !skipped;
}

}  // namespace arctic

#endif  // ARCTIC_PLATFORM_WINDOWS
