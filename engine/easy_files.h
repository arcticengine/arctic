// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
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

#ifndef ENGINE_EASY_FILES_H_
#define ENGINE_EASY_FILES_H_

#include <vector>

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_files
/// @{

/// @brief Loads all data from a file specified.
/// @param [in] file_name Name of the file to load.
/// @param [in] is_bulletproof If true, the function will not throw an exception if the file is not found.
/// @return Vector of bytes containing the file data.
std::vector<Ui8> ReadFile(const char *file_name, bool is_bulletproof = false);

/// @brief Saved the data specified to a file.
/// @param [in] file_name Name of the file to save to.
/// @param [in] data Pointer to the data to save.
/// @param [in] data_size Size of the data to save.
void WriteFile(const char *file_name, const Ui8 *data, const Ui64 data_size);

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_FILES_H_
