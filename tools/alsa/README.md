# ALSA null PCM for headless Linux sound tests

This box (and many CI containers) has no sound card and no kernel modules
(`snd-dummy` cannot be loaded). Arctic Engine's `StartSoundMixer` opens ALSA
device `"default"` (stereo S16 @ 44100 Hz). Pointing `"default"` at ALSA's
userspace **null** plugin lets the dedicated mixer thread start so
`SoundMixerIsDedicatedThreadRunning` is exercised.

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
ARCTIC_HEADLESS=1 ARCTIC_DISABLE_HW=1 ./tests -E --verbose=3 Sound
```

Success for the SIGIO regression: verbose output shows
`successful start must use the dedicated mixer thread... ok`
(not `skipped runtime thread check`).

## Notes

- No production mixer code changes are required.
- `ctl.!default` is intentionally omitted (no hardware card).
- Named alias `arctic_null` is also defined if a caller opens that string.

## SIGIO heap-race reproduction

Production always mixes on a dedicated thread. To **reproduce** the old
SIGIO hazard (MixSound / SoundCheck allocating from a signal handler while
the main thread runs malloc/free):

```bash
# from repo root, after cmake in tests/
make -C tests tests_sigio_repro
./tools/alsa/run_sigio_heap_repro.sh
```

Or manually:

```bash
./tests/tests_sigio_repro --safe     # must exit 0
./tests/tests_sigio_repro --unsafe   # must abort / print REPRODUCED
```

`ARCTIC_TEST_SIGIO_REPRO` enables test-only hooks in
`engine/arctic_platform_pi_sound.cpp`. The default `./tests` suite stays green
and does not register an ALSA async handler.

## SIGIO heap-race reproduction (path 2)

Production keeps `snd_async_add_pcm_handler` (SIGIO) when ALSA supports it.
The handler mixes with preallocated buffers only (no malloc / SoundCheck).

```bash
make -C tests tests_sigio_repro
./tools/alsa/run_sigio_heap_repro.sh
# or:
./tests/tests_sigio_repro --safe            # path-2 safe signal mix → exit 0
./tests/tests_sigio_repro --legacy-unsafe   # old hazard → abort / REPRODUCED
```

On this box, ALSA `null` returns `-ENOSYS` for `snd_async_add_pcm_handler`, so `StartSoundMixer` uses the dedicated-thread fallback. The path-2 signal-safe `MixSound` is still validated by `tests_sigio_repro --safe` (MixSound from SIGUSR1 under malloc stress).
