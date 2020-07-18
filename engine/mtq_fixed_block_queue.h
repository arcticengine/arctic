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

// This is a single-thread queue, it's an auxiliary queue.
// It works with memory-pool allocator.

#ifndef ENGINE_MTQ_FIXED_BLOCK_QUEUE_H_
#define ENGINE_MTQ_FIXED_BLOCK_QUEUE_H_

#include <cstddef>
#include <new>
#include "engine/mtq_mempool_allocator.h"

namespace arctic {

class FixedBlockQueue_Gears {
protected:
  void prepare_slot(I_FixedSizeAllocator *pool, size_t item_size);
  void *get_front(I_FixedSizeAllocator *pool, size_t item_size);
  void front_cleanup(I_FixedSizeAllocator *pool, size_t item_size);

  inline void *get_back(size_t item_size) {
    return &back->items[item_size * (back_offset - 1)];
  }

  bool empty() const {
    return front == nullptr;
  }

  struct BlockItems {
    BlockItems *next = nullptr;
    alignas(std::max_align_t) char items[0];

    BlockItems() {
    }

    BlockItems(BlockItems const & ) = delete;
    BlockItems& operator=(BlockItems const & ) = delete;
  };

  BlockItems *getNewBlock(I_FixedSizeAllocator *pool) {
    void *block = pool->alloc();
    return new(block) BlockItems;
  }

  size_t inline getItemCount(
    I_FixedSizeAllocator *pool,
    size_t item_size);

  size_t front_offset = 0;
  size_t back_offset = 0;
  BlockItems *back = nullptr;
  BlockItems *front = nullptr;
};


template<typename ElemType>
class FixedBlockQueue : protected FixedBlockQueue_Gears {
public:
  void push_back(ElemType elem, I_FixedSizeAllocator *pool) {
    prepare_slot(pool, sizeof(ElemType));
    // state is absolutely not nullptr after prepare_slot
    ++back_offset;
    new(get_back(sizeof(ElemType))) ElemType(std::move(elem));
  }


  ElemType &front(I_FixedSizeAllocator *pool) {
    // let's find out where is the front element
    return *static_cast<ElemType *>(get_front(pool, sizeof(ElemType)));
  }


  void pop_front(I_FixedSizeAllocator *pool) {
    // let's find out where is the front element
    ElemType *front = static_cast<ElemType *>(
      get_front(pool, sizeof(ElemType)));
    // let's destroy the front element and free some memory
    front->~ElemType();
    front_offset++;
    front_cleanup(pool, sizeof(ElemType));
  }

  bool empty() const {
    return FixedBlockQueue_Gears::empty();
  }
};


} // namespace arctic

#endif  // ENGINE_MTQ_FIXED_BLOCK_QUEUE_H_
