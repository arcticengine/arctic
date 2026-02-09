// The MIT License (MIT)
//
// Copyright (c) 2016 - 2022 Huldra
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

#include <vector>
#include <string>
#include "engine/arctic_types.h"

#include "engine/arctic_platform_byteorder.h"
#include "engine/arctic_platform_fatal.h"
#include "engine/arctic_platform_sound.h"

namespace arctic {

/// @addtogroup global_files
/// @{
struct DirectoryEntry {
  std::string title;  ///< entries own full name, like "pet" or "font.tga"
  Trivalent is_directory = kTrivalentUnknown;
  Trivalent is_file = kTrivalentUnknown;
};
/// @}

/// @addtogroup global_utility
/// @{

/// @brief Exits the program
/// @param exit_code Exit code returned to the OS
void ExitProgram(Si32 exit_code = 0);

#ifdef ARCTIC_NO_MAIN
/// @brief Initializes the platform code for headless-mode use
void HeadlessPlatformInit();
#endif

/// @brief Swaps virtual frontbuffer and backbuffer and updates user input
void Swap();

/// @brief Returns true if VSync is supported by the software and hardware
/// @return true if VSync is supported by the software and hardware
bool IsVSyncSupported();

/// @brief Sets the VSync mode
/// @param is_enable true enables VSync, false disables VSync
/// @return true if VSync mode is successfuly set
bool SetVSync(bool is_enable);

/// @brief Returns true if the application is running in Full Screen mode
/// @return true if the application is running in Full Screen mode
bool IsFullScreen();

/// @brief Toggles between the Full Screen mode and the Windowed mode
/// @param is_enable true swithes the application into the Full Screen mode
/// false switches the application into the Windowed mode
void SetFullScreen(bool is_enable);

/// @brief Returns true if the OS mouse cursor (pointer) is visible
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
/// kTrivalentTrue if the directory exists,
/// kTrivalentUnknown if the application can not determine
/// whether the direcotry exists.
Trivalent DoesDirectoryExist(const char *path);

/// @brief Create a directory
/// @param [in] path Path to a directory to create
/// @return true on success
bool MakeDirectory(const char *path);

/// @brief Returns the current path
/// @param [out] out_dir Address of an std::string to fill with the path
/// @return true if the path is successfuly detected, false otherwise
bool GetCurrentPath(std::string *out_dir);

/// @brief List directory entries
/// @param [in] path Path to a direcotry
/// @param [out] out_entries Address of a vector to fill
/// @return true if the path is a directory and its entries are
/// successfuly listed, false otherwise
bool GetDirectoryEntries(const char *path,
    std::vector<DirectoryEntry> *out_entries);

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

/// @brief Returns a path to a parent directory
/// @param [in] path Path to the file or directory to get the parent for
/// @return path to the parent directory for the path provided, or unmodified path if parent path is invalid
std::string ParentPath(const char *path);

/// @brief Returns a path glued from first_part and second_part
/// @param [in] first_part Path to a directory
/// @param [in] second_part name of a file/directory in the first_part directory
/// @return a composite path built by glueing together first_part and second_part
std::string GluePath(const char *first_part, const char *second_part);

/// @brief Finds a system font file by font name
/// @param [in] font_name Name of the font to find (e.g. "Arial", "Helvetica")
/// @return Full path to the font file, or empty string if not found
std::string FindSystemFont(const char *font_name);

/// @}

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_H_
