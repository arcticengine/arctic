// The lock-free queues of engine/mtq_*.h.
#define TEST_NO_MAIN
#include "test_helpers.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>
#include <vector>

#include "engine/mtq_fixed_block_queue.h"
#include "engine/mtq_mpmc_befsbfsp_allocator.h"
#include "engine/mtq_mpsc_vinfarr.h"

// MpscVirtInfArray allocates a chunk as one variable-sized block through
// ::operator new(size) and used to free it with a plain delete of the header
// type. With sized deallocation (the default of clang 19 and later on Linux,
// and switched on for this suite by its CMakeLists.txt) that delete passes
// sizeof(header) to ::operator delete(void *, size_t), which does not match
// the size the block was allocated with. AddressSanitizer aborts on the
// mismatch (new-delete-type-mismatch); this is what killed rehammer's logger
// thread. A release build has no sanitizer, so the suite replaces the global
// allocation functions with a pair that remembers the requested size and counts
// sized deletes whose size disagrees. Under AddressSanitizer the replacement is
// left out and the sanitizer is the judge.
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define TESTS_ADDRESS_SANITIZER 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define TESTS_ADDRESS_SANITIZER 1
#endif

namespace {

std::atomic<Ui64> g_sized_delete_mismatches{0};

#if !defined(TESTS_ADDRESS_SANITIZER)
// The header keeps the block aligned for anything ::operator new(size) may be
// asked for (alignof(max_align_t) is 16 on the supported platforms).
const std::size_t kAllocationHeaderSize = 16;
#endif

}  // namespace

#if !defined(TESTS_ADDRESS_SANITIZER)

void *operator new(std::size_t size) {
  void *block = std::malloc(size + kAllocationHeaderSize);
  if (block == nullptr) {
    throw std::bad_alloc();
  }
  *static_cast<std::size_t *>(block) = size;
  return static_cast<char *>(block) + kAllocationHeaderSize;
}

void operator delete(void *ptr) noexcept {
  if (ptr == nullptr) {
    return;
  }
  std::free(static_cast<char *>(ptr) - kAllocationHeaderSize);
}

void operator delete(void *ptr, std::size_t size) noexcept {
  if (ptr == nullptr) {
    return;
  }
  char *block = static_cast<char *>(ptr) - kAllocationHeaderSize;
  if (*reinterpret_cast<std::size_t *>(block) != size) {
    g_sized_delete_mismatches.fetch_add(1, std::memory_order_relaxed);
  }
  std::free(block);
}

#endif  // !TESTS_ADDRESS_SANITIZER

namespace {

Ui64 SizedDeleteMismatches() {
  return g_sized_delete_mismatches.load(std::memory_order_relaxed);
}

typedef MpscVirtInfArray<Si32 *, TuneChunkSize<4>> SmallChunkQueue;
typedef MpscVirtInfArray<Si32 *, TuneChunkSize<1>> OneSlotChunkQueue;

void EnqueueAll(OneSlotChunkQueue *queue, Si32 *values, Si32 count) {
  for (Si32 i = 0; i < count; ++i) {
    queue->enqueue(&values[i]);
  }
}

}  // namespace

// The consumer releases a chunk once it has read past it. Four slots per chunk
// and sixty-four items make fifteen releases and one more of every chunk in
// the destructor, each of which used to be a mismatched sized delete.
void test_mpsc_vinfarr_frees_chunks_with_matching_size() {
  const Si32 kCount = 64;
  Si32 values[kCount];
  for (Si32 i = 0; i < kCount; ++i) {
    values[i] = i;
  }

  const Ui64 mismatches_before = SizedDeleteMismatches();
  {
    SmallChunkQueue queue;
    TEST_CHECK(queue.isOK());
    TEST_CHECK(queue.dequeue() == nullptr);

    for (Si32 i = 0; i < kCount; ++i) {
      queue.enqueue(&values[i]);
    }
    for (Si32 i = 0; i < kCount; ++i) {
      Si32 *item = queue.dequeue();
      TEST_CHECK_(item == &values[i], "item %d came out of order", i);
    }
    TEST_CHECK(queue.dequeue() == nullptr);
    TEST_CHECK_(SizedDeleteMismatches() == mismatches_before,
      "released chunks were freed with a wrong size (%llu mismatches)",
      (unsigned long long)(SizedDeleteMismatches() - mismatches_before));

    // Interleaved traffic crosses chunk borders the way the logger does.
    for (Si32 i = 0; i < kCount; ++i) {
      queue.enqueue(&values[i]);
      Si32 *item = queue.dequeue();
      TEST_CHECK_(item == &values[i], "interleaved item %d is wrong", i);
    }
    TEST_CHECK(queue.dequeue() == nullptr);
  }
  TEST_CHECK_(SizedDeleteMismatches() == mismatches_before,
    "the destructor freed chunks with a wrong size (%llu mismatches)",
    (unsigned long long)(SizedDeleteMismatches() - mismatches_before));
}

