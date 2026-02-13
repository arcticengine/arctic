// The MIT License (MIT)
//
// Copyright (c) 2016 - 2019 Huldra
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

#ifndef ENGINE_ARCTIC_PLATFORM_FATAL_H_
#define ENGINE_ARCTIC_PLATFORM_FATAL_H_

namespace arctic {

/// @addtogroup global_utility
/// @{

/// @brief Exits the application with an error if condition is false
/// @param condition Condition to check
/// @param [in] error_message Error message to display upon exiting
/// @param [in] error_message_postfix A postfix to append to the error message.
/// Nothing is appended if error_message_postfix is nullptr
/// @details The function does not return if the condition is false
void Check(bool condition, const char *error_message,
    const char *error_message_postfix = nullptr);

/// @brief Exits the application with an error
/// @param [in] error_message Error message to display upon exiting
/// @param [in] message_postfix A postfix to append to the error message.
/// Nothing is appended if error_message_postfix is nullptr
/// @details The function does not return
[[noreturn]] void Fatal(const char *error_message, const char *message_postfix = nullptr);

/// @}

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_FATAL_H_
