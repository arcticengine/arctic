// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2018 Vitaliy Manushkin
// Copyright (c) 2020 Huldra
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

#ifndef ENGINE_MTQ_BASE_COMMON_H_
#define ENGINE_MTQ_BASE_COMMON_H_

#include <atomic>

namespace arctic {

typedef unsigned long ui64;
static_assert(sizeof(ui64) == 8, "invalid ui64 definition");

typedef signed long si64;
static_assert(sizeof(si64) == 8, "invalid si64 definition");

typedef signed long i64;
static_assert(sizeof(i64) == 8, "invalid i64 definition");

typedef unsigned int ui32;
static_assert(sizeof(ui32) == 4, "invalid ui32 definition");

typedef unsigned char ui8;
typedef signed char si8;
typedef signed char i8;

static constexpr auto MO_SEQUENCE = std::memory_order_seq_cst;
static constexpr auto MO_RELAXED = std::memory_order_relaxed;
static constexpr auto MO_ACQUIRE = std::memory_order_acquire;
static constexpr auto MO_RELEASE = std::memory_order_release;
static constexpr auto MO_ACQUIRE_RELEASE = std::memory_order_acq_rel;

}

#endif  // ENGINE_MTQ_BASE_COMMON_H_
