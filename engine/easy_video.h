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

#ifndef ENGINE_EASY_VIDEO_H_
#define ENGINE_EASY_VIDEO_H_

#include <string>

namespace arctic {

/// @addtogroup global_utility
/// @{

/// @brief Plays a fullscreen video file with letterboxing.
/// Blocks until the video ends or the user skips it (Escape, Space, or mouse click).
/// @param file_name Path to the video file.
/// @return true if the video played to completion, false if skipped by the user.
bool PlayFullscreenVideo(const char *file_name);

/// @brief Plays a fullscreen video file with letterboxing.
/// Blocks until the video ends or the user skips it (Escape, Space, or mouse click).
/// @param file_name Path to the video file.
/// @return true if the video played to completion, false if skipped by the user.
bool PlayFullscreenVideo(const std::string &file_name);

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_VIDEO_H_
