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

#ifdef ARCTIC_PLATFORM_MACOSX

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <chrono>
#include <sstream>
#include <thread>
#include <vector>

#include "engine/easy_video.h"
#include "engine/easy_video_internal.h"
#include "engine/engine.h"
#include "engine/easy_advanced.h"
#include "engine/arctic_platform.h"
#include "engine/gl_texture2d.h"
#include "engine/gl_program.h"
#include "engine/gl_buffer.h"
#include "engine/log.h"

namespace arctic {

bool PlayFullscreenVideo(const char *file_name) {
  @autoreleasepool {
    NSString *path = [NSString stringWithUTF8String:file_name];
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) {
      std::string initial = GetEngine()->GetInitialPath();
      path = [NSString stringWithFormat:@"%s/%s",
          initial.c_str(), file_name];
    }

    NSURL *url = [NSURL fileURLWithPath:path];
    AVAsset *asset = [AVAsset assetWithURL:url];

    __block NSArray<AVAssetTrack *> *videoTracks = nil;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    [asset loadTracksWithMediaType:AVMediaTypeVideo
        completionHandler:^(NSArray<AVAssetTrack *> *tracks, NSError *err) {
      videoTracks = tracks;
      dispatch_semaphore_signal(sem);
    }];
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

    if ([videoTracks count] == 0) {
      *Log() << "PlayFullscreenVideo: no video track found in " << file_name;
      return false;
    }

    NSError *error = nil;
    AVAssetReader *reader =
        [[AVAssetReader alloc] initWithAsset:asset error:&error];
    if (error) {
      *Log() << "PlayFullscreenVideo: AVAssetReader error: "
             << [[error localizedDescription] UTF8String];
      return false;
    }

    AVAssetTrack *videoTrack = videoTracks[0];
    CGSize naturalSize = videoTrack.naturalSize;
    Si32 video_width = static_cast<Si32>(naturalSize.width);
    Si32 video_height = static_cast<Si32>(naturalSize.height);
    float video_fps = videoTrack.nominalFrameRate;
    if (video_fps <= 0.f) {
      video_fps = 30.f;
    }

    NSDictionary *videoSettings = @{
      (NSString *)kCVPixelBufferPixelFormatTypeKey :
          @(kCVPixelFormatType_32BGRA)
    };
    AVAssetReaderTrackOutput *videoOutput =
        [AVAssetReaderTrackOutput
            assetReaderTrackOutputWithTrack:videoTrack
            outputSettings:videoSettings];
    videoOutput.alwaysCopiesSampleData = NO;

    if (![reader canAddOutput:videoOutput]) {
      Log("PlayFullscreenVideo: cannot add video output");
      return false;
    }
    [reader addOutput:videoOutput];

    if (![reader startReading]) {
      *Log() << "PlayFullscreenVideo: failed to start reading: "
             << [[reader.error localizedDescription] UTF8String];
      return false;
    }

    AVPlayer *audioPlayer = [AVPlayer playerWithURL:url];
    [audioPlayer play];

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

    std::vector<Ui8> rgba_buffer(
        static_cast<size_t>(video_width) * video_height * 4);

    while (!finished && !skipped) {
      double elapsed = GetEngine()->GetTime() - start_time;

      CMSampleBufferRef sampleBuffer =
          [videoOutput copyNextSampleBuffer];
      if (!sampleBuffer) {
        finished = true;
        break;
      }

      CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
      double frame_time = CMTimeGetSeconds(pts);

      if (frame_time > elapsed + frame_duration) {
        double sleep_sec = frame_time - elapsed - 0.001;
        if (sleep_sec > 0.0) {
          std::this_thread::sleep_for(
              std::chrono::microseconds(
                  static_cast<int64_t>(sleep_sec * 1000000.0)));
        }
      }

      CVImageBufferRef imageBuffer =
          CMSampleBufferGetImageBuffer(sampleBuffer);
      if (imageBuffer) {
        CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

        void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
        size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
        size_t height = CVPixelBufferGetHeight(imageBuffer);

        if (bytesPerRow == static_cast<size_t>(video_width) * 4) {
          texture.UpdateData(baseAddress);
        } else {
          const Ui8 *src = static_cast<const Ui8 *>(baseAddress);
          Ui8 *dst = rgba_buffer.data();
          size_t row_bytes = static_cast<size_t>(video_width) * 4;
          for (size_t y = 0; y < height; ++y) {
            memcpy(dst, src, row_bytes);
            src += bytesPerRow;
            dst += row_bytes;
          }
          texture.UpdateData(rgba_buffer.data());
        }

        CVPixelBufferUnlockBaseAddress(imageBuffer,
            kCVPixelBufferLock_ReadOnly);
      }

      DrawVideoFrame(texture, program, vbo, ebo,
          video_width, video_height);
      Swap();

      skipped = CheckVideoSkipInput();

      CFRelease(sampleBuffer);
    }

    [audioPlayer pause];
    [reader cancelReading];

    return !skipped;
  }
}

}  // namespace arctic

#endif  // ARCTIC_PLATFORM_MACOSX
