// The MIT License (MIT)
//
// Copyright (c) 2016 - 2018 Huldra
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

#ifndef ENGINE_ARCTIC_PLATFORM_H_
#define ENGINE_ARCTIC_PLATFORM_H_

#include <deque>
#include <string>
#include "engine/arctic_types.h"

#include "engine/arctic_platform_byteorder.h"
#include "engine/arctic_platform_fatal.h"
#include "engine/arctic_platform_sound.h"

namespace arctic {

struct DirectoryEntry {
  std::string title;  // entries own full name, like "pet" or "font.tga"
  Trivalent is_directory = kTrivalentUnknown;
  Trivalent is_file = kTrivalentUnknown;
};

void ExitProgram(Si32 exit_code = 0);
void Swap();
bool IsVSyncSupported();
bool SetVSync(bool is_enable);
bool IsFullScreen();
void SetFullScreen(bool is_enable);
bool IsCursorVisible();
void SetCursorVisible(bool is_enable);

Trivalent DoesDirectoryExist(const char *path);
bool MakeDirectory(const char *path);
bool GetCurrentPath(std::string *out_dir);
bool GetDirectoryEntries(const char *path,
    std::deque<DirectoryEntry> *out_entries);
std::string CanonicalizePath(const char *path);
std::string RelativePathFromTo(const char *from, const char *to);

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_H_
