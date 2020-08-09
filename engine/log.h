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

#include "engine/arctic_types.h"
#include <memory>
#include <sstream>

namespace arctic {

/// @addtogroup global_log
/// @{

/// @brief Writes message text to log
void Log(const char *text);

/// @brief Writes message text to log
void Log(const char *text1, const char *text2);

/// @brief Writes message text to log
void Log(const char *text1, const char *text2, const char *text3);

/// @brief Provides a streaming interface to write log
/// Usage example:
/// @code
///   *Log() << "Hello World!";
/// @endcode
std::unique_ptr<std::ostringstream, void(*)(std::ostringstream *str)> Log();

/// @brief Starts the logger
void StartLogger();

/// @brief Stops the logger
void StopLogger();

/// @}

}  // namespace arctic

#endif  // ENGINE_LOG_H_