// Two producers racing on one-slot chunks: the one that loses the race for
// Next keeps its chunk as a cache and, when its own slot turns out to be in
// the winner's chunk, has to discard the cache the same way the consumer
// frees a released chunk. Every item must still come out exactly once.
void test_mpsc_vinfarr_two_producers_deliver_every_item() {
  const Si32 kPerProducer = 20000;
  std::vector<Si32> values_a(kPerProducer);
  std::vector<Si32> values_b(kPerProducer);
  for (Si32 i = 0; i < kPerProducer; ++i) {
    values_a[i] = i;
    values_b[i] = kPerProducer + i;
  }
  std::vector<Si32> seen(2 * kPerProducer, 0);

  const Ui64 mismatches_before = SizedDeleteMismatches();
  {
    OneSlotChunkQueue queue;
    TEST_CHECK(queue.isOK());

    std::thread producer_a(EnqueueAll, &queue, values_a.data(), kPerProducer);
    std::thread producer_b(EnqueueAll, &queue, values_b.data(), kPerProducer);

    Si32 received = 0;
    Si32 idle_spins = 0;
    while (received < 2 * kPerProducer && idle_spins < 20000000) {
      Si32 *item = queue.dequeue();
      if (item == nullptr) {
        ++idle_spins;
        std::this_thread::yield();
        continue;
      }
      ++received;
      ++seen[*item];
    }
    producer_a.join();
    producer_b.join();

    TEST_CHECK_(received == 2 * kPerProducer,
      "received %d of %d items", received, 2 * kPerProducer);
    TEST_CHECK(queue.dequeue() == nullptr);
    for (Si32 i = 0; i < 2 * kPerProducer; ++i) {
      if (seen[i] != 1) {
        TEST_CHECK_(false, "item %d was dequeued %d times", i, seen[i]);
        break;
      }
    }
  }
  TEST_CHECK_(SizedDeleteMismatches() == mismatches_before,
    "chunks were freed with a wrong size (%llu mismatches)",
    (unsigned long long)(SizedDeleteMismatches() - mismatches_before));
}

namespace {

const Si32 kPerProducer = 20000;

struct ReleaseRaceRound {
  OneSlotChunkQueue queue;
  std::vector<Si32> values_a;
  std::vector<Si32> values_b;
  std::vector<Si32> seen;
  std::atomic<Si32> received{0};
  std::atomic<bool> done{false};

  ReleaseRaceRound()
    : values_a(kPerProducer)
    , values_b(kPerProducer)
    , seen(2 * kPerProducer, 0) {
    for (Si32 i = 0; i < kPerProducer; ++i) {
      values_a[i] = i;
      values_b[i] = kPerProducer + i;
    }
  }

  void Consume() {
    while (received.load(std::memory_order_relaxed) < 2 * kPerProducer) {
      Si32 *item = queue.dequeue();
      if (item == nullptr) {
        std::this_thread::yield();
        continue;
      }
      ++seen[*item];
      received.fetch_add(1, std::memory_order_relaxed);
    }
    done.store(true, std::memory_order_release);
  }
};

}  // namespace

