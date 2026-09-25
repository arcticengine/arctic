#!/usr/bin/env bash
# Install ALSA userspace null PCM as "default" for headless Arctic Engine sound.
# Uses $HOME/.asoundrc (this environment may set HOME=/tmp).
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="${SCRIPT_DIR}/asoundrc"
DEST="${HOME}/.asoundrc"
if [[ -f "${DEST}" ]] && ! grep -q 'arctic_null\|slave.pcm "null"' "${DEST}" 2>/dev/null; then
  cp -a "${DEST}" "${DEST}.bak.$(date +%s)"
  echo "Backed up existing ${DEST}"
fi
cp "${SRC}" "${DEST}"
echo "Installed ${DEST} (default -> null PCM plugin)"
command -v aplay >/dev/null 2>&1 || {
  echo "Optional: sudo apt-get install -y alsa-utils libasound2-plugins"
}
aplay -L 2>/dev/null | grep -E '^(default|null|arctic_null)$' || true
