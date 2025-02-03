// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2019 Huldra
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

#include "engine/arctic_platform_def.h"

#if defined(ARCTIC_PLATFORM_PI) || defined(ARCTIC_PLATFORM_WEB) 

#include <arpa/inet.h>

namespace arctic {

Ui16 FromBe(Ui16 x) {
  return ntohs(x);
}
Si16 FromBe(Si16 x) {
  return ntohs(x);
}
Ui32 FromBe(Ui32 x) {
  return ntohl(x);
}
Si32 FromBe(Si32 x) {
  return ntohl(x);
}
Ui16 ToBe(Ui16 x) {
  return htons(x);
}
Si16 ToBe(Si16 x) {
  return htons(x);
}
Ui32 ToBe(Ui32 x) {
  return htonl(x);
}
Si32 ToBe(Si32 x) {
  return htonl(x);
}


}  // namespace arctic

#endif  // defined(ARCTIC_PLATFORM_PI) || defined(ARCTIC_PLATFORM_WEB) 
