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
// The best way to understand how to use the queue is look through
// unit tests in mpsc_ut.cc
//
// This is a multiple producers single consumer queue. The queue is wait-free
// on x86 and amd64 platforms (actually if xadd instruction is wait-free).
//
// fetch_add seems to be not wait-free on aarch64, arm, mips, mips64
// and PowerPC playforms. That's why the queue is not wait-free
// (but lock-free) on these platforms.
//
// The queue uses virtual infinite array represented by double-linked list
// of chunks, each chunk contains several slots for payload data. Payload data
// must be a pointer to an address in memory. nullptr is a reserved value and
// must not be enqueued.
//
// Consumer never blocks and receives nullptr in the result
// if no more items were available.
//
// The queue uses original reclaimation technic to free unused chunks based
// on a counter of enqueued items. Each time a producer advances the tail
// to a next chunk it reads "tailCounter" (a total number of slots allocated
// by producers) and save it into special field "ReleaseCounter" in the next
// chunk. When a consumer successfully read all "ReleaseCounter" items,
// all previous chunks may be freed.
//

#ifndef ENGINE_MTQ_MPSC_VINFARR_H_
#define ENGINE_MTQ_MPSC_VINFARR_H_

#include <new>
#include <cstring>
#include <deque>
#include <memory.h>
#include <cstdint>

#include "engine/mtq_base_common.h"
#include "engine/mtq_fixed_block_queue.h"
#include "engine/template_tune.h"

namespace arctic {

using std::atomic;
using std::deque;

namespace dtl {

template<bool PAYLOAD_IS_PTR>
struct InfArrayChunk;

template<>
struct InfArrayChunk<true> final {
  typedef InfArrayChunk<true> SelfType;

  // link to the previous chunk of the double-linked list
  SelfType *Prev;

  // link to the next chunk of the double-linked list
  atomic<SelfType *> Next{nullptr};

  // id of the first slot of the chunk
  Ui64 const StartSlot;

  // If a consumer received all items with ids up to ReleaseCounter
  // then it is safe to reclaim all the previous chunks in
  // the double-linked list
  atomic<Ui64> ReleaseCounter{0};

  // let's allocate the chunk, calculating enough memory for payload items
  static SelfType *
  allocateNew(SelfType *prev, Ui64 start_slot, Ui32 size) {
    void *memblock =
      ::operator new(sizeof(SelfType) + sizeof(slot[0]) * size);
    return new(memblock) SelfType(prev, start_slot, size);
  }

  static SelfType *reset(
    SelfType *self,
    SelfType *prev,
    Ui64 start_slot,
    Ui32 size) {
    self->~SelfType();
    return new(self) SelfType(prev, start_slot, size);
  }

  static constexpr size_t getChunkSize(size_t chunk_elements_count) {
    return sizeof(SelfType) + sizeof(slot[0]) * chunk_elements_count;
  }


  // least_busy_counter is a lowest slot id of all active producers
  template<typename Releaser>
  bool releaseBefore(Releaser *releaser, Ui64 least_busy_counter) {
    if (!Prev) {
      return true;
    }
    // let's check if it was safe to reclaim all previous chunks
    if (least_busy_counter < ReleaseCounter) {
      return false;
    }
    // Here no active producer has a link to any of the previous chunks.
    // All the previous chunks may be reclaimed.
    // Reclaiming all chunks at once will effectively deprive wait-free
    // bounded guarantee for a consumer. That is why only one chunk is
    // released per dequeue call.
    auto i = Prev;
    Prev = i->Prev;
    releaser->freeChunk(i);
    return !Prev;
  }

  void set_slot(Ui64 slot_num, void *value) {
    Ui32 lslot = slot_num - StartSlot;
    slot[lslot].store(value, MO_RELEASE);
  }

  void *get_slot(Ui32 chunk_slot) {
    return slot[chunk_slot].load(MO_RELAXED);
  }

private:
  InfArrayChunk(
    SelfType *prev,
    Ui64 start_slot,
    Ui32 size)
    : Prev(prev)
    , StartSlot(start_slot) {
    std::memset(&slot, 0, sizeof(slot[0]) * size);
  }

  InfArrayChunk(InfArrayChunk const & );            // undefined
  InfArrayChunk& operator=(InfArrayChunk const & ); // undefined


  // payload data
  atomic<void *> slot[0];
};


template<typename INT_TYPE>
static constexpr INT_TYPE align_up(INT_TYPE value, INT_TYPE alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}


template<>
struct InfArrayChunk<false> final {
  typedef InfArrayChunk<false> SelfType;

