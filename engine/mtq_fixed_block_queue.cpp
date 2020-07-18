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

#include "engine/mtq_fixed_block_queue.h"

namespace arctic {

void FixedBlockQueue_Gears::prepare_slot(
  I_FixedSizeAllocator *pool,
  size_t item_size) {
  if (front == nullptr) {
    front = getNewBlock(pool);
    back = front;
    front_offset = 0;
    back_offset = 0;
    return;
  }
  if (getItemCount(pool, item_size) != back_offset) {
    return;
  }
  back->next = getNewBlock(pool);
  back = back->next;
  back_offset = 0;
}


void *FixedBlockQueue_Gears::get_front(
  I_FixedSizeAllocator *pool,
  size_t item_size) {
  if (front_offset < getItemCount(pool, item_size)) {
    return &front->items[item_size * front_offset];
  }

  size_t front_idx = front_offset - getItemCount(pool, item_size);
  return &front->next->items[front_idx * item_size];
}


void FixedBlockQueue_Gears::front_cleanup(
  I_FixedSizeAllocator *pool,
  size_t item_size) {
  if (front->next == nullptr) {
    // there is only one block in the queue
    if (front_offset == back_offset) {
      // the queue is empty, let's free everything
      pool->free(front);
      front = nullptr;
    }
    return;
  }
  if (front_offset < getItemCount(pool, item_size)) {
    return;
  }

  // let's transfer the state to the next block
  front_offset -= getItemCount(pool, item_size);
  back_offset += getItemCount(pool, item_size) - getItemCount(pool, item_size);

  BlockItems *next_block = front->next;
  pool->free(front);
  front = next_block;
}

size_t inline FixedBlockQueue_Gears::getItemCount(
  I_FixedSizeAllocator *pool,
  size_t item_size) {
  auto block_size = pool->getBlockSize();
  return (block_size - sizeof(BlockItems)) / item_size;
}


} // namespace arctic
