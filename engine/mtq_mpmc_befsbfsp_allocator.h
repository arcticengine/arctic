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
// Also provides MpmcNoFallbackFixedSizeBufferFixedSizePool: same shape, but
// alloc returns nullptr when empty and free aborts if it cannot reclaim
// (used by the sound mixer page_pool so SIGIO never hits the heap).
//
// alloc and free are both wait-free if you are lucky
// alloc/free requires at most kArraySize operations if you are lucky

#ifndef ENGINE_MPMC_BEFSBFSP_ALLOCATOR_H_
#define ENGINE_MPMC_BEFSBFSP_ALLOCATOR_H_

#include "engine/mtq_mempool_allocator.h"
#include <array>
#include <atomic>
#include <cstdlib>

namespace arctic {

/// @brief Fixed size allocator for fixed size buffers
/// @tparam kArraySize Size of the array
/// @tparam kBufferSize Size of the buffer
///
/// Best-effort: falls back to ::malloc / ::free when the pool is empty or
/// when free cannot reclaim a slot (e.g. after a malloc fallback).
template<size_t kArraySize, size_t kBufferSize>
class alignas(64) MpmcBestEffortFixedSizeBufferFixedSizePool : public I_FixedSizeAllocator {
  std::array<std::atomic<void*>, kArraySize> items;
 public:
  MpmcBestEffortFixedSizeBufferFixedSizePool() {
    for (size_t i = 0; i < items.size(); ++i) {
      items[i] = ::malloc(kBufferSize);
    }
  }

  ~MpmcBestEffortFixedSizeBufferFixedSizePool() {
    for (size_t i = 0; i < items.size(); ++i) {
      void *p = std::atomic_exchange(&items[i], (void*)nullptr);
      if (p) {
        ::free(p);
      }
    }
  }

  /// @brief Allocates a buffer
  /// @return Pointer to the allocated buffer
  void *alloc() override {
    for (size_t i = 0; i < items.size(); ++i) {
      void *p = std::atomic_exchange(&items[i], (void*)nullptr);
      if (p) {
        return p;
      }
    }
    return ::malloc(kBufferSize);
  }

  void free(void *ptr) override {
    for (size_t i = 0; i < items.size(); ++i) {
      ptr = std::atomic_exchange(&items[i], ptr);
      if (!ptr) {
        return;
      }
    }
    ::free(ptr);
  }

  /// @brief Gets the size of the buffer
  /// @return Size of the buffer
  size_t getBlockSize() override {
    return kBufferSize;
  }
};


/// @brief Fixed-size pool with no heap fallback.
///
/// alloc() returns nullptr when every slot is checked out. free() must always
/// be able to return a block into a slot (we never hand out non-pool memory);
/// inability to do so is an invariant violation and aborts.
template<size_t kArraySize, size_t kBufferSize>
class alignas(64) MpmcNoFallbackFixedSizeBufferFixedSizePool : public I_FixedSizeAllocator {
  std::array<std::atomic<void*>, kArraySize> items;
 public:
  MpmcNoFallbackFixedSizeBufferFixedSizePool() {
    for (size_t i = 0; i < items.size(); ++i) {
      items[i] = ::malloc(kBufferSize);
    }
  }

  ~MpmcNoFallbackFixedSizeBufferFixedSizePool() {
    for (size_t i = 0; i < items.size(); ++i) {
      void *p = std::atomic_exchange(&items[i], (void*)nullptr);
      if (p) {
        ::free(p);
      }
    }
  }

  /// @brief Allocates a buffer from the pool, or nullptr if exhausted.
  void *alloc() override {
    for (size_t i = 0; i < items.size(); ++i) {
      void *p = std::atomic_exchange(&items[i], (void*)nullptr);
      if (p) {
        return p;
      }
    }
    return nullptr;
  }

  void free(void *ptr) override {
    if (!ptr) {
      return;
    }
    for (size_t i = 0; i < items.size(); ++i) {
      ptr = std::atomic_exchange(&items[i], ptr);
      if (!ptr) {
        return;
      }
    }
    // Never malloc'd outside the pool, so a free that cannot reclaim is fatal.
    abort();
  }

  size_t getBlockSize() override {
    return kBufferSize;
  }
};

}  // namespace arctic

#endif  // ENGINE_MPMC_BEFSBFSP_ALLOCATOR_H_
