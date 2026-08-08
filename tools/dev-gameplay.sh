#!/usr/bin/env bash

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
BINARY_PATH="$BUILD_DIR/duppy"
POLL_INTERVAL="${DEV_POLL_INTERVAL:-1}"
GAME_PID=""
GAME_EXITED=0

if [ "$#" -gt 0 ]; then
  GAME_ARGS=("$@")
else
  GAME_ARGS=(gameplay --fps 144 --uncapped --dev --debug-world)
fi

source_snapshot() {
  (
    cd "$ROOT_DIR" || exit 1
    {
      printf '%s\0' "CMakeLists.txt"
      find src -type f \
        \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.inl' \) \
        -print0
    } | xargs -0 stat -f '%m %N' 2>/dev/null | sort
  )
}

rebuild() {
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" && cmake --build "$BUILD_DIR"
}

launch_game() {
  "$BINARY_PATH" "${GAME_ARGS[@]}" &
  GAME_PID="$!"
  GAME_EXITED=0
  echo "[dev-run] launched pid=$GAME_PID: $BINARY_PATH ${GAME_ARGS[*]}"
}

stop_game() {
  if [ -n "$GAME_PID" ] && kill -0 "$GAME_PID" 2>/dev/null; then
    kill "$GAME_PID" 2>/dev/null || true
    wait "$GAME_PID" 2>/dev/null || true
  fi
  GAME_PID=""
}

cleanup() {
  stop_game
}

trap cleanup EXIT INT TERM

LAST_SNAPSHOT="$(source_snapshot)"

echo "[dev-run] initial configure/build..."
if ! rebuild; then
  echo "[dev-run] initial build failed"
  exit 1
fi

launch_game

while true; do
  sleep "$POLL_INTERVAL"

  if [ -n "$GAME_PID" ] && ! kill -0 "$GAME_PID" 2>/dev/null; then
    wait "$GAME_PID" 2>/dev/null || true
    GAME_PID=""
    if [ "$GAME_EXITED" -eq 0 ]; then
      echo "[dev-run] game exited; waiting for the next successful rebuild to relaunch"
      GAME_EXITED=1
    fi
  fi

  CURRENT_SNAPSHOT="$(source_snapshot)"
  if [ "$CURRENT_SNAPSHOT" = "$LAST_SNAPSHOT" ]; then
    continue
  fi

  LAST_SNAPSHOT="$CURRENT_SNAPSHOT"
  echo "[dev-run] source change detected; rebuilding..."
  if rebuild; then
    stop_game
    launch_game
  else
    echo "[dev-run] build failed; fix the compile error and save again to retry"
  fi
done
