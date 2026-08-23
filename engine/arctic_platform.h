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
///
/// The process ends with the code given, and the code is what the shell sees,
/// so a console subcommand can report a failure the usual way. The log is
/// flushed and the sound device is released on the way out, which is why this
/// is the way to leave and std::exit or std::_Exit are not.
///
/// EasyMain can not return a value (it is void), so returning from it is the
/// same as ExitProgram(0). Note that global destructors do run, as they do for
/// any exit, so a global that outlives EasyMain must be able to destruct.
void ExitProgram(Si32 exit_code = 0);

/// @brief Type of the function that answers "start without a window?"
using HeadlessDecider = bool (*)();

/// @brief Registers the function the engine asks before it creates the window
/// @param decider Function returning true to start without a window, or nullptr
/// @return true always, so that a global can be initialized with the call
///
/// A GUI application often has console subcommands: `mygame convert a.png b.tga`
/// has no business opening a window, and a window that opens and closes right
/// away is worse than none on a machine with no display at all. The engine asks
/// the decider before it creates the window, the GL context and the sound
/// device; when it answers true, none of the three is created, the backbuffer is
/// a plain piece of memory, and GetEngine()->IsHeadless() stays true for the
/// whole run.
///
/// The catch is the timing: the decision is needed before EasyMain is called, so
/// the registration has to happen before main runs. That is what the
/// ARCTIC_HEADLESS_DECIDER macro is for. The command line is already there to
/// look at, through GetEngine()->GetArgc() and GetEngine()->GetArgv().
///
/// @code
/// bool IsConsoleSubcommand() {
///   Engine *engine = GetEngine();
///   for (Si32 i = 1; i < engine->GetArgc(); ++i) {
///     const std::string arg = engine->GetArgv()[i];
///     if (arg == "convert" || arg == "test") {
///       return true;
///     }
///   }
///   return false;
/// }
/// ARCTIC_HEADLESS_DECIDER(IsConsoleSubcommand)
///
/// void EasyMain() {
///   if (GetEngine()->IsHeadless()) {
///     RunTheSubcommand();  // draws nothing, ends with ExitProgram(code)
///     return;
///   }
///   ...
/// }
/// @endcode
///
/// Setting both the ARCTIC_HEADLESS and the ARCTIC_DISABLE_HW environment
/// variables asks for the same thing from outside, without a decider.
bool SetHeadlessDecider(HeadlessDecider decider);

/// @brief Registers a headless decider before main runs
///
/// Put it at namespace scope in one of the translation units of the application,
/// next to the decider itself. See SetHeadlessDecider.
#define ARCTIC_HEADLESS_DECIDER(decider_function)                    \
  namespace {                                                        \
  const bool g_arctic_headless_decider_registered =                  \
      ::arctic::SetHeadlessDecider(decider_function);                 \
  }  // namespace

/// @brief Asks the registered decider and the environment about the window
/// @return true if this run must have no window, no GL context and no sound
///
/// Called by the engine startup code before it creates anything; an application
/// asks GetEngine()->IsHeadless() instead, at any time after EasyMain begins.
bool IsHeadlessStartupRequested();

/// @brief Type of the function that answers "close the window now?"
using MainWindowCloseHandler = bool (*)();

