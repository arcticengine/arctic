#!/usr/bin/env bash
# Path 2 SIGIO harness:
#   --legacy-unsafe → must REPRODUCE / abort (old heap-from-signal hazard)
#   --safe          → must PASS (signal-safe MixSound under malloc stress)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TESTS_DIR="${ROOT}/tests"
BIN="${TESTS_DIR}/tests_sigio_repro"

if [[ ! -x "${BIN}" ]]; then
  echo "Building tests_sigio_repro..."
  (cd "${TESTS_DIR}" && cmake . >/dev/null && make -j"$(nproc)" tests_sigio_repro)
fi

if [[ ! -x "${BIN}" ]]; then
  echo "error: ${BIN} not found (Linux ALSA build required)" >&2
  exit 1
fi

echo "=== --safe (path-2 signal-safe MixSound; must PASS) ==="
"${BIN}" --safe
echo "safe: PASS"

echo "=== --legacy-unsafe (old MixSound+malloc from signal; must REPRODUCE) ==="
set +e
reproduced=0
for attempt in 1 2 3 4 5 6 7 8; do
  echo "legacy-unsafe attempt ${attempt}..."
  out="$(mktemp)"
  timeout 20 "${BIN}" --legacy-unsafe >"${out}" 2>&1
  ec=$?
  tail -n 20 "${out}" || true
  if grep -q 'REPRODUCED:' "${out}"; then
    reproduced=1
    rm -f "${out}"
    break
  fi
  if [[ "${ec}" -ge 128 ]] || [[ "${ec}" -eq 134 ]] || [[ "${ec}" -eq 6 ]]; then
    reproduced=1
    rm -f "${out}"
    break
  fi
  rm -f "${out}"
done
set -e

if [[ "${reproduced}" -ne 1 ]]; then
  echo "error: --legacy-unsafe did not reproduce the SIGIO heap hazard" >&2
  exit 1
fi
echo "legacy-unsafe: REPRODUCED (old hazard demonstrated)"
echo "All SIGIO path-2 checks completed."