// A producer links a chunk and publishes its ReleaseCounter a moment later;
// another producer may fill the chunk in between. The consumer read that item
// and took the unpublished counter (0) for "every producer is past the
// previous chunk", freed it while a third producer still walked it from a
// stale tail, and that producer wrote its item into freed memory. The item
// was lost and dequeue then spun forever on its slot, so the consumer runs on
// a thread of its own here and a stuck one is a failure, not a hang.
void test_mpsc_vinfarr_keeps_chunks_until_release_counter_published() {
  const Si32 kRounds = 300;
  for (Si32 round = 0; round < kRounds; ++round) {
    ReleaseRaceRound *state = new ReleaseRaceRound;
    std::thread consumer(&ReleaseRaceRound::Consume, state);
    std::thread producer_a(EnqueueAll, &state->queue,
      state->values_a.data(), kPerProducer);
    std::thread producer_b(EnqueueAll, &state->queue,
      state->values_b.data(), kPerProducer);
    producer_a.join();
    producer_b.join();

    const double deadline = Time() + 5.0;
    while (!state->done.load(std::memory_order_acquire)
        && Time() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!state->done.load(std::memory_order_acquire)) {
      TEST_CHECK_(false, "round %d: the consumer is stuck with %d of %d "
        "items, an item was lost", (int)round,
        (int)state->received.load(), 2 * kPerProducer);
      // The stuck consumer still uses the queue, so both are left behind.
      consumer.detach();
      return;
    }
    consumer.join();
    for (Si32 i = 0; i < 2 * kPerProducer; ++i) {
      if (state->seen[i] != 1) {
        TEST_CHECK_(false, "round %d: item %d was dequeued %d times",
          (int)round, (int)i, (int)state->seen[i]);
        break;
      }
    }
    delete state;
  }
}

// Logger enqueues `new std::string` with TuneDeletePayloadFlag<true>. The
// pointer DeleteSelector used to free() those payloads, which skips the
// destructor. A type whose destructor flips a counter proves delete runs.
namespace {

std::atomic<Si32> g_probe_live_count{0};

struct DestructorProbe {
  DestructorProbe() {
    g_probe_live_count.fetch_add(1, std::memory_order_relaxed);
  }
  ~DestructorProbe() {
    g_probe_live_count.fetch_sub(1, std::memory_order_relaxed);
  }
};

typedef MpscVirtInfArray<DestructorProbe *, TuneDeletePayloadFlag<true>,
    TuneChunkSize<4>> ProbeQueue;

}  // namespace

void test_mpsc_vinfarr_destructor_deletes_pointer_payloads() {
  TEST_CHECK(g_probe_live_count.load() == 0);
  {
    ProbeQueue queue;
    TEST_CHECK(queue.isOK());
    const Si32 kCount = 17;
    for (Si32 i = 0; i < kCount; ++i) {
      queue.enqueue(new DestructorProbe);
    }
    TEST_CHECK_(g_probe_live_count.load() == kCount,
        "expected %d live probes before drain, got %d",
        kCount, g_probe_live_count.load());
    // Leave items in the queue so the DeleteSelector destructor drains them.
  }
  TEST_CHECK_(g_probe_live_count.load() == 0,
      "queue destructor must delete pointer payloads (live=%d); free() would "
      "leave destructors unrun",
      g_probe_live_count.load());
}

namespace {

struct BlockQueueItem {
  Ui64 value;
  Ui64 pad;
};

// A 48-byte block holds two 16-byte items after its 16-byte header.
typedef MpmcNoFallbackFixedSizeBufferFixedSizePool<8, 48> TwoItemBlockPool;

template<typename Pool>
size_t CountFreeBlocks(Pool *pool) {
  std::vector<void *> blocks;
  for (void *block = pool->alloc(); block != nullptr; block = pool->alloc()) {
    blocks.push_back(block);
  }
  for (void *block : blocks) {
    pool->free(block);
  }
  return blocks.size();
}

}  // namespace

