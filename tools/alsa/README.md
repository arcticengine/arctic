# ALSA null PCM for headless Linux sound tests

A CI container usually has no sound card and cannot load kernel modules
(`snd-dummy` is not available). Arctic Engine's `StartSoundMixer` opens ALSA
device `"default"` (stereo S16 @ 44100 Hz). Pointing `"default"` at ALSA's
userspace **null** plugin lets the mixer start there too.

How the Linux mixer runs: when ALSA supports it, `StartSoundMixer` registers
`snd_async_add_pcm_handler` and mixes from the SIGIO handler, which only
touches preallocated memory and atomics. If the handler cannot be registered
(`-ENOSYS`), it mixes on a dedicated thread instead. The null plugin returns
`-ENOSYS`, so with it the dedicated thread is what runs.

## One-time setup

```bash
# Optional packages (aplay / speaker-test / extra plugins)
sudo apt-get install -y alsa-utils libasound2-plugins

# Install ~/.asoundrc
./tools/alsa/setup_null_pcm.sh
# or: cp tools/alsa/asoundrc ~/.asoundrc
```

## Verify

```bash
aplay -L | grep -E '^(default|null)$'
speaker-test -D default -c 2 -r 44100 -t sine -l 1
```

## Run sound regression tests

```bash
cd tests
xvfb-run ./tests --verbose=3 Sound
```

The suite runs with a hidden window, which opens the sound device; `xvfb-run`
is only needed when there is no X display. Do not set
`ARCTIC_HEADLESS=1 ARCTIC_DISABLE_HW=1`: that is the no-window mode, which
opens no sound device, and every live-mixer check is skipped.

In the verbose output, "Sound mixer ALSA async SIGIO path is signal-safe"
reports which mixer runs: `dedicated thread fallback (snd_async_add_pcm_handler
ENOSYS)` on the null PCM, `async PCM handler registered (SIGIO mix)` on real
hardware. `skipped runtime mixer check` means the mixer did not start.

## Notes

- `ctl.!default` is intentionally omitted (no hardware card).
- Named alias `arctic_null` is also defined if a caller opens that string.

## SIGIO heap-race harness

`tests_sigio_repro` is a separate binary, not part of `./tests`. It exists
because the SIGIO handler once ran `MixSound` and `SoundCheck`, which
allocate, while the main thread could be inside malloc/free.

```bash
make -C tests tests_sigio_repro
./tools/alsa/run_sigio_heap_repro.sh
# or:
./tests/tests_sigio_repro --safe            # must exit 0
./tests/tests_sigio_repro --legacy-unsafe   # must abort / print REPRODUCED
```

`--safe` calls the signal-safe `MixSound` from a signal handler while the main
thread allocates and frees; it must exit cleanly. Its mixer has an empty task
queue and no playing sounds, so it does not exercise the task queue or the
Vorbis decoder.

`--legacy-unsafe` runs the old shape (`MixSound` plus a malloc) from a signal
handler and aborts as soon as a signal lands while the main thread is marked
as being inside malloc/free. It shows that such an overlap happens, not that
the heap got corrupted.

`ARCTIC_TEST_SIGIO_REPRO` enables the test-only hooks in
`engine/arctic_platform_pi_sound.cpp` for this binary only.