  // link to the previous chunk of the double-linked list
  SelfType *Prev;

  // link to the next chunk of the double-linked list
  atomic<SelfType *> Next{nullptr};

  // id of the first slot of the chunk
  Ui64 const StartSlot;

  // If a consumer received all items with ids up to ReleaseCounter
  // then it is safe to reclaim all the previous chunks in
  // the double-linked list
  atomic<Ui64> ReleaseCounter{0};

  // let's allocate the chunk, calculating enough memory for payload items
  static SelfType *allocateNew(
    SelfType *prev,
    Ui64 start_slot,
    Ui32 number_of_slots,
    Ui32 payload_size) {
    void *memblock = ::operator new(
      sizeof(SelfType) + getTotalPackSize(number_of_slots, payload_size));
    return new(memblock)
      SelfType(prev, start_slot, number_of_slots, payload_size);
  }

  static SelfType *reset(
    SelfType *self,
    SelfType *prev,
    Ui64 start_slot,
    Ui32 number_of_slots,
    Ui32 payload_size) {
    self->~SelfType();
    return new(self)
      SelfType(prev, start_slot, number_of_slots, payload_size);
  }

  static constexpr size_t
  getChunkSize(Ui32 number_of_slots, Ui32 payload_size) {
    return sizeof(SelfType) +
      getTotalPackSize(number_of_slots, payload_size);
  }


  // least_busy_counter is a lowest slot id of all active producers
  template<typename Releaser>
  bool releaseBefore(Releaser *releaser, Ui64 least_busy_counter) {
    if (!Prev) {
      return true;
    }
    // let's check if it was safe to reclaim all previous chunks
    if (least_busy_counter < ReleaseCounter) {
      return false;
    }
    // Here no active producer has a link to any of the previous chunks.
    // All the previous chunks may be reclaimed.
    // Reclaiming all chunks at once will effectively deprive wait-free
    // bounded guarantee for a consumer. That is why only one chunk is
    // released per dequeue call.
    auto i = Prev;
    Prev = i->Prev;
    releaser->freeChunk(i);
    return !Prev;
  }

  void set_slot(Ui64 slot_num, void *value, Ui32 payload_size) {
    Ui32 lslot = slot_num - StartSlot;
    Ui32 pack_num = lslot / NUMBER_OF_SLOTS_IN_PACK;
    Ui32 slot_in_pack = lslot % NUMBER_OF_SLOTS_IN_PACK;
    SlotPack &pack = getPack(pack_num, payload_size);
    memcpy(&pack.Payload[payload_size * slot_in_pack],
      value,
      payload_size);
    pack.ReadyFlags.fetch_or(1ull << slot_in_pack, MO_RELEASE);
  }

  bool get_slot(Ui32 chunk_slot, void *value, Ui32 payload_size) {
    Ui32 pack_num = chunk_slot / NUMBER_OF_SLOTS_IN_PACK;
    Ui32 slot_in_pack = chunk_slot % NUMBER_OF_SLOTS_IN_PACK;
    SlotPack &pack = getPack(pack_num, payload_size);
    auto ready_flags = pack.ReadyFlags.load(MO_RELAXED);
    if (!(ready_flags & (1ull << slot_in_pack))) {
      return false;
    }
    std::atomic_thread_fence(MO_ACQUIRE);
    memcpy(value,
      &pack.Payload[payload_size * slot_in_pack],
      payload_size);
    return true;
  }

  static constexpr Ui32
  getNumberOfSlotsForSize(Ui32 size, Ui32 payload_size) {
    Ui32 pack_size = getPackPayloadSize(payload_size) + sizeof(SlotPack);
    Ui32 total_packs = size / pack_size;
    Ui32 pack_tail = size % pack_size;
    if (pack_tail < sizeof(SlotPack) + payload_size) {
      return 0;
    }
    Ui32 tail_number_of_slots =
      (pack_tail - sizeof(SlotPack)) / payload_size;
    return total_packs * NUMBER_OF_SLOTS_IN_PACK + tail_number_of_slots;
  }

private:
  InfArrayChunk(
    SelfType *prev,
    Ui64 start_slot,
    Ui32 number_of_slots,
    Ui32 payload_size)
    : Prev(prev)
    , StartSlot(start_slot) {
    auto total_pack_size = getTotalPackSize(number_of_slots, payload_size);
    std::memset(&slot_pack, 0, total_pack_size);
  }

  static constexpr Ui32 NUMBER_OF_SLOTS_IN_PACK =
    sizeof(std::uintmax_t) * 8;