// The MPSC consumer keeps its skipped slots in a FixedBlockQueue. Dropping the
// front block took the block capacity off back_offset too, though back_offset
// counts in the back block: it wrapped around, the queue never turned empty
// again and front() handed out an item that was never pushed.
void test_fixed_block_queue_spans_many_blocks() {
  TwoItemBlockPool pool;
  {
    FixedBlockQueue<BlockQueueItem> queue;
    TEST_CHECK(queue.empty());
    // Five items take three blocks; the queue is drained twice so the
    // front moves through the middle block and reaches the back one.
    for (Si32 pass = 0; pass < 2; ++pass) {
      const Ui64 kCount = 5;
      for (Ui64 i = 0; i < kCount; ++i) {
        queue.push_back(BlockQueueItem{100 * pass + i, 0}, &pool);
      }
      TEST_CHECK_(CountFreeBlocks(&pool) == 8 - 3,
        "pass %d: five items must take three blocks, %d free",
        (int)pass, (int)CountFreeBlocks(&pool));
      for (Ui64 i = 0; i < kCount; ++i) {
        TEST_CHECK_(!queue.empty(), "pass %d: the queue emptied after %d of "
          "%d items", (int)pass, (int)i, (int)kCount);
        if (queue.empty()) {
          break;
        }
        const Ui64 got = queue.front(&pool).value;
        TEST_CHECK_(got == 100 * pass + i, "pass %d: item %d is %llu, "
          "expected %llu", (int)pass, (int)i, (unsigned long long)got,
          (unsigned long long)(100 * pass + i));
        queue.pop_front(&pool);
      }
      TEST_CHECK_(queue.empty(), "pass %d: the queue must be empty after "
        "every item was popped", (int)pass);
      TEST_CHECK_(CountFreeBlocks(&pool) == 8, "pass %d: every block must "
        "return to the pool, %d of 8 free", (int)pass,
        (int)CountFreeBlocks(&pool));
    }

    // FIFO with the queue never empty, as the skipped slots of a busy
    // consumer: the front keeps leaving blocks the back has already left.
    Ui64 next_push = 0;
    Ui64 next_pop = 0;
    bool in_order = true;
    for (Si32 step = 0; step < 100 && in_order; ++step) {
      queue.push_back(BlockQueueItem{next_push++, 0}, &pool);
      queue.push_back(BlockQueueItem{next_push++, 0}, &pool);
      queue.push_back(BlockQueueItem{next_push++, 0}, &pool);
      for (Si32 k = 0; k < 3 - (step < 2 ? 1 : 0) && !queue.empty(); ++k) {
        if (queue.front(&pool).value != next_pop) {
          in_order = false;
          break;
        }
        ++next_pop;
        queue.pop_front(&pool);
      }
    }
    TEST_CHECK_(in_order, "a long-lived queue returned %llu out of order",
      (unsigned long long)next_pop);
    while (in_order && next_pop < next_push) {
      TEST_CHECK(!queue.empty());
      if (queue.empty() || queue.front(&pool).value != next_pop) {
        in_order = false;
        break;
      }
      ++next_pop;
      queue.pop_front(&pool);
    }
    TEST_CHECK_(in_order && next_pop == next_push && queue.empty(),
      "the long-lived queue must drain to empty, popped %llu of %llu",
      (unsigned long long)next_pop, (unsigned long long)next_push);
  }
  TEST_CHECK_(CountFreeBlocks(&pool) == 8,
    "every block must return to the pool, %d of 8 free",
    (int)CountFreeBlocks(&pool));
}

namespace {

// A block that holds the chunk header and exactly two slots.
const size_t kTwoSlotChunkBlockSize =
  sizeof(arctic::dtl::InfArrayChunk<true>) + 2 * sizeof(Si32 *);

// Two-slot chunks, so producers cross chunk borders all the time and the
// consumer frees chunks right behind them.
// The consumer frees at most one chunk per dequeue and none while it has
// skipped slots, so with two-slot chunks the freed chunks lag behind and the
// pool needs room far beyond the in-flight items.
const size_t kTinyPoolBlocks = 8192;
typedef MpmcNoFallbackFixedSizeBufferFixedSizePool<kTinyPoolBlocks,
    kTwoSlotChunkBlockSize> TinyChunkPool;
typedef MpscVirtInfArray<Si32 *, TuneMemoryPoolFlag<true>> PoolQueue;

const Si32 kPoolProducers = 8;
const Si32 kPoolPerProducer = 50000;
const Si32 kPoolTotal = kPoolProducers * kPoolPerProducer;
// Keeps the backlog well within the kMpscMaxSkippedSlots items a pool-backed
// queue may hold.
const Si32 kPoolMaxInFlight = 128;

template<typename Pool>
struct PoolProducersRound {
  Pool pool;
  std::unique_ptr<PoolQueue> queue;
  std::vector<Si32> values;
  std::vector<Si32> seen;
  std::atomic<Si32> sent{0};
  std::atomic<Si32> received{0};
  // Every thread of the round counts itself here when it returns.
  std::atomic<Si32> finished{0};
  // Set by the test on timeout so the waiting loops stop spinning.
  std::atomic<bool> give_up{false};

