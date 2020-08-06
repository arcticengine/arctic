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

#ifndef ENGINE_ARCTIC_PLATFORM_BYTEORDER_H_
#define ENGINE_ARCTIC_PLATFORM_BYTEORDER_H_

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_utility
/// @{

/// @brief Transform value from the big-endian representation 
/// @param [in] x Big-endian value to transform
/// @return Transformed value in local cpu-specific representation
Ui16 FromBe(Ui16 x);

/// @copydoc FromBe()
Si16 FromBe(Si16 x);

/// @copydoc FromBe()
Ui32 FromBe(Ui32 x);

/// @copydoc FromBe()
Si32 FromBe(Si32 x);

/// @brief Transform value to the big-endian representation 
/// @param [in] x Local cpu-specific representation of a value to transform
/// @return Transformed value in big-endian representation
Ui16 ToBe(Ui16 x);

/// @copydoc ToBe()
Si16 ToBe(Si16 x);

/// @copydoc ToBe()
Ui32 ToBe(Ui32 x);

/// @copydoc ToBe()
Si32 ToBe(Si32 x);

/// @}

}  // namespace arctic

#endif  // ENGINE_ARCTIC_PLATFORM_BYTEORDER_H_
