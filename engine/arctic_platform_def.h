// The MIT License (MIT)
//
// Copyright (c) 2016 - 2017 Huldra
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

#ifndef ENGINE_ARCTIC_PLATFORM_DEF_H_
#define ENGINE_ARCTIC_PLATFORM_DEF_H_

#include "engine/arctic_types.h"

#if defined _WIN32
#define ARCTIC_PLATFORM_WINDOWS
#elif defined __APPLE__
#define ARCTIC_PLATFORM_MACOSX
#elif defined PLATFORM_RPI
#define ARCTIC_PLATFORM_PI
#define ARCTIC_PLATFORM_PI_ES_EGL
#elif defined PLATFORM_LINUX
#define ARCTIC_PLATFORM_PI
#define ARCTIC_PLATFORM_PI_OPENGL_GLX
#endif  // PLATFORM_LINUX

namespace arctic {

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_DEF_H_