  static constexpr Ui32 getPackPayloadSize(Ui32 payload_size) {
    return payload_size * NUMBER_OF_SLOTS_IN_PACK;
  }

  struct SlotPack {
    atomic<std::uintmax_t> ReadyFlags;
    char Payload[0];

    SlotPack(SlotPack const & );            // undefined
    SlotPack& operator=(SlotPack const & ); // undefined
  };

  static constexpr size_t
  getTotalPackSize(Ui32 number_of_slots, Ui32 payload_size) {
    Ui32 total_payload_size = payload_size * number_of_slots;
    Ui32 number_of_packs =
      align_up(number_of_slots, NUMBER_OF_SLOTS_IN_PACK);
    return total_payload_size
      + number_of_packs * sizeof(SlotPack::ReadyFlags);
  }

  SlotPack &getPack(Ui32 pack_num, Ui32 payload_size) {
    size_t pack_size = getPackPayloadSize(payload_size) + sizeof(SlotPack);
    return *(SlotPack *) (&slot_pack[0] + pack_size * pack_num);
  }

  // payload data
  char slot_pack[0];
};


/*
 * AuxiliaryChunkSize encases logic for allocating new chunks.
 * AuxiliaryChunkSize uses malloc if ForMemoryPool was false.
 * AuxiliaryChunkSize uses memory pool with I_FixedSizeAllocator interface
 * if ForMemoryPool was true.
 * In both cases AuxiliaryChunkSize constructs objects of struct InfArrayChunk.
 */
template<bool ForMemoryPool, bool PtrPayload>
class AuxiliaryChunkSize;


template<>
class AuxiliaryChunkSize<false, true> {
protected:
  typedef InfArrayChunk<true> ChunkType;

  AuxiliaryChunkSize(Ui32 number_of_slots_in_chunk)
    : numberOfSlotsInChunk(number_of_slots_in_chunk) {}

  inline ChunkType *
  allocateChunk(ChunkType *current, Ui64 start_slot) {
    return ChunkType::allocateNew(
      current, start_slot, numberOfSlotsInChunk);
  }

  inline ChunkType *
  resetChunk(ChunkType *chunk,
    ChunkType *prev,
    Ui64 start_slot) {
    return ChunkType::reset(
      chunk, prev, start_slot, numberOfSlotsInChunk);
  }

  inline void freeChunk(ChunkType *chunk) {
    delete chunk;
  }

  void setSlot(ChunkType *chunk, Ui64 slot, void *item) {
    chunk->set_slot(slot, item);
  }

  bool getSlot(void *result, ChunkType *chunk, Ui32 chunk_slot) {
    void *item = chunk->get_slot(chunk_slot);
    if (!item) {
      return false;
    }
    *reinterpret_cast<void **>(result) = item;
    return true;
  }

  Ui32 const numberOfSlotsInChunk;
};


template<>
class AuxiliaryChunkSize<true, true> {
protected:
  typedef InfArrayChunk<true> ChunkType;

  AuxiliaryChunkSize(I_FixedSizeAllocator *pool_)
    : numberOfSlotsInChunk(calculateNumberOfSlots(pool_))
    , pool(pool_) {}

  inline ChunkType *
  allocateChunk(ChunkType *current, Ui64 start_slot) {
    ChunkType *new_chunk =
      reinterpret_cast<ChunkType *>(pool->alloc());
    return ChunkType::reset(
      new_chunk, current, start_slot, numberOfSlotsInChunk);
  }

  inline ChunkType *
  resetChunk(ChunkType *chunk,
    ChunkType *prev,
    Ui64 start_slot) {
    return ChunkType::reset(
      chunk, prev, start_slot, numberOfSlotsInChunk);
  }

  inline void freeChunk(ChunkType *chunk) {
    pool->free(chunk);
  }

  void setSlot(ChunkType *chunk, Ui64 slot, void *item) {
    chunk->set_slot(slot, item);
  }

  bool getSlot(void *result, ChunkType *chunk, Ui32 chunk_slot) {
    void *item = chunk->get_slot(chunk_slot);
    if (!item) {
      return false;
    }
    *reinterpret_cast<void **>(result) = item;
    return true;
  }

  Ui32 const numberOfSlotsInChunk;
  I_FixedSizeAllocator *const pool;

  static Ui32 calculateNumberOfSlots(I_FixedSizeAllocator *pool) {
    Ui32 block_size = pool->getBlockSize();
    if (block_size < sizeof(ChunkType) + sizeof(void *)) {
      return 0;
    }
    return (block_size - sizeof(ChunkType)) / sizeof(void *);
  }
};


template<>
class AuxiliaryChunkSize<false, false> {
protected:
  typedef InfArrayChunk<false> ChunkType;

