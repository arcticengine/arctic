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
  if (state == nullptr) {
    state = getNewState(pool);
    state->back = static_cast<BlockItems *>(state);
    return;
  }
  if (getItemCount(pool, state->back, item_size) != state->back_offset) {
    return;
  }
  state->back->next = getNewBlock(pool);
  state->back = state->back->next;
  state->back_offset = 0;
}


void *FixedBlockQueue_Gears::get_front(
  I_FixedSizeAllocator *pool,
  size_t item_size) {
  if (state->front_offset < getItemCount(pool, state, item_size)) {
    return &state->items[item_size * state->front_offset];
  }

  size_t front_offset =
    state->front_offset - getItemCount(pool, state, item_size);
  return &state->next->items[front_offset * item_size];
}


void FixedBlockQueue_Gears::front_cleanup(
  I_FixedSizeAllocator *pool,
  size_t item_size) {
  if (state->next == nullptr) {
    // there is only one block in the queue
    if (state->front_offset == state->back_offset) {
      // the queue is empty, let's free everything
      pool->free(state);
      state = nullptr;
    }
    return;
  }
  if (state->front_offset < getItemCount(pool, false, item_size)) {
    return;
  }

  // let's transfer the state to the next block
  auto next_field_is_moving = state->next->next;
  auto next_state = reinterpret_cast<BlockQueueState *>(state->next);
  next_state->next = next_field_is_moving;
  next_state->front_offset =
    state->front_offset - getItemCount(pool, next_state, item_size);
  next_state->back_offset =
    state->back_offset + getItemCount(pool, true, item_size)
      - getItemCount(pool, false, item_size);
  next_state->back = state->back != state->next ?
    state->back : static_cast<BlockItems *>(next_state);

  pool->free(state);
  state = next_state;
}


size_t inline FixedBlockQueue_Gears::getItemCount(
  I_FixedSizeAllocator *pool,
  BlockItems *block,
  size_t item_size) {
  return getItemCount(
    pool, static_cast<BlockItems *>(state) == block, item_size);
}


size_t inline FixedBlockQueue_Gears::getItemCount(
  I_FixedSizeAllocator *pool,
  bool first_block,
  size_t item_size) {
  auto block_size = pool->getBlockSize();
  if (first_block) {
    return (block_size - sizeof(BlockQueueState)) / item_size;
  } else {
    return (block_size - sizeof(BlockItems)) / item_size;
  }
}


} // namespace arctic
