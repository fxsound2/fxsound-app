#!/usr/bin/env bash
# FxSound — Linux build: live test, app exit restores the default sink (ctest label pw-live)
#
# Logout and shutdown send SIGTERM. The app must hand the user's configured
# default sink back on exit instead of leaving it set to "FxSound". Runs the
# app under Xvfb with an isolated config, against a silent null sink made the
# default. Restores the original default on exit. Exit 77 = skipped.
set -u
APP=$1; SIG=${2:-TERM}

for t in pactl pw-metadata xvfb-run; do command -v $t >/dev/null || { echo "$t missing"; exit 77; }; done
ORIG=$(pactl get-default-sink 2>/dev/null) || { echo "no PipeWire/pulse server"; exit 77; }
configured() { pw-metadata 0 default.configured.audio.sink | grep -o '"name": *"[^"]*"' | cut -d'"' -f4; }

CFG=$(mktemp -d)
MOD=$(pactl load-module module-null-sink sink_name=fxsound_exit_test) || exit 77
pid=
cleanup() {
    [ -n "$pid" ] && kill -KILL "$pid" 2>/dev/null
    pactl set-default-sink "$ORIG"
    pactl unload-module "$MOD"
    rm -rf "$CFG"
}
trap cleanup EXIT
pactl set-default-sink fxsound_exit_test

XDG_CONFIG_HOME=$CFG xvfb-run -a "$APP" >/dev/null 2>&1 &
for _ in $(seq 100); do [ "$(configured)" = FxSound ] && break; sleep 0.1; done
[ "$(configured)" = FxSound ] || { echo "app did not become the default"; exit 1; }
sleep 1
pid=$(pgrep -n -x "$(basename "$APP")")
kill -$SIG "$pid"
for _ in $(seq 100); do kill -0 "$pid" 2>/dev/null || break; sleep 0.1; done
kill -0 "$pid" 2>/dev/null && { echo "app still running 10 s after SIG$SIG"; exit 1; }
pid=
got=$(configured)
echo "configured default after SIG$SIG: $got"
[ "$got" = fxsound_exit_test ] && { echo "APP EXIT LIVE: PASS"; exit 0; }
echo "APP EXIT LIVE: FAIL (expected fxsound_exit_test)"; exit 1
