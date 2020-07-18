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

#include "engine/mtq_spmc_array.h"
#include <cstring>

namespace arctic {

SPMC_ArrayImpl::SPMC_ArrayImpl(Ui64 const size)
  : numberOfSlots(size)
  , array(new Slot[size])
  , head(0)
  , headSeq(0)
  , dequeueBarrier(0)
  , dequeueCounter(0)
  , tail(0)
  , tailSeq(0)
  , enqueueCounter(0) {
  memset(&array.get()[0], 0, sizeof(Slot) * size);
}


bool SPMC_ArrayImpl::enqueue(void *item) {
  /* it's only a single consumer that's why the order does not metter here */
  auto total_enqueued = enqueueCounter.load(MO_RELAXED);
  auto total_dequeued = dequeueCounter.load(MO_RELAXED);
  if (total_enqueued - total_dequeued == numberOfSlots) {
    return false;
  }

  for (;;) {
    void *expect_item = nullptr;
    auto success = array[tail].item.compare_exchange_strong(
      expect_item, item, MO_RELAXED, MO_RELAXED);
    if (success) {
      break;
    }
    if (++tail == numberOfSlots) {
      tail = 0;
    }
  }
  array[tail].seq.store(++tailSeq, MO_RELAXED);
  if (++tail == numberOfSlots) {
    tail = 0;
  }
  enqueueCounter.store(total_enqueued + 1, MO_RELEASE);
  return true;
}


void *SPMC_ArrayImpl::dequeue() {
  auto total_enqueue = enqueueCounter.load(MO_ACQUIRE);
  auto barrier = dequeueBarrier.fetch_add(1, MO_RELAXED) + 1;
  if (barrier > total_enqueue) {
    dequeueBarrier.fetch_sub(1, MO_RELAXED);
    return nullptr;
  }

  // "acquire" is here for better chances to find required cell quickly
  Ui64 current_head = head.load(MO_ACQUIRE);
  Ui64 const my_seq = headSeq.fetch_add(1, MO_RELAXED) + 1;
  for (;;) {
    Ui64 head_seq = array[current_head].seq.load(MO_RELAXED);
    if (head_seq == my_seq) {
      break;
    }
    Ui64 old_head = current_head;
    if (++current_head == numberOfSlots) {
      current_head = 0;
    }
    head.compare_exchange_strong(old_head, current_head, MO_RELAXED);
  }

  Ui64 const next_head =
    (current_head + 1 == numberOfSlots) ? 0 : current_head + 1;
  Ui64 expect_head = current_head;
  head.compare_exchange_strong(expect_head, next_head, MO_RELAXED);

  auto item = array[current_head].item.load(MO_RELAXED);
  // atomic operations for the same variable is always ordered
  array[current_head].item.store(nullptr, MO_RELAXED);
  // a slot should be cleared before increasing dequeueCounter
  dequeueCounter.fetch_add(1, MO_RELEASE);
  return item;
}


} // namespace arctic
