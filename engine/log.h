// The MIT License (MIT)
//
// Copyright (c) 2018 Huldra
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

#ifndef ENGINE_LOG_H_
#define ENGINE_LOG_H_

#include <memory>
//#include <sstream>
#include <iosfwd>
#include <string>
#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_log
/// @{

/// @brief Provides a streaming interface to write log
/// @return A unique pointer to an ostringstream with a custom deleter
/// 
/// The logger is automatically started by the engine before EasyMain is called,
/// so users only need to use this function to write log messages.
/// 
/// Usage example:
/// @code
///   *Log() << "Hello World!";
///   *Log() << "Value: " << 42;
///   *Log() << "Position: " << x << ", " << y;
/// @endcode
/// 
/// @warning This header declares std::ostringstream through <iosfwd> only, so it
/// stays cheap to include. The streaming form above needs the complete type, so
/// a translation unit that uses it has to include <sstream> itself:
/// @code
///   #include <sstream>
///   #include "engine/log.h"
/// @endcode
/// Including "engine/easy.h" is enough as well, it pulls <sstream> in already.
/// Without either of them the compiler reports something like
/// "invalid operands to binary expression
/// ('std::basic_ostringstream<char>' and 'const char[13]')"
/// on the line with the << operator. The Log(const char *) overloads below need
/// no streams at all, so they are a good fit for code that wants to keep its
/// includes light.
std::unique_ptr<std::ostringstream, void(*)(std::ostringstream *str)> Log();

/// @brief Writes message text to log
/// @param text The text message to be logged
void Log(const char *text);

/// @brief Writes two message texts to log
/// @param text1 The first text message to be logged
/// @param text2 The second text message to be logged
void Log(const char *text1, const char *text2);

/// @brief Writes three message texts to log
/// @param text1 The first text message to be logged
/// @param text2 The second text message to be logged
/// @param text3 The third text message to be logged
void Log(const char *text1, const char *text2, const char *text3);

/// @brief Starts the logger
/// 
/// @note This function is called automatically by the engine before EasyMain is called.
/// Users do not need to call this function manually.
void StartLogger();

/// @brief Stops the logger
/// 
/// @note This function is called automatically by the engine when the program exits.
/// Users do not need to call this function manually.
void StopLogger();

/// @brief Returns the full path of the file the log is written to
/// @return Absolute path of the log file, or an empty string on the web, where
///   the log goes to the console instead of a file
///
/// The log file is opened by a name relative to the current directory, which the
/// engine may have changed before EasyMain (the Resources folder of the bundle
/// on macOS), so where it ends up is not obvious from the name alone. Print this
/// path from an application that asks its user to send the log in.
std::string LogFilePath();

/// @brief Limits how large the log file may grow
/// @param [in] max_bytes Size in bytes after which the log is rotated, or 0 for
///   no limit, which is the default
///
/// When the file grows past the limit, it is renamed to log_prev.txt (replacing
/// an older one) and a new, empty log is started, so at most two files are kept
/// and the newer one is always the shorter to read. Rotation happens between
/// messages, so a single message is never split across the two files. Set the
/// limit before StartLogger to have the first rotation happen at the right size
/// even for a log inherited from a previous run.
void SetLogSizeLimit(Ui64 max_bytes);

/// @brief Returns the current log size limit in bytes, 0 when unlimited
Ui64 LogSizeLimit();

/// @brief Throws away everything the log holds
///
/// The request goes through the same queue as the messages, so it is ordered
/// with them: nothing logged before the call survives, nothing logged after it
/// is lost. A run that wants a log of itself alone can call this right at the
/// start instead of reading past the previous runs.
void ClearLog();

/// @brief Writes a line telling when the run started, with what and from where
///
/// @note The engine writes this itself right after starting the logger. It is
/// public because an application that clears or rotates the log of a long
/// session may want the same line at the top of the new file.
void LogRunHeader();

/// @}

}  // namespace arctic

#endif  // ENGINE_LOG_H_
