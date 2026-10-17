#!/usr/bin/env bash
# FxSound — Linux build: live PipeWire routing test (ctest label pw-live)
#
# With no device selected, the processor must follow the user's default sink
# rather than whichever sink has the lowest node id. Creates a silent null
# sink, makes it the default, runs passthru_selftest and checks the links.
# Restores the original default sink on exit. Exit 77 = skipped.
set -u
SELFTEST=$1

command -v pactl >/dev/null && command -v pw-link >/dev/null || { echo "pactl/pw-link missing"; exit 77; }
ORIG=$(pactl get-default-sink 2>/dev/null) || { echo "no PipeWire/pulse server"; exit 77; }

MOD=$(pactl load-module module-null-sink sink_name=fxsound_routing_test) || exit 77
pid=
cleanup() {
    [ -n "$pid" ] && kill "$pid" 2>/dev/null && wait "$pid" 2>/dev/null
    pactl set-default-sink "$ORIG"
    pactl unload-module "$MOD"
}
trap cleanup EXIT
pactl set-default-sink fxsound_routing_test

"$SELFTEST" 4 >/dev/null & pid=$!
for _ in $(seq 30); do
    pw-link -l | grep -q '^FxSound Processor:output_FL' && break
    sleep 0.1
done
sleep 0.5
links=$(pw-link -l | grep -A1 '^FxSound Processor:output_FL')
echo "$links"
if echo "$links" | grep -q 'fxsound_routing_test:playback_FL'; then
    echo "ROUTING LIVE: PASS"; exit 0
fi
echo "ROUTING LIVE: FAIL (processor not linked to the default sink)"; exit 1