  AuxiliaryChunkSize(Ui32 payload_size, Ui32 number_of_slots_in_chunk)
    : numberOfSlotsInChunk(number_of_slots_in_chunk)
    , payloadSize(payload_size) {}

  inline ChunkType *
  allocateChunk(ChunkType *current, Ui64 start_slot) {
    return ChunkType::allocateNew(
      current, start_slot, numberOfSlotsInChunk, payloadSize);
  }

  inline ChunkType *
  resetChunk(ChunkType *chunk, ChunkType *prev, Ui64 start_slot) {
    return ChunkType::reset(
      chunk, prev, start_slot, numberOfSlotsInChunk, payloadSize);
  }

  inline void freeChunk(InfArrayChunk<false> *chunk) {
    delete chunk;
  }

  void setSlot(ChunkType *chunk, Ui64 slot, void *item) {
    chunk->set_slot(slot, item, payloadSize);
  }

  bool getSlot(void *result, ChunkType *chunk, Ui32 chunk_slot) {
    return chunk->get_slot(chunk_slot, result, payloadSize);
  }

  Ui32 const numberOfSlotsInChunk;
  Ui32 const payloadSize;
};


template<>
class AuxiliaryChunkSize<true, false> {
protected:
  typedef InfArrayChunk<false> ChunkType;

  AuxiliaryChunkSize(Ui32 payload_size, I_FixedSizeAllocator *pool_)
    : pool(pool_)
    , payloadSize(payload_size)
    , numberOfSlotsInChunk(calculateNumberOfSlots()) {}

  inline ChunkType *
  allocateChunk(ChunkType *prev, Ui64 start_slot) {
    ChunkType *new_chunk = reinterpret_cast<ChunkType *>(pool->alloc());
    return ChunkType::reset(
      new_chunk, prev, start_slot, numberOfSlotsInChunk, payloadSize);
  }

  inline ChunkType *
  resetChunk(ChunkType *chunk, ChunkType *prev, Ui64 start_slot) {
    return ChunkType::reset(
      chunk, prev, start_slot, numberOfSlotsInChunk, payloadSize);
  }

  inline void freeChunk(ChunkType *chunk) {
    pool->free(chunk);
  }

  void setSlot(ChunkType *chunk, Ui64 slot, void *item) {
    chunk->set_slot(slot, item, payloadSize);
  }

  bool getSlot(void *result, ChunkType *chunk, Ui32 chunk_slot) {
    return chunk->get_slot(chunk_slot, result, payloadSize);
  }

  I_FixedSizeAllocator *const pool;
  Ui32 const payloadSize;
  Ui32 const numberOfSlotsInChunk;

  Ui32 calculateNumberOfSlots() {
    Ui32 block_size = pool->getBlockSize();
    return ChunkType::getNumberOfSlotsForSize(block_size, payloadSize);
  }
};


/*
 * SimpleQueueSelector is a selector for a container class for ids
 * of skipped slots.
 * Consumer skips a slot if the slot had no set value yet.
 */
template<bool ForMemoryPool, typename ElemType>
struct SimpleQueueSelector {
  typedef std::deque<ElemType> type;
};


template<typename ElemType>
struct SimpleQueueSelector<true, ElemType> {
  typedef FixedBlockQueue<ElemType> type;
};


class NoCopyable {
public:
  NoCopyable(const NoCopyable &) = delete;
  NoCopyable &operator=(const NoCopyable &) = delete;

  NoCopyable() noexcept = default;
  NoCopyable(NoCopyable &&) noexcept = default;
  NoCopyable &operator=(NoCopyable &&) noexcept = default;
};


template<bool ForMemoryPool, bool PtrPayload>
class MPSC_VirtInfArray_Impl;


template<bool ForMemoryPool>
struct ContainerOpsSelector {
  template<bool FOR_MEMORY_POOL,
    bool PTR_PAYLOAD,
    typename ContainerType,
    typename ParamType>
  static void push_back(
    MPSC_VirtInfArray_Impl<FOR_MEMORY_POOL, PTR_PAYLOAD> *,
    ContainerType &container,
    ParamType &&item) {
    container.push_back(std::forward<ParamType>(item));
  }

  template<bool FOR_MEMORY_POOL, bool PTR_PAYLOAD, typename ContainerType>
  static void pop_front(
    MPSC_VirtInfArray_Impl<FOR_MEMORY_POOL, PTR_PAYLOAD> *,
    ContainerType &container) {
    container.pop_front();
  }

