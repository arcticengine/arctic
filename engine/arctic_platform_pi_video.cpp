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

#ifdef ARCTIC_PLATFORM_PI

#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>

#include "engine/easy_video.h"
#include "engine/easy_video_internal.h"
#include "engine/engine.h"
#include "engine/easy_advanced.h"
#include "engine/arctic_platform.h"
#include "engine/gl_texture2d.h"
#include "engine/gl_program.h"
#include "engine/gl_buffer.h"
#include "engine/log.h"

#ifdef ARCTIC_HAS_GSTREAMER

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

namespace arctic {

static std::string ResolveVideoPath(const char *file_name) {
  std::string path = file_name;
  FILE *f = fopen(path.c_str(), "rb");
  if (f) {
    fclose(f);
    return path;
  }
  std::string full = GetEngine()->GetInitialPath() + "/" + path;
  return full;
}

static std::string PathToUri(const std::string &path) {
  if (path.find("://") != std::string::npos) {
    return path;
  }
  char *uri = nullptr;
  if (path[0] == '/') {
    uri = g_strdup_printf("file://%s", path.c_str());
  } else {
    char *cwd = g_get_current_dir();
    uri = g_strdup_printf("file://%s/%s", cwd, path.c_str());
    g_free(cwd);
  }
  std::string result(uri);
  g_free(uri);
  return result;
}

bool PlayFullscreenVideo(const char *file_name) {
  gst_init(nullptr, nullptr);

  std::string resolved = ResolveVideoPath(file_name);
  std::string uri = PathToUri(resolved);

  GstElement *pipeline = gst_element_factory_make("playbin", "player");
  if (!pipeline) {
    *Log() << "PlayFullscreenVideo: failed to create playbin element. "
           << "Is GStreamer properly installed?";
    return false;
  }

  GstElement *video_bin = gst_bin_new("video_sink_bin");
  GstElement *videoconvert =
      gst_element_factory_make("videoconvert", "vc");
  GstElement *appsink =
      gst_element_factory_make("appsink", "video_sink");

  if (!videoconvert || !appsink) {
    *Log() << "PlayFullscreenVideo: failed to create GStreamer elements";
    if (videoconvert) {
      gst_object_unref(videoconvert);
    }
    if (appsink) {
      gst_object_unref(appsink);
    }
    gst_object_unref(video_bin);
    gst_object_unref(pipeline);
    return false;
  }

  GstCaps *caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "RGBA",
      nullptr);
  g_object_set(appsink,
      "caps", caps,
      "emit-signals", FALSE,
      "sync", TRUE,
      "max-buffers", 2,
      "drop", TRUE,
      nullptr);
  gst_caps_unref(caps);

  gst_bin_add_many(GST_BIN(video_bin), videoconvert, appsink, nullptr);
  gst_element_link(videoconvert, appsink);

  GstPad *sink_pad = gst_element_get_static_pad(videoconvert, "sink");
  GstPad *ghost_pad = gst_ghost_pad_new("sink", sink_pad);
  gst_element_add_pad(video_bin, ghost_pad);
  gst_object_unref(sink_pad);

  g_object_set(pipeline,
      "uri", uri.c_str(),
      "video-sink", video_bin,
      nullptr);

  GstStateChangeReturn ret =
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    *Log() << "PlayFullscreenVideo: failed to start pipeline for "
           << file_name;
    gst_object_unref(pipeline);
    return false;
  }

  Si32 video_width = 0;
  Si32 video_height = 0;
  bool got_dimensions = false;

  GlTexture2D texture;
  GlProgram program;
  GlBuffer vbo;
  GlBuffer ebo;

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
  gl_FragColor = texture2D(s_texture, v_texCoord);
}
)SHADER";
  program.Create(vs, fs);
  vbo.Create();
  ebo.Create();

  bool skipped = false;
  bool finished = false;

  while (!finished && !skipped) {
    GstSample *sample = gst_app_sink_try_pull_sample(
        GST_APP_SINK(appsink), 10 * GST_MSECOND);

    if (!sample) {
      if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
        finished = true;
        break;
      }
      GstState state = GST_STATE_NULL;
      gst_element_get_state(pipeline, &state, nullptr, 0);
      if (state != GST_STATE_PLAYING && state != GST_STATE_PAUSED) {
        finished = true;
        break;
      }
      skipped = CheckVideoSkipInput();
      continue;
    }

    GstCaps *sample_caps = gst_sample_get_caps(sample);
    if (sample_caps && !got_dimensions) {
      GstStructure *s = gst_caps_get_structure(sample_caps, 0);
      gst_structure_get_int(s, "width", &video_width);
      gst_structure_get_int(s, "height", &video_height);
      if (video_width > 0 && video_height > 0) {
        texture.Create(video_width, video_height);
        got_dimensions = true;
      }
    }

    if (got_dimensions) {
      GstBuffer *buf = gst_sample_get_buffer(sample);
      GstMapInfo map;
      if (gst_buffer_map(buf, &map, GST_MAP_READ)) {
        Si32 expected = video_width * video_height * 4;
        if (static_cast<Si32>(map.size) >= expected) {
          texture.UpdateData(map.data);
        }
        gst_buffer_unmap(buf, &map);
      }

      DrawVideoFrame(texture, program, vbo, ebo,
          video_width, video_height);
      Swap();
    }

    gst_sample_unref(sample);
    skipped = CheckVideoSkipInput();
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return !skipped;
}

}  // namespace arctic

#else  // !ARCTIC_HAS_GSTREAMER

namespace arctic {

bool PlayFullscreenVideo(const char *file_name) {
  *Log() << "PlayFullscreenVideo: GStreamer not available. "
         << "Install gstreamer-1.0, gstreamer-plugins-base, and "
         << "gstreamer-plugins-good to enable video playback. "
         << "Skipping video: " << file_name;
  return false;
}

}  // namespace arctic

#endif  // ARCTIC_HAS_GSTREAMER

#endif  // ARCTIC_PLATFORM_PI