/// @brief Registers the function the engine asks when the window is closed
/// @param handler Function returning true to let the engine end the process
///   right now, or nullptr to go back to the default behaviour
///
/// The user closes the window with the red button, with Cmd+Q, with Alt+F4 or
/// with the window manager, and by default the engine ends the process there and
/// then. An application that has something to do first (ask about unsaved work,
/// finish the frame, write a save file, leave with a code of its own) registers a
/// handler and returns false from it: nothing is closed, the frame goes on, and
/// IsMainWindowCloseRequested() keeps returning true so that the main loop can
/// end when the application is ready.
///
/// The handler is called from the thread that pumps the window messages. On
/// macOS and on Linux that is the thread EasyMain runs on, because the messages
/// are pumped from inside ShowFrame, and the handler is as free to touch the
/// application data as the frame itself is. On Windows the window belongs to the
/// main thread while EasyMain runs on a thread of its own, so the handler runs in
/// parallel with the frame and has to treat everything it reads or writes as
/// shared with another thread. A handler that only sets a flag, or that is not
/// there at all while the main loop watches IsMainWindowCloseRequested(), needs
/// no such care and behaves the same everywhere. The handler is called once per
/// request; a user who clicks the button twice gets two calls.
///
/// @code
/// std::atomic<bool> g_is_save_dialog_shown{false};  // written by the handler
///
/// bool OnClose() {
///   if (!g_has_unsaved_work) {
///     return true;  // nothing to lose, let the engine close
///   }
///   g_is_save_dialog_shown.store(true);
///   return false;  // the application decides when to leave
/// }
///
/// void EasyMain() {
///   SetMainWindowCloseHandler(OnClose);
///   while (!IsMainWindowCloseRequested() || g_is_save_dialog_shown.load()) {
///     ...
///     ShowFrame();
///   }
/// }
/// @endcode
void SetMainWindowCloseHandler(MainWindowCloseHandler handler);

/// @brief Tells whether the user has asked to close the main window
/// @return true once the window was asked to close, false before that
///
/// Reading it is the way for a main loop to end on its own terms:
/// `while (!IsMainWindowCloseRequested())`. It only ever goes from false to true,
/// and it stays true afterwards. Note that without a handler registered with
/// SetMainWindowCloseHandler the engine ends the process immediately on the
/// request, so a loop that wants to see the flag has to register one.
///
/// Escape is not part of this: no key closes anything by itself, and a program
/// that ends on Escape checks IsKeyDownward(kKeyEscape) because it decided to,
/// not because the engine did.
bool IsMainWindowCloseRequested();

/// @brief Reports a close request from the platform window code
/// @return true if the engine should end the process right now
///
/// Called by the platform code that receives the request from the system
/// (windowShouldClose: on macOS, WM_CLOSE on Windows, the WM_DELETE_WINDOW
/// client message on X11). An application has no reason to call it.
bool OnMainWindowCloseRequested();

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

/// @brief Captures the mouse
void CaptureMouse();

/// @brief Releases the mouse
void ReleaseMouse();

/// @brief Returns true if the mouse is captured
/// @return true if the mouse is captured
bool IsMouseCaptured();

/// @brief Replaces the system clipboard contents with UTF-8 text
/// @param [in] text UTF-8 encoded text to put into the clipboard
void SetClipboardText(const std::string &text);

/// @brief Returns the system clipboard contents as UTF-8 text
/// @return UTF-8 encoded clipboard text, or an empty string if unavailable
std::string GetClipboardText();

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

/// @brief Checks if a filesystem file exists
/// @param [in] path Path to a file
/// @return kTrivalentFalse if nothing exists at the path,
/// kTrivalentTrue if a regular file exists there,
/// kTrivalentUnknown if the path leads to something that is not a regular file
/// (a directory or a device), or the application can not determine what it is.
///
/// An existing file is not necessarily a readable one, so a program that is
/// about to read the file may just as well open it and handle the failure.
Trivalent DoesFileExist(const char *path);

/// @brief Create a directory
/// @param [in] path Path to a directory to create
/// @return true on success
bool MakeDirectory(const char *path);

/// @brief Returns the current path
/// @param [out] out_dir Address of an std::string to fill with the path
/// @return true if the path is successfuly detected, false otherwise
bool GetCurrentPath(std::string *out_dir);

