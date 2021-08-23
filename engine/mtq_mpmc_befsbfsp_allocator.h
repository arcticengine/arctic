// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2018 Vitaliy Manushkin
// Copyright (c) 2020 Huldra
// Copyright (c) 2021 The Lasting Curator
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

// Multiple-producer - multiple-consumer
// Best-effort (falls back to malloc/free when preemption/overflow occurs)
// Fixed size buffer allocator implemented as a
// Fixed size pool
//
// alloc and free are both wait-free if you are lucky
// alloc/free requires at most kArraySize operations if you are lucky

#ifndef ENGINE_MPMC_BEFSBFSP_ALLOCATOR_H_
#define ENGINE_MPMC_BEFSBFSP_ALLOCATOR_H_

#include "engine/mtq_mempool_allocator.h"
#include <atomic>

namespace arctic {

template<size_t kArraySize, size_t kBufferSize>
class alignas(64) MpmcBestEffortFixedSizeBufferFixedSizePool : public I_FixedSizeAllocator {
  std::array<std::atomic<void*>, kArraySize> items;
 public:
  MpmcBestEffortFixedSizeBufferFixedSizePool() {
    for (size_t i = 0; i < items.size(); ++i) {
      items[i] = malloc(kBufferSize);
    }
  }

  ~MpmcBestEffortFixedSizeBufferFixedSizePool() {
    for (size_t i = 0; i < items.size(); ++i) {
      void *p = std::atomic_exchange(&items[i], (void*)nullptr);
      if (p) {
        free(p);
      }
    }
  }

  void *alloc() override {
    for (size_t i = 0; i < items.size(); ++i) {
      void *p = std::atomic_exchange(&items[i], (void*)nullptr);
      if (p) {
        return p;
      }
    }
    return malloc(kBufferSize);
  }

  void free(void *ptr) override {
    for (size_t i = 0; i < items.size(); ++i) {
      ptr = std::atomic_exchange(&items[i], ptr);
      if (!ptr) {
        return;
      }
    }
    free(ptr);
    return;
  }

  size_t getBlockSize() override {
    return kBufferSize;
  }
};

}  // namespace arctic

#endif  // ENGINE_MPMC_BEFSBFSP_ALLOCATOR_H_
