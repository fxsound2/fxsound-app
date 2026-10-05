#!/bin/bash
set -euo pipefail
if [[ $# != 4 ]]; then echo 'Usage: coordinate-session.sh ENGINE SOCKET ENGINE_PID AUDIO_GUARD' >&2; exit 2; fi
engine=$1 socket=$2 engine_pid=$3 guard=$4
if [[ $(id -u) == 0 ]]; then echo 'Run coordination as the engine session owner, not root.' >&2; exit 1; fi
case "$engine_pid" in ''|*[!0-9]*) echo 'Invalid engine PID.' >&2; exit 2;; esac
if pgrep -f '(^|/)fxsound-supervisor([[:space:]]|$)' >/dev/null; then
    echo 'Disable supervisor registration and stop the supervisor before coordinating engine removal.' >&2; exit 1
fi
owner=$(ps -p "$engine_pid" -o uid= | tr -d ' ')
command=$(ps -p "$engine_pid" -o comm=)
if [[ "$owner" != "$(id -u)" || "$command" != "$engine" ]]; then echo 'Engine PID owner/path mismatch.' >&2; exit 1; fi
response=$("$engine" --request "$socket" '{"version":1,"command":"activateRouting","value":false}')
case "$response" in *'"ok":true'*) ;; *) echo 'Engine did not acknowledge routing restoration.' >&2; exit 1;; esac
"$guard" --defaults-only
kill -TERM "$engine_pid"
for attempt in {1..10}; do
    if ! kill -0 "$engine_pid" 2>/dev/null; then "$guard"; exit 0; fi
    sleep 1
done
echo 'Engine did not stop; privileged removal must remain blocked.' >&2
exit 1