  PoolProducersRound()
    : queue(new PoolQueue(&pool))
    , values(kPoolTotal)
    , seen(kPoolTotal, 0) {
    for (Si32 i = 0; i < kPoolTotal; ++i) {
      values[i] = i;
    }
  }

  bool GaveUp() const {
    return give_up.load(std::memory_order_relaxed);
  }

  void Produce(Si32 producer) {
    for (Si32 i = 0; i < kPoolPerProducer && !GaveUp(); ++i) {
      while (sent.load(std::memory_order_relaxed)
          - received.load(std::memory_order_relaxed) > kPoolMaxInFlight
          && !GaveUp()) {
        std::this_thread::yield();
      }
      Si32 *item = &values[producer * kPoolPerProducer + i];
      queue->enqueue(item);
      sent.fetch_add(1, std::memory_order_relaxed);
    }
    finished.fetch_add(1, std::memory_order_release);
  }

  void Consume() {
    while (received.load(std::memory_order_relaxed) < kPoolTotal
        && !GaveUp()) {
      Si32 *item = queue->dequeue();
      if (item == nullptr) {
        std::this_thread::yield();
        continue;
      }
      ++seen[*item];
      received.fetch_add(1, std::memory_order_relaxed);
    }
    finished.fetch_add(1, std::memory_order_release);
  }
};

}  // namespace

