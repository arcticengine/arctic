// SIGIO heap-race harness (path 2).
//
// Historical bug: ALSA's snd_async_add_pcm_handler ran MixSound / SoundCheck
// from SIGIO while those paths allocated; concurrent malloc/free on the main
// thread could abort with "free(): double free detected in tcache".
//
// Path 2 keeps the async/SIGIO handler but makes production MixSound
// async-signal-safe (preallocated buffers, no malloc/SoundCheck in-handler).
//
// Modes:
//   --legacy-unsafe / --unsafe  old hazard → expect abort / REPRODUCED
//   --safe                      path-2 MixSound from signal under heap stress
//                               → expect clean exit
//
// Build: make tests_sigio_repro
// Run:   ./tools/alsa/run_sigio_heap_repro.sh

#include "engine/arctic_platform_def.h"

#if !defined(ARCTIC_PLATFORM_PI) || defined(ARCTIC_NO_ALSA)
#include <cstdio>
int main() {
  std::fprintf(stderr,
      "tests_sigio_repro requires Linux ALSA (ARCTIC_PLATFORM_PI without "
      "ARCTIC_NO_ALSA)\n");
  return 1;
}
#else

#include "engine/arctic_platform_sound.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>

namespace {

std::atomic<bool> g_stop{false};
pthread_t g_main_thread;
bool g_use_safe_handler = false;

void SigioReproSignalHandler(int /*signum*/) {
  if (g_use_safe_handler) {
    arctic::SoundMixerSigioReproInvokeSafeMixFromSignal();
  } else {
    arctic::SoundMixerSigioReproInvokeLegacyUnsafeFromSignal();
  }
}

void *SignalRaiserThread(void *) {
  while (!g_stop.load(std::memory_order_relaxed)) {
    pthread_kill(g_main_thread, SIGUSR1);
    // Brief pause so the main thread can enter malloc/free (race window)
    // without being completely starved.
    for (int i = 0; i < 50; ++i) {
      sched_yield();
    }
  }
  return nullptr;
}

int RunLegacyUnsafe(long iterations) {
  if (!arctic::SoundMixerSigioReproPrepareBuffers()) {
    std::fprintf(stderr, "prepare buffers failed\n");
    return 1;
  }
  g_use_safe_handler = false;
  g_main_thread = pthread_self();

  struct sigaction sa;
  std::memset(&sa, 0, sizeof sa);
  sa.sa_handler = SigioReproSignalHandler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGUSR1, &sa, nullptr) != 0) {
    std::perror("sigaction");
    return 1;
  }

  pthread_t raiser;
  if (pthread_create(&raiser, nullptr, SignalRaiserThread, nullptr) != 0) {
    std::perror("pthread_create");
    return 1;
  }

  std::fprintf(stderr,
      "tests_sigio_repro --legacy-unsafe: MixSound + malloc from SIGUSR1 "
      "while main hammers malloc/free (expects abort / REPRODUCED)\n");
  for (long i = 0; i < iterations; ++i) {
    arctic::SoundMixerSigioReproSetMainThreadInHeap(true);
    const size_t n = 16 + static_cast<size_t>(i % 2048);
    void *p = std::malloc(n);
    if (p) {
      *static_cast<volatile char *>(p) = static_cast<char>(i);
      std::free(p);
    }
    void *q = std::malloc(48);
    void *r = std::malloc(96);
    if (q) {
      std::free(q);
    }
    if (r) {
      std::free(r);
    }
    arctic::SoundMixerSigioReproSetMainThreadInHeap(false);
  }

  g_stop.store(true, std::memory_order_relaxed);
  pthread_join(raiser, nullptr);
  std::fprintf(stderr,
      "tests_sigio_repro --legacy-unsafe: survived %ld iterations without "
      "detecting the race (unexpected)\n", iterations);
  return 2;
}

int RunSafe(long iterations) {
  if (!arctic::SoundMixerSigioReproPrepareBuffers()) {
    std::fprintf(stderr, "prepare buffers failed\n");
    return 1;
  }
  g_use_safe_handler = true;

  struct sigaction sa;
  std::memset(&sa, 0, sizeof sa);
  sa.sa_handler = SigioReproSignalHandler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGUSR1, &sa, nullptr) != 0) {
    std::perror("sigaction");
    return 1;
  }

  std::fprintf(stderr,
      "tests_sigio_repro --safe: path-2 MixSound(async_signal_safe) from "
      "SIGUSR1 while main hammers malloc/free (expects clean exit)\n");
  // raise() delivers to this thread between heap ops — stresses the fixed
  // signal path without starving the main loop.
  for (long i = 0; i < iterations; ++i) {
    arctic::SoundMixerSigioReproSetMainThreadInHeap(true);
    const size_t n = 16 + static_cast<size_t>(i % 2048);
    void *p = std::malloc(n);
    if (p) {
      *static_cast<volatile char *>(p) = static_cast<char>(i);
      std::free(p);
    }
    void *q = std::malloc(48);
    void *r = std::malloc(96);
    if (q) {
      std::free(q);
    }
    if (r) {
      std::free(r);
    }
    arctic::SoundMixerSigioReproSetMainThreadInHeap(false);
    if ((i & 7) == 0) {
      raise(SIGUSR1);
    }
  }
  std::fprintf(stderr, "tests_sigio_repro --safe: ok\n");
  return 0;
}

}  // namespace

int main(int argc, char **argv) {
  const bool safe = (argc >= 2 && std::strcmp(argv[1], "--safe") == 0);
  const bool legacy = (argc >= 2 && (
      std::strcmp(argv[1], "--legacy-unsafe") == 0 ||
      std::strcmp(argv[1], "--unsafe") == 0));
  if (!safe && !legacy) {
    std::fprintf(stderr,
        "Usage: %s --legacy-unsafe | --unsafe | --safe\n"
        "  --legacy-unsafe / --unsafe  old MixSound+malloc from SIGUSR1 "
        "(expects abort)\n"
        "  --safe                      path-2 signal-safe MixSound from "
        "SIGUSR1 (expects exit 0)\n",
        argv[0]);
    return 1;
  }
  if (safe) {
    return RunSafe(2000000L);
  }
  return RunLegacyUnsafe(5000000L);
}

#endif  // ARCTIC_PLATFORM_PI && !ARCTIC_NO_ALSA
