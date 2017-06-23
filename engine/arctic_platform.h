// The MIT License(MIT)
//
// Copyright 2016 - 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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
#include "engine/arctic_types.h"

namespace arctic {

void Check(bool condition, const char *error_message,
    const char *error_message_postfix = nullptr);
void Fatal(const char *error_message, const char *message_postfix = nullptr);
void Draw();
void Swap();
bool IsVSyncSupported();
bool SetVSync(bool is_enable);
void ProcessUserInput();
std::vector<Ui8> ReadWholeFile(const char *file_name);
void WriteWholeFile(const char *file_name, const Ui8 *data,
    const Ui64 data_size);
void SleepSeconds(double duration);

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_H_
