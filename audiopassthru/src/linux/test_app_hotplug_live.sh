#!/usr/bin/env bash
# FxSound — Linux build: live test, hot-plugged devices reach the output menu (ctest label pw-live)
#
# With power on, adding a sink must add it to the tray's output menu and
# removing it must take it away again. Runs the app under Xvfb with an
# isolated config; uses the StatusNotifierItem dbusmenu. Exit 77 = skipped.
set -u
APP=$1

for t in pactl xvfb-run gdbus; do command -v $t >/dev/null || { echo "$t missing"; exit 77; }; done
ORIG=$(pactl get-default-sink 2>/dev/null) || { echo "no PipeWire/pulse server"; exit 77; }

CFG=$(mktemp -d)
MOD=
pid=
cleanup() {
    [ -n "$pid" ] && kill -TERM "$pid" 2>/dev/null && sleep 1
    [ -n "$MOD" ] && pactl unload-module "$MOD"
    pactl set-default-sink "$ORIG"
    rm -rf "$CFG"
}
trap cleanup EXIT

XDG_CONFIG_HOME=$CFG xvfb-run -a "$APP" >/dev/null 2>&1 &
for _ in $(seq 100); do pid=$(pgrep -n -x "$(basename "$APP")") && break; sleep 0.1; done
bus=
for _ in $(seq 100); do
    # the tray registers org.kde.StatusNotifierItem:<pid>:<n>
    bus=$(gdbus call --session -d org.freedesktop.DBus -o /org/freedesktop/DBus \
            -m org.freedesktop.DBus.ListNames | grep -o "org.kde.StatusNotifierItem[-:]$pid[-:][0-9]*" | head -1)
    [ -n "$bus" ] && break; sleep 0.1
done
[ -n "$bus" ] || { echo "tray not on the session bus"; exit 77; }
menu() { gdbus call --session -d "$bus" -o /com/fxsound/MenuBar -m com.canonical.dbusmenu.GetLayout -- 0 -1 '[]'; }
in_menu() { menu | grep -q "FxHotplugTest"; }
sleep 2

MOD=$(pactl load-module module-null-sink sink_name=fxsound_hotplug_test sink_properties=device.description=FxHotplugTest)
for _ in $(seq 50); do in_menu && break; sleep 0.1; done
in_menu && added=yes || added=no
pactl unload-module "$MOD"; MOD=
for _ in $(seq 50); do in_menu || break; sleep 0.1; done
in_menu && removed=no || removed=yes
echo "added to menu: $added, removed from menu: $removed"
[ $added = yes ] && [ $removed = yes ] && { echo "APP HOTPLUG LIVE: PASS"; exit 0; }
echo "APP HOTPLUG LIVE: FAIL"; exit 1
