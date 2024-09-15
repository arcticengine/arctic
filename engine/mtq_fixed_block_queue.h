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
#include <utility>
#include "engine/mtq_mempool_allocator.h"

#pragma warning(push)
#pragma warning(disable : 4200)

namespace arctic {

/// @brief A class that provides the core functionality for a fixed block queue.
class FixedBlockQueue_Gears {
 protected:
  /// @brief Prepares a slot for a new item in the queue.
  /// @param pool The memory pool allocator.
  /// @param item_size The size of the item to be added.
  void prepare_slot(I_FixedSizeAllocator *pool, size_t item_size);

  /// @brief Gets a pointer to the front item in the queue.
  /// @param pool The memory pool allocator.
  /// @param item_size The size of the items in the queue.
  /// @return A pointer to the front item.
  void *get_front(I_FixedSizeAllocator *pool, size_t item_size);

  /// @brief Cleans up the front item after it has been processed.
  /// @param pool The memory pool allocator.
  /// @param item_size The size of the items in the queue.
  void front_cleanup(I_FixedSizeAllocator *pool, size_t item_size);

  /// @brief Gets a pointer to the back item in the queue.
  /// @param item_size The size of the items in the queue.
  /// @return A pointer to the back item.
  inline void *get_back(size_t item_size) {
    return &back->items[item_size * (back_offset - 1)];
  }

  /// @brief Checks if the queue is empty.
  /// @return True if the queue is empty, false otherwise.
  bool empty() const {
    return front == nullptr;
  }

  /// @brief A structure representing a block of items in the queue.
  struct BlockItems {
    BlockItems *next = nullptr;
    alignas(std::max_align_t) char items[0];

    BlockItems() {
    }

    BlockItems(BlockItems const & ) = delete;
    BlockItems& operator=(BlockItems const & ) = delete;
  };

  /// @brief Allocates a new block of items.
  /// @param pool The memory pool allocator.
  /// @return A pointer to the newly allocated block.
  BlockItems *getNewBlock(I_FixedSizeAllocator *pool) {
    void *block = pool->alloc();
    return new(block) BlockItems;
  }

  /// @brief Gets the number of items in the queue.
  /// @param pool The memory pool allocator.
  /// @param item_size The size of the items in the queue.
  /// @return The number of items in the queue.
  size_t inline getItemCount(
    I_FixedSizeAllocator *pool,
    size_t item_size);

  size_t front_offset = 0;  ///< Offset of the front item in the front block.
  size_t back_offset = 0;   ///< Offset of the back item in the back block.
  BlockItems *back = nullptr;  ///< Pointer to the back block of items.
  BlockItems *front = nullptr; ///< Pointer to the front block of items.
};


/// @brief A template class for a fixed block queue.
/// @tparam ElemType The type of elements stored in the queue.
template<typename ElemType>
class FixedBlockQueue : protected FixedBlockQueue_Gears {
 public:
  /// @brief Pushes an element to the back of the queue.
  /// @param elem The element to push.
  /// @param pool The memory pool allocator.
  void push_back(ElemType elem, I_FixedSizeAllocator *pool) {
    prepare_slot(pool, sizeof(ElemType));
    // state is absolutely not nullptr after prepare_slot
    ++back_offset;
    new(get_back(sizeof(ElemType))) ElemType(std::move(elem));
  }


  /// @brief Gets a reference to the front element of the queue.
  /// @param pool The memory pool allocator.
  /// @return A reference to the front element.
  ElemType &front(I_FixedSizeAllocator *pool) {
    // let's find out where is the front element
    return *static_cast<ElemType *>(get_front(pool, sizeof(ElemType)));
  }


  /// @brief Removes the front element from the queue.
  /// @param pool The memory pool allocator.
  void pop_front(I_FixedSizeAllocator *pool) {
    // let's find out where is the front element
    ElemType *front = static_cast<ElemType *>(
      get_front(pool, sizeof(ElemType)));
    // let's destroy the front element and free some memory
    front->~ElemType();
    front_offset++;
    front_cleanup(pool, sizeof(ElemType));
  }

  /// @brief Checks if the queue is empty.
  /// @return True if the queue is empty, false otherwise.
  bool empty() const {
    return FixedBlockQueue_Gears::empty();
  }
};


}  // namespace arctic

#pragma warning(pop)

#endif  // ENGINE_MTQ_FIXED_BLOCK_QUEUE_H_