// Eight producers and a consumer on two-slot pool chunks: every item must come
// out exactly once and every pool block must come back.
void test_mpsc_vinfarr_pool_producers_deliver_every_item() {
  typedef PoolProducersRound<TinyChunkPool> Round;
  const Si32 kRounds = 10;
  for (Si32 round = 0; round < kRounds; ++round) {
    Round *state = new Round;
    TEST_CHECK(state->queue->isOK());
    std::thread consumer(&Round::Consume, state);
    std::vector<std::thread> producers;
    for (Si32 p = 0; p < kPoolProducers; ++p) {
      producers.emplace_back(&Round::Produce, state, p);
    }

    // A lost item stalls the consumer, and the producers then wait for it
    // on the in-flight limit; a thread stuck inside the queue never returns.
    const Si32 kThreads = kPoolProducers + 1;
    const double deadline = Time() + 20.0;
    while (state->finished.load(std::memory_order_acquire) < kThreads
        && Time() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (state->finished.load(std::memory_order_acquire) < kThreads) {
      TEST_CHECK_(false, "round %d: stuck with %d of %d items sent and %d "
        "received, an item was lost or a producer hangs in the queue",
        (int)round, (int)state->sent.load(), (int)kPoolTotal,
        (int)state->received.load());
      state->give_up.store(true);
      // A thread may be stuck for good, so the state is left to them.
      consumer.detach();
      for (std::thread &producer : producers) {
        producer.detach();
      }
      return;
    }
    consumer.join();
    for (std::thread &producer : producers) {
      producer.join();
    }
    for (Si32 i = 0; i < kPoolTotal; ++i) {
      if (state->seen[i] != 1) {
        TEST_CHECK_(false, "round %d: item %d was dequeued %d times",
          (int)round, (int)i, (int)state->seen[i]);
        break;
      }
    }
    TEST_CHECK_(state->queue->dequeue() == nullptr,
      "round %d: the queue must be empty after every item came out",
      (int)round);

    state->queue.reset();
    size_t returned = 0;
    std::vector<void *> blocks;
    for (void *block = state->pool.alloc(); block != nullptr;
        block = state->pool.alloc()) {
      blocks.push_back(block);
      ++returned;
    }
    for (void *block : blocks) {
      state->pool.free(block);
    }
    TEST_CHECK_(returned == kTinyPoolBlocks,
      "round %d: %d of %d pool blocks came back after the queue was "
      "destroyed",
      (int)round, (int)returned, (int)kTinyPoolBlocks);
    delete state;
  }
}

namespace {

// Two blocks: the first two two-slot chunks come from the pool, the rest
// from the heap.
typedef MpmcNoFallbackFixedSizeBufferFixedSizePool<2, kTwoSlotChunkBlockSize>
    TwoChunkPool;
typedef MpmcNoFallbackFixedSizeBufferFixedSizePool<8, kTwoSlotChunkBlockSize>
    EightChunkPool;

}  // namespace

// With an empty pool a producer takes a chunk from the heap. The consumer may
// run in a signal handler and must not free it: it parks the chunk, and only
// freeReleasedHeapChunks, called where the heap is allowed, frees it. A heap
// chunk must never be handed to the pool, which would take it for one of its
// own blocks.
void test_mpsc_vinfarr_heap_chunks_wait_for_game_thread() {
  TwoChunkPool pool;
  const Si32 kCount = 12;
  Si32 values[kCount];
  for (Si32 i = 0; i < kCount; ++i) {
    values[i] = i;
  }
  {
    PoolQueue queue(&pool);
    TEST_CHECK(queue.isOK());
    for (Si32 i = 0; i < kCount; ++i) {
      queue.enqueue(&values[i]);
    }
    TEST_CHECK_(CountFreeBlocks(&pool) == 0,
      "the pool must be used up before the heap, %d blocks free",
      (int)CountFreeBlocks(&pool));
    TEST_CHECK_(queue.freeReleasedHeapChunks() == 0,
      "nothing is released before the consumer reads");
    for (Si32 i = 0; i < kCount; ++i) {
      Si32 *item = queue.dequeue();
      TEST_CHECK_(item == &values[i], "item %d came out wrong", (int)i);
    }
    TEST_CHECK(queue.dequeue() == nullptr);

    // Six chunks: the consumer released all but the one it reads from. The
    // two pool chunks went straight back to the pool, the three released heap
    // chunks were parked.
    TEST_CHECK_(CountFreeBlocks(&pool) == 2,
      "released pool chunks must return to the pool, %d of 2 free",
      (int)CountFreeBlocks(&pool));
    const size_t freed = queue.freeReleasedHeapChunks();
    TEST_CHECK_(freed == 3,
      "the consumer must park the released heap chunks, %d freed",
      (int)freed);
    TEST_CHECK_(queue.freeReleasedHeapChunks() == 0,
      "parked chunks must be freed once");

    // The queue keeps going on pool chunks again.
    for (Si32 i = 0; i < kCount; ++i) {
      queue.enqueue(&values[i]);
      Si32 *item = queue.dequeue();
      TEST_CHECK_(item == &values[i], "item %d after the round trip came "
        "out wrong", (int)i);
    }
  }
  TEST_CHECK_(CountFreeBlocks(&pool) == 2,
    "the destructor must return pool chunks to the pool and free heap "
    "chunks itself, %d of 2 free", (int)CountFreeBlocks(&pool));
}

// Eight producers race to link chunks of an empty pool, and the ones that
// lose free their heap chunks at once. Then the consumer releases the rest
// while another thread frees what it has parked. Items are all written
// before the consumer starts, so it skips no slot.
void test_mpsc_vinfarr_concurrent_heap_chunks_are_freed() {
  const Si32 kProducers = 8;
  const Si32 kPerProducer = 20000;
  const Si32 kTotal = kProducers * kPerProducer;
  const Si32 kRounds = 3;
  std::vector<Si32> values(kTotal);
  for (Si32 i = 0; i < kTotal; ++i) {
    values[i] = i;
  }
  for (Si32 round = 0; round < kRounds; ++round) {
    EightChunkPool pool;
    std::unique_ptr<PoolQueue> queue(new PoolQueue(&pool));
    std::vector<std::thread> producers;
    for (Si32 p = 0; p < kProducers; ++p) {
      producers.emplace_back([&queue, &values, p, kPerProducer]() {
        for (Si32 i = 0; i < kPerProducer; ++i) {
          queue->enqueue(&values[p * kPerProducer + i]);
        }
      });
    }
    for (std::thread &producer : producers) {
      producer.join();
    }
    TEST_CHECK_(CountFreeBlocks(&pool) == 0,
      "round %d: the producers must have used up the pool", (int)round);

    std::vector<Si32> seen(kTotal, 0);
    std::atomic<bool> drained{false};
    std::thread consumer([&queue, &seen, &drained, kTotal]() {
      for (Si32 received = 0; received < kTotal;) {
        Si32 *item = queue->dequeue();
        if (item == nullptr) {
          break;
        }
        ++seen[*item];
        ++received;
      }
      drained.store(true, std::memory_order_release);
    });
    size_t freed = 0;
    while (!drained.load(std::memory_order_acquire)) {
      freed += queue->freeReleasedHeapChunks();
      std::this_thread::yield();
    }
    consumer.join();
    freed += queue->freeReleasedHeapChunks();

    for (Si32 i = 0; i < kTotal; ++i) {
      if (seen[i] != 1) {
        TEST_CHECK_(false, "round %d: item %d was dequeued %d times",
          (int)round, (int)i, (int)seen[i]);
        break;
      }
    }
    // kTotal / 2 two-slot chunks, all but the pool ones and the last few
    // came from the heap and were released.
    TEST_CHECK_(freed + 16 >= static_cast<size_t>(kTotal / 2),
      "round %d: only %d of about %d heap chunks were parked and freed",
      (int)round, (int)freed, (int)(kTotal / 2));
    queue.reset();
    TEST_CHECK_(CountFreeBlocks(&pool) == 8,
      "round %d: %d of 8 pool blocks came back", (int)round,
      (int)CountFreeBlocks(&pool));
  }
}

// Producers are allowed to empty the pool: they take chunks from the heap
// then. The consumer, which may run in a signal handler, still needs blocks
// for the slots it skips while a producer is writing them. It used to take
// those from the same pool and abort when the producers had emptied it.
// Here the pool has two blocks, eight producers run beside the consumer, and
// the consumer skips slots all the time.
void test_mpsc_vinfarr_consumer_skips_slots_with_empty_pool() {
  typedef PoolProducersRound<TwoChunkPool> Round;
  const Si32 kRounds = 3;
  for (Si32 round = 0; round < kRounds; ++round) {
    Round *state = new Round;
    TEST_CHECK(state->queue->isOK());
    std::thread consumer(&Round::Consume, state);
    std::vector<std::thread> producers;
    for (Si32 p = 0; p < kPoolProducers; ++p) {
      producers.emplace_back(&Round::Produce, state, p);
    }

    const Si32 kThreads = kPoolProducers + 1;
    const double deadline = Time() + 20.0;
    size_t freed = 0;
    while (state->finished.load(std::memory_order_acquire) < kThreads
        && Time() < deadline) {
      freed += state->queue->freeReleasedHeapChunks();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (state->finished.load(std::memory_order_acquire) < kThreads) {
      TEST_CHECK_(false, "round %d: stuck with %d of %d items sent and %d "
        "received", (int)round, (int)state->sent.load(), (int)kPoolTotal,
        (int)state->received.load());
      state->give_up.store(true);
      consumer.detach();
      for (std::thread &producer : producers) {
        producer.detach();
      }
      return;
    }
    consumer.join();
    for (std::thread &producer : producers) {
      producer.join();
    }
    freed += state->queue->freeReleasedHeapChunks();
    for (Si32 i = 0; i < kPoolTotal; ++i) {
      if (state->seen[i] != 1) {
        TEST_CHECK_(false, "round %d: item %d was dequeued %d times",
          (int)round, (int)i, (int)state->seen[i]);
        break;
      }
    }
    // kPoolTotal / 2 two-slot chunks; a pool of two runs empty all the time
    // with up to 64 of them in flight.
    TEST_CHECK_(freed >= static_cast<size_t>(kPoolTotal / 8),
      "round %d: only %d of %d chunks came from the heap, the pool was "
      "not empty often enough", (int)round, (int)freed,
      (int)(kPoolTotal / 2));
    state->queue.reset();
    TEST_CHECK_(CountFreeBlocks(&state->pool) == 2,
      "round %d: %d of 2 pool blocks came back", (int)round,
      (int)CountFreeBlocks(&state->pool));
    delete state;
  }
}
