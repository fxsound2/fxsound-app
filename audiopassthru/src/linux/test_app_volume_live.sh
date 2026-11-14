#!/usr/bin/env bash
# FxSound — Linux build: live test, sink volume reaches the output (ctest label pw-live)
#
# Volume keys change the "FxSound" sink volume. The processor captures the sink
# monitor, so the monitor must follow that volume (monitor.channel-volumes) or
# the keys do nothing. Runs the app under Xvfb with an isolated config, against
# a silent null sink made the default. Restores the original default on exit.
# Exit 77 = skipped.
set -u
APP=$1

for t in pactl pw-metadata pw-dump python3 xvfb-run; do command -v $t >/dev/null || { echo "$t missing"; exit 77; }; done
ORIG=$(pactl get-default-sink 2>/dev/null) || { echo "no PipeWire/pulse server"; exit 77; }
configured() { pw-metadata 0 default.configured.audio.sink | grep -o '"name": *"[^"]*"' | cut -d'"' -f4; }

CFG=$(mktemp -d)
MOD=$(pactl load-module module-null-sink sink_name=fxsound_volume_test) || exit 77
pid=
cleanup() {
    [ -n "$pid" ] && kill -KILL "$pid" 2>/dev/null
    pactl set-default-sink "$ORIG"
    pactl unload-module "$MOD"
    rm -rf "$CFG"
}
trap cleanup EXIT
pactl set-default-sink fxsound_volume_test

XDG_CONFIG_HOME=$CFG xvfb-run -a "$APP" >/dev/null 2>&1 &
for _ in $(seq 100); do [ "$(configured)" = FxSound ] && break; sleep 0.1; done
[ "$(configured)" = FxSound ] || { echo "app did not become the default"; exit 1; }
pid=$(pgrep -n -x "$(basename "$APP")")

# Match by our pid: an installed FxSound may also own a node named "FxSound".
got=$(pw-dump | python3 -c 'import json, sys
objs = [o for o in json.load(sys.stdin) if o.get("info")]
mine = {o["id"] for o in objs if str(o["info"]["props"].get("application.process.id")) == sys.argv[1]}
print([str(p.get("monitor.channel-volumes")).lower() for p in (o["info"]["props"] for o in objs)
       if p.get("node.name") == "FxSound" and p.get("client.id") in mine])' "$pid")
echo "FxSound monitor.channel-volumes: $got"
[ "$got" = "['true']" ] && { echo "APP VOLUME LIVE: PASS"; exit 0; }
echo "APP VOLUME LIVE: FAIL (expected true)"; exit 1
