// The lock-free queues of engine/mtq_*.h.
#define TEST_NO_MAIN
#include "test_helpers.h"

#include <atomic>
#include <cstdlib>
#include <new>

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