  template<bool FOR_MEMORY_POOL, bool PTR_PAYLOAD, typename ContainerType>
  static auto &front(
    MPSC_VirtInfArray_Impl<FOR_MEMORY_POOL, PTR_PAYLOAD> *,
    ContainerType &container) {
    return container.front();
  }
};


template<>
struct ContainerOpsSelector<true> {
  template<bool FOR_MEMORY_POOL,
    bool PTR_PAYLOAD,
    typename ContainerType,
    typename ParamType>
  static void push_back(
    MPSC_VirtInfArray_Impl<FOR_MEMORY_POOL, PTR_PAYLOAD> *queue,
    ContainerType &container,
    ParamType &&item) {
    container.push_back(std::forward<ParamType>(item), queue->pool);
  }

  template<bool FOR_MEMORY_POOL, bool PTR_PAYLOAD, typename ContainerType>
  static void pop_front(
    MPSC_VirtInfArray_Impl<FOR_MEMORY_POOL, PTR_PAYLOAD> *queue,
    ContainerType &container) {
    container.pop_front(queue->pool);
  }

  template<bool FOR_MEMORY_POOL, bool PTR_PAYLOAD, typename ContainerType>
  static auto &front(
    MPSC_VirtInfArray_Impl<FOR_MEMORY_POOL, PTR_PAYLOAD> *queue,
    ContainerType &container) {
    return container.front(queue->pool);
  }
};


/* MPSC_VirtInfArray_Impl does not know anything about payload type,
 * MPSC_VirtInfArray_Impl assumes that payload is a pointer to some type.
 * nullptr is not allowed to enqueue.
 */
template<bool ForMemoryPool, bool PtrPayload>
class MPSC_VirtInfArray_Impl
  : public AuxiliaryChunkSize<ForMemoryPool, PtrPayload>
    , public NoCopyable {
public:
  template<typename...Params>
  MPSC_VirtInfArray_Impl(Params &&...params)
    : AuxiliaryChunkSize<ForMemoryPool, PtrPayload>(
    std::forward<Params>(params)...) {}

  bool isOK() noexcept {
    return numberOfSlotsInChunk;
  }

  ~MPSC_VirtInfArray_Impl() {
    /* The queue must be empty here.
     * head points to the last chunk of the queue */
    for (auto i = head; i != nullptr;) {
      auto prev = i->Prev;
      freeChunk(i);
      i = prev;
    }
  }

  void enqueue(void *item);
  bool dequeue(void *result);

protected:
  template<bool>
  friend
  struct InfArrayChunk;

  typedef InfArrayChunk<PtrPayload> ChunkType;

  void checkForRelease();

  atomic<Ui64> tailCounter{0};
  atomic<ChunkType *> tail{allocateChunk(nullptr, 0)};

  struct SlotChunkPair {
    Ui32 slot;
    ChunkType *chunk;
  };


  using AuxiliaryChunkSize<ForMemoryPool, PtrPayload>::numberOfSlotsInChunk;
  using AuxiliaryChunkSize<ForMemoryPool, PtrPayload>::freeChunk;
  using AuxiliaryChunkSize<ForMemoryPool, PtrPayload>::allocateChunk;
  using AuxiliaryChunkSize<ForMemoryPool, PtrPayload>::resetChunk;
  using AuxiliaryChunkSize<ForMemoryPool, PtrPayload>::setSlot;
  using AuxiliaryChunkSize<ForMemoryPool, PtrPayload>::getSlot;

  template<bool>
  friend
  struct ContainerOpsSelector;

  typedef
  typename SimpleQueueSelector<ForMemoryPool, SlotChunkPair>::type
    SkipContainer;

  void pushBackSkipped(SlotChunkPair pair) {
    ContainerOpsSelector<ForMemoryPool>::
    template push_back<ForMemoryPool>(
      this, skippedSlots, std::move(pair));
  }

  void popFrontRetry() {
    ContainerOpsSelector<ForMemoryPool>::
    template pop_front<ForMemoryPool>(this, retrySlots);
  }

  SlotChunkPair &frontRetry() {
    return ContainerOpsSelector<ForMemoryPool>::
    template front<ForMemoryPool>(this, retrySlots);
  }

  Ui64 headCounter{0};
  Ui64 lastKnownTailCounter{0};
  ChunkType *head = tail.load(std::memory_order_relaxed);

  SkipContainer skippedSlots;
  SkipContainer retrySlots;

  ChunkType *releaseHead = head;
};


template<bool ForMemoryPool, bool PtrPayload>
void MPSC_VirtInfArray_Impl<ForMemoryPool, PtrPayload>::enqueue(void *item) {
  Ui64 slot = tailCounter.fetch_add(1, MO_ACQUIRE);
  ChunkType *current_chunk = tail.load(MO_ACQUIRE);

  if (current_chunk->StartSlot > slot) {
    do {
      current_chunk = current_chunk->Prev;
    } while (current_chunk->StartSlot > slot);
  } else if (current_chunk->StartSlot + numberOfSlotsInChunk <= slot) {
    /* Construct virtual array */

    // no need for unique_ptr because only allocateNew can throw
    // an exception and cache == nullptr in the case
    ChunkType *cache = nullptr;
    do {
      auto next = current_chunk->Next.load(MO_ACQUIRE);
      auto prev_chunk = current_chunk;
      if (next != nullptr) {
        current_chunk = next;
      } else {
        ChunkType *new_chunk;
        if (cache == nullptr) {
          new_chunk = allocateChunk(
            current_chunk,
            current_chunk->StartSlot + numberOfSlotsInChunk);
        } else {
          new_chunk = resetChunk(
            cache,
            current_chunk,
            current_chunk->StartSlot + numberOfSlotsInChunk);
          cache = nullptr;
        }

        bool set = current_chunk->Next.compare_exchange_strong(
          next, new_chunk, MO_SEQUENCE);
        if (set) {
          current_chunk = new_chunk;
        } else {
          cache = new_chunk;
          current_chunk = next;
        }
      }

      tail.compare_exchange_strong(
        prev_chunk, current_chunk, MO_SEQUENCE);

      if (current_chunk->ReleaseCounter.load(MO_ACQUIRE) != 0) {
        continue;
      }

      // ReleaseCounter of the current_chunk is not set,
      // let's help another thread to set ReleaseCounter
      auto current_tail_counter = tailCounter.load(MO_ACQUIRE);
      Ui64 release_counter = 0;
      bool set;
      do {
        set = current_chunk->ReleaseCounter.compare_exchange_strong(
          release_counter, current_tail_counter, MO_SEQUENCE);
      } while (!set && release_counter > current_tail_counter);

    } while (current_chunk->StartSlot + numberOfSlotsInChunk <= slot);

    delete cache;
  }

  /* Chunk found */
  setSlot(current_chunk, slot, item);
}


template<bool ForMemoryPool, bool PtrPayload>
bool MPSC_VirtInfArray_Impl<ForMemoryPool, PtrPayload>::dequeue(void *result) {
  look_through_skipped_slots:
  while (!retrySlots.empty()) {
    Ui32 chunk_slot = frontRetry().slot;
    ChunkType *chunk = frontRetry().chunk;
    bool ready = getSlot(result, chunk, chunk_slot);
    if (ready) {
      popFrontRetry(); // no-throw
      if (skippedSlots.empty()) {
        checkForRelease();
      }
      return true;
    }
    pushBackSkipped(frontRetry()); // may throw
    popFrontRetry(); // no-throw
  }

  next_slot:
  /* The run is a dequeueing sequence of slots with increasing
   * slot numbers, whenever the run seeks lastKnownTailCounter
   * the current run must stop and the new run begins from
   * the first skipped slot. */
  if (headCounter == lastKnownTailCounter) {
    // End of the run reached
    lastKnownTailCounter = tailCounter.load(MO_ACQUIRE);
    if (!skippedSlots.empty()) {
      /* There is at least one skipped slot.
       * Let's start the new run from the first skipped slot */
      std::swap(skippedSlots, retrySlots);
      goto look_through_skipped_slots;
    }
    /* There is no skipped slots. Let's continue the current run
     * with new lastKnownTailCounter */
    if (headCounter == lastKnownTailCounter) {
      return false;
    } // no items in the queue
  }

  Ui32 head_slot = headCounter - head->StartSlot;
  if (head_slot == numberOfSlotsInChunk) {
    // the slot in the next chunk, let's seek the chunk
    auto new_head = head->Next.load(MO_ACQUIRE);
    if (new_head == nullptr) {
      if (skippedSlots.empty()) {
        return false;
      } // no items in the queue
      std::swap(skippedSlots, retrySlots);
      goto look_through_skipped_slots;
    }
    head = new_head;
    head_slot = 0;
  }

  ++headCounter;
  bool ready = getSlot(result, head, head_slot);
  if (ready) {
    if (skippedSlots.empty()) {
      checkForRelease();
    }
    return true;
  }
  pushBackSkipped({head_slot, head});
  goto next_slot;
}


template<bool ForMemoryPool, bool PtrPayload>
void MPSC_VirtInfArray_Impl<ForMemoryPool, PtrPayload>::checkForRelease() {
  Ui64 least_slot = headCounter;
  if (!retrySlots.empty()) {
    const auto &front = frontRetry();
    Ui64 front_slot = front.chunk->StartSlot + front.slot;
    if (front_slot < least_slot) {
      least_slot = front_slot;
    }
  }

  while (releaseHead->releaseBefore(this, least_slot)) {
    if (head == releaseHead) {
      break;
    }
    auto next = releaseHead->Next.load(MO_RELAXED);
    if (next == nullptr) {
      break;
    }
    releaseHead = next;
  }
}


template<typename Params,
  bool FOR_MEMORY_POOL,
  typename...ForwardParams>
class MPSC_VirtInfArray_ConstructorSelector;


template<typename Params, typename...ForwardParams>
class MPSC_VirtInfArray_ConstructorSelector<Params, false, ForwardParams...> {
public:
  MPSC_VirtInfArray_ConstructorSelector(
    ForwardParams...params, Ui32 size = Params::DEFAULT_CHUNK_SIZE)
    : impl(params..., size) {}

protected:
  MPSC_VirtInfArray_Impl<
    Params::FOR_MEMORY_POOL,
    std::is_pointer<typename Params::PayloadType>::value> impl;
};


template<typename Params, typename...ForwardParams>
class MPSC_VirtInfArray_ConstructorSelector<Params, true, ForwardParams...> {
public:
  MPSC_VirtInfArray_ConstructorSelector(
    ForwardParams...params, I_FixedSizeAllocator *pool)
    : impl(pool, params...) {}

protected:
  MPSC_VirtInfArray_Impl<
    Params::FOR_MEMORY_POOL,
    std::is_pointer<typename Params::PayloadType>::value> impl;
};


template<
  typename Params,
  typename ForwardType,
  bool DELETE_PAYLOAD = Params::DELETE_PAYLOAD_IN_DESTRUCTOR,
  bool USE_FREE = std::is_pointer<typename Params::PayloadType>::value>
class MPSC_VirtInfArray_DeleteSelector {
};

template<typename Params, typename ForwardType>
class MPSC_VirtInfArray_DeleteSelector<Params, ForwardType, true, false> {
private:
  using PayloadType = typename Params::PayloadType;

public:
  ~MPSC_VirtInfArray_DeleteSelector()
  noexcept(std::is_nothrow_destructible<PayloadType>::value) {
    for (;;) {
      PayloadType *item = static_cast<ForwardType *>(this)->dequeue();
      if (!item) {
        break;
      }
      delete item;
    }
  }
};

template<typename Params, typename ForwardType>
class MPSC_VirtInfArray_DeleteSelector<Params, ForwardType, true, true> {
public:
  ~MPSC_VirtInfArray_DeleteSelector() {
    for (;;) {
      void *item = static_cast<ForwardType *>(this)->dequeue();
      if (!item) {
        break;
      }
      free(item);
    }
  }
};


enum class EnqueueDequeueAPIEnum {
  POINTER,
  ARRAY,
  OTHER,
};


template<typename PayloadType>
struct EnqueueDequeueAPIEnumSelector {
  static constexpr EnqueueDequeueAPIEnum value =
    EnqueueDequeueAPIEnum::OTHER;
};

template<typename PayloadType>
struct EnqueueDequeueAPIEnumSelector<PayloadType *> {
  static constexpr EnqueueDequeueAPIEnum value =
    EnqueueDequeueAPIEnum::POINTER;
};

template<typename PayloadType, size_t SIZE>
struct EnqueueDequeueAPIEnumSelector<PayloadType[SIZE]> {
  static constexpr EnqueueDequeueAPIEnum value =
    EnqueueDequeueAPIEnum::ARRAY;
};


template<
  typename Params,
  EnqueueDequeueAPIEnum PAYLOAD_TYPE =
  EnqueueDequeueAPIEnumSelector<typename Params::PayloadType>::value>
class MpscVirtInfArray_Aux;


template<typename Params>
class MpscVirtInfArray_Aux<Params, EnqueueDequeueAPIEnum::POINTER>
  : public MPSC_VirtInfArray_ConstructorSelector<
    Params, Params::FOR_MEMORY_POOL>
    , public MPSC_VirtInfArray_DeleteSelector<
    Params, MpscVirtInfArray_Aux<Params>> {
private:
  typedef typename Params::PayloadType PayloadType;
  typedef
  MPSC_VirtInfArray_ConstructorSelector<
    Params, Params::FOR_MEMORY_POOL>
    BaseType;

public:
  template<typename...CallParams>
  inline MpscVirtInfArray_Aux(CallParams &&...params)
    : BaseType(std::forward<CallParams>(params)...) {}

  void enqueue(PayloadType item) {
    BaseType::impl.enqueue(item);
  }

  inline PayloadType dequeue() {
    void *item;
    bool ready = BaseType::impl.dequeue(&item);
    if (!ready) {
      return nullptr;
    }
    return reinterpret_cast<PayloadType>(item);
  }
};


template<typename Params>
class MpscVirtInfArray_Aux<Params, EnqueueDequeueAPIEnum::OTHER>
  : public MPSC_VirtInfArray_ConstructorSelector<
    Params, Params::FOR_MEMORY_POOL, size_t>
    , public MPSC_VirtInfArray_DeleteSelector<
    Params, MpscVirtInfArray_Aux<Params>> {
private:
  typedef typename Params::PayloadType PayloadType;
  typedef
  MPSC_VirtInfArray_ConstructorSelector<
    Params, Params::FOR_MEMORY_POOL, size_t>
    BaseType;

public:
  template<typename...CallParams>
  inline MpscVirtInfArray_Aux(CallParams &&...params)
    : BaseType(sizeof(typename Params::PayloadType),
    std::forward<CallParams>(params)...) {}

  void enqueue(PayloadType item) {
    BaseType::impl.enqueue(&item);
  }

  inline bool dequeue(PayloadType *item) {
    return BaseType::impl.dequeue(item);
  }
};


template<typename Params>
class MpscVirtInfArray_Aux<Params, EnqueueDequeueAPIEnum::ARRAY>
  : public MPSC_VirtInfArray_ConstructorSelector<
    Params, Params::FOR_MEMORY_POOL, size_t>
    , public MPSC_VirtInfArray_DeleteSelector<
    Params, MpscVirtInfArray_Aux<Params>> {
private:
  typedef typename Params::PayloadType PayloadType;
  typedef
  MPSC_VirtInfArray_ConstructorSelector<
    Params, Params::FOR_MEMORY_POOL, size_t>
    BaseType;

public:
  template<typename...CallParams>
  inline MpscVirtInfArray_Aux(CallParams &&...params)
    : BaseType(sizeof(typename Params::PayloadType),
    std::forward<CallParams>(params)...) {}

  void enqueue(PayloadType item) {
    BaseType::impl.enqueue(item);
  }

  inline bool dequeue(PayloadType item) {
    return BaseType::impl.dequeue(item);
  }
};


struct MpscVirtInfArray_Default_Params {
  static constexpr Ui32 DEFAULT_CHUNK_SIZE = 16;
  static constexpr bool FOR_MEMORY_POOL = false;
  static constexpr bool DELETE_PAYLOAD_IN_DESTRUCTOR = false;
};


} // namespace dtl


DECLARE_TUNE_TYPE_PARAM(TunePayload, PayloadType);
DECLARE_TUNE_VALUE_PARAM(TuneChunkSize, Ui32, DEFAULT_CHUNK_SIZE);
DECLARE_TUNE_VALUE_PARAM_DEFVALUE(
    TuneDeletePayloadFlag, bool, true, DELETE_PAYLOAD_IN_DESTRUCTOR);
DECLARE_TUNE_VALUE_PARAM_DEFVALUE(
    TuneMemoryPoolFlag, bool, true, FOR_MEMORY_POOL);


template<typename PayloadParam = void *, typename...MoreParams>
class MpscVirtInfArray
  : public dtl::MpscVirtInfArray_Aux<
    FuseParams<
      dtl::MpscVirtInfArray_Default_Params,
      TunePayload<PayloadParam>,
      MoreParams...
    >
  > {
private:
  using BaseType =
  dtl::MpscVirtInfArray_Aux<
    FuseParams<
      dtl::MpscVirtInfArray_Default_Params,
      TunePayload<PayloadParam>,
      MoreParams...
    >
  >;
public:
  template<typename...CallParams>
  inline MpscVirtInfArray(CallParams &&...params)
    : BaseType(std::forward<CallParams>(params)...) {}

  bool isOK() noexcept {
    return BaseType::impl.isOK();
  }
};


} // namespace arctic

#endif  // ENGINE_MTQ_MPSC_VINFARR_H_
