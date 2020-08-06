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
  std::string title;  ///< entries own full name, like "pet" or "font.tga"
  Trivalent is_directory = kTrivalentUnknown;
  Trivalent is_file = kTrivalentUnknown;
};

/// @addtogroup global_utility
/// @{

/// @brief Exits the program
/// @param exit_code Exit code returned to the OS
void ExitProgram(Si32 exit_code = 0);

/// @brief Swaps virtual frontbuffer and backbuffer and updates user input
void Swap();

/// @return true if VSync is supported by the software and hardware
bool IsVSyncSupported();

/// @brief Sets the VSync mode
/// @param is_enable true enables VSync, false disables VSync
/// @return true if VSync mode is successfuly set
bool SetVSync(bool is_enable);

/// @return true if the application is running in Full Screen mode
bool IsFullScreen();

/// @brief Toggles between the Full Screen mode and the Windowed mode
/// @param is_enable true swithes the application into the Full Screen mode
/// false switches the application into the Windowed mode
void SetFullScreen(bool is_enable);

/// @return true if the OS mouse cursor (pointer) is visible
bool IsCursorVisible();

/// @brief Sets OS mouse cursor visibility
/// @param is_enable true makes the cursor visible, false hides it
void SetCursorVisible(bool is_enable);

/// @}
/// @addtogroup global_files
/// @{

/// @brief Checks if a filesystem directory exists
/// @param [in] path Path to a directory
/// @return kTrivalentFalse if the directory does not exist,
/// kTrivalentTrue if the directory does not exist,
/// kTrivalentUnknown if the application can not determine
/// whether the direcotry exists.
Trivalent DoesDirectoryExist(const char *path);

/// @brief Create a directory
/// @param [in] path Path to a directory to create
/// @return true on success
bool MakeDirectory(const char *path);

/// @brief returns the current path
/// @param [out] out_dir Address of an std::string to fill with the path
/// @return true if the path is successfuly detected, false otherwise
bool GetCurrentPath(std::string *out_dir);

/// @brief List directory entries
/// @param [in] path Path to a direcotry
/// @param [out] out_entries Address of a deque to fill
/// @return true if the path is a directory and its entries are
/// successfuly listed, false otherwise
bool GetDirectoryEntries(const char *path,
    std::deque<DirectoryEntry> *out_entries);

/// @brief Transforms the path into it's canonical form
/// @param [in] path Path to transform
/// @return The canonical form of the path specified
std::string CanonicalizePath(const char *path);

/// @brief Creates a relative path to a file or directory
/// The path is transformed into a relative form so that
/// it leads *from* the source directory *to* the target file or directory
/// @param [in] from Path to the source directory
/// @param [in] to Path to the destination directory
/// @return relative path *from* source *to* target
std::string RelativePathFromTo(const char *from, const char *to);

/// @}

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_H_