/// @brief Makes the directory specified current
/// @param [in] path Path to the directory to make current
/// @return true if the current directory is changed, false otherwise
///
/// The engine may have made a directory of its own choice current before
/// EasyMain is called (the Resources folder of the bundle on macOS), and assets
/// are loaded by paths relative to it, so an application that moves the current
/// directory elsewhere should either move it back or load its assets by
/// absolute paths built from GetEngine()->GetInitialPath().
bool ChangeCurrentDirectory(const char *path);

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
///
/// A relative path is resolved against the *current* directory, which is not
/// necessarily the directory the process was started from: on macOS the engine
/// makes the Resources folder of the bundle current before EasyMain is called,
/// so that assets can be loaded by their relative paths. Use
/// CanonicalizeArgvPath for a path that came from the command line.
std::string CanonicalizePath(const char *path);

/// @brief Returns the path of the file the process was loaded from
/// @return Absolute canonical path of the executable file, or an empty string if
///   the platform can not tell (the web)
///
/// This is the one path that does not depend on the current directory, on the
/// directory the user was standing in, or on what argv[0] happens to contain, so
/// it is the way for a tool to find the data shipped next to it. On macOS the
/// executable of a bundled application lives in `<name>.app/Contents/MacOS`, so
/// a tool looking for its own data folder should be ready to step out of the
/// bundle; GetEngine()->GetInitialPath() already points at the directory that
/// contains the bundle.
///
/// Example:
/// @code
/// const std::string exe = GetExecutablePath();
/// const std::string dir = exe.substr(0, exe.find_last_of('/'));
/// Sprite s;
/// s.Load(GluePath(dir.c_str(), "data/hero.tga").c_str());
/// @endcode
std::string GetExecutablePath();

/// @brief Returns the directory the process was started from
/// @return Absolute path of the startup directory, or an empty string if the
///   platform has no such notion (the web) or the startup code never ran
///   (headless use through ARCTIC_NO_MAIN)
///
/// Captured once, before anything changes the current directory, and never
/// modified afterwards, so it is safe to read from any thread.
std::string GetStartupDirectory();

/// @brief Remembers the directory the process was started from
/// @param [in] path Absolute path of the startup directory
///
/// Called by the platform startup code (PrepareInitialPath) before the current
/// directory is changed. An application normally has no reason to call this;
/// it is public so that a custom main (ARCTIC_NO_MAIN) can do what the engine's
/// own main does.
void SetStartupDirectory(const std::string &path);

/// @brief Turns a path taken from the command line into an absolute one
/// @param [in] path Path exactly as it was typed by the user, absolute or
///   relative to the directory the program was started from
/// @return The canonical absolute form of the path, or an empty string if path
///   is null or empty
///
/// The current directory of an Arctic application is not the directory the user
/// started it from: on macOS the engine makes `<bundle>/Contents/Resources`
/// current before EasyMain, so that `Sprite::Load("data/hero.tga")` works
/// wherever the bundle was copied to. That is right for assets and wrong for
/// arguments: `mygame open level.txt` would look for the level inside the
/// bundle, fail, and say nothing useful about why. Resolve every path that came
/// from argv through this function and the shell's meaning is preserved, on
/// every platform, whether or not that platform changes the directory.
///
/// Example:
/// @code
/// void EasyMain() {
///   if (GetEngine()->GetArgc() >= 2) {
///     const std::string path =
///         CanonicalizeArgvPath(GetEngine()->GetArgv()[1]);
///     std::vector<Ui8> data = ReadFile(path.c_str(), true);
///     ...
///   }
/// }
/// @endcode
std::string CanonicalizeArgvPath(const char *path);

/// @brief Describes a file path for an error message
/// @param [in] path Path of the file that could not be opened
/// @return A one line description of the path
///
/// "The file does not exist" is a useless complaint when the reader cannot tell
/// which file was looked for. The description always names the path as it was
/// given and the absolute path it resolved to, and adds what usually explains
/// the failure: the current directory a relative path was resolved against, a
/// parent directory that does not exist, and the fact that the file does exist
/// under the startup directory, which means the path came from the command line
/// and should have been read through CanonicalizeArgvPath.
///
/// ReadFile and WriteFile report failures this way already; call this directly
/// when reporting a file your own code failed to open.
std::string DescribeFilePath(const char *path);

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
