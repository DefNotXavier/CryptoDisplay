#!/bin/bash
# Runs from the root of the CRYPTOS drive, wherever it got mounted.
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INDEX="file://$DIR/dashboard/index.html"

# Watch the block device, not the mount point: after a surprise unplug the
# mount entry can linger, but udev removes the device node right away.
SRC_DEV="$(findmnt -n -o SOURCE --target "$DIR" 2>/dev/null)"
if [ -n "$SRC_DEV" ]; then
  still_mounted() { [ -e "$SRC_DEV" ]; }
else
  still_mounted() { mountpoint -q "$DIR"; }
fi

BROWSER_BIN=""
for BROWSER in chromium-browser chromium google-chrome microsoft-edge; do
  if command -v "$BROWSER" >/dev/null 2>&1; then
    BROWSER_BIN="$BROWSER"
    break
  fi
done

if [ -z "$BROWSER_BIN" ]; then
  echo "No supported browser found (tried chromium-browser, chromium, google-chrome, microsoft-edge)."
  exit 1
fi

# Per-user runtime dir: a leftover from a sudo run can't block it the way a
# root-owned folder in /tmp would.
RUN_DIR="${XDG_RUNTIME_DIR:-/tmp}"
PROFILE_DIR="$RUN_DIR/crypto-kiosk"
LOG="$RUN_DIR/crypto-kiosk.log"

kiosk_running() { pgrep -f "user-data-dir=$PROFILE_DIR" >/dev/null 2>&1; }

# Close a kiosk left over from an earlier session and start from a clean
# profile; a stale or crashed one can make Chromium abort on startup.
pkill -f "user-data-dir=$PROFILE_DIR" 2>/dev/null
sleep 1
rm -rf "$PROFILE_DIR"

# Separate profile: forces a new, trackable browser process.
# --password-store=basic: skips the "unlock your keyring" prompt.
start_browser() {
  echo "=== $(date) starting $BROWSER_BIN $*" >>"$LOG"
  "$BROWSER_BIN" --kiosk --no-first-run --allow-file-access-from-files --password-store=basic \
    --user-data-dir="$PROFILE_DIR" "$@" "$INDEX" >>"$LOG" 2>&1 &
  BROWSER_PID=$!
}

# If it dies within 10s, retry once without GPU acceleration: the usual
# cause of Chromium crashing at startup on a Pi.
start_browser
for _ in $(seq 1 10); do
  sleep 1
  kill -0 "$BROWSER_PID" 2>/dev/null || kiosk_running || break
done
if ! kill -0 "$BROWSER_PID" 2>/dev/null && ! kiosk_running; then
  start_browser --disable-gpu
fi

# Close the browser once the drive is unplugged.
while still_mounted; do
  sleep 2
  # Match on the profile dir too: on Raspberry Pi OS $! is a wrapper script's PID.
  if ! kill -0 "$BROWSER_PID" 2>/dev/null && ! kiosk_running; then
    exit 0
  fi
done

kill "$BROWSER_PID" 2>/dev/null
pkill -f "user-data-dir=$PROFILE_DIR" 2>/dev/null
