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

//
// This is a single producer multiple consumers bounded queue.
// It is implemented by array of elemenets. It uses fetch_add atomic
// operatation. It is wait-free on x86 and amd64 platforms. But lock-free
// on all other platforms, including arm, aarch64, PowerPC, MIPS, MIPS64, etc.
//

#ifndef ENGINE_MTQ_SPMC_ARRAY_H_
#define ENGINE_MTQ_SPMC_ARRAY_H_

#include <atomic>
#include <memory>

#include "engine/mtq_base_common.h"

namespace arctic {

using std::atomic;

class SPMC_ArrayImpl {
public:
  SPMC_ArrayImpl(ui64 size);

  bool enqueue(void *elem);
  void *dequeue();

protected:
  struct Slot {
    atomic<void *> item;
    atomic<ui64> seq;
  };

  ui64 const numberOfSlots;
  std::unique_ptr<Slot[]> const array;
  atomic<ui64> head;
  atomic<ui64> headSeq;
  atomic<ui64> dequeueBarrier;
  atomic<ui64> dequeueCounter;
  ui64 tail;
  ui64 tailSeq;
  atomic<ui64> enqueueCounter;
};


template<typename Payload, bool Delete = true>
class SpmcArray {
public:
  SpmcArray(ui64 size)
    : impl(size) {}

  ~SpmcArray() {
    if (Delete) {
      while (auto item = dequeue()) {
        delete item;
      }
    }
  }

  bool enqueue(Payload *item) {
    return impl.enqueue(reinterpret_cast<void *>(item));
  }

  Payload *dequeue() {
    return reinterpret_cast<Payload *>(impl.dequeue());
  }

protected:
  SPMC_ArrayImpl impl;
};


template<bool Delete>
class SpmcArray<void, Delete> {
public:
  SpmcArray(ui64 size)
    : impl(size) {}

  ~SpmcArray() {
    if (Delete) {
      while (auto item = dequeue()) {
        free(item);
      }
    }
  }

  bool enqueue(void *item) {
    return impl.enqueue(item);
  }

  void *dequeue() {
    return impl.dequeue();
  }

protected:
  SPMC_ArrayImpl impl;
};

}  // namespace arctic

#endif  // ENGINE_MTQ_SPMC_ARRAY_H_
