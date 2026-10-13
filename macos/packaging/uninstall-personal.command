#!/bin/bash
set -euo pipefail
if pgrep -f '(^|/)fxsound-(engine|supervisor)([[:space:]]|$)' >/dev/null; then
    printf '%s\n' 'Quit FxSound before uninstalling.'
    exit 1
fi
guard=/Applications/FxSound.app/Contents/Helpers/audio-install-guard
if [[ ! -x "$guard" ]]; then
    printf '%s\n' 'The installed safety helper is missing. Uninstall refused.'
    exit 1
fi
"$guard"
/usr/bin/osascript <<'APPLESCRIPT'
do shell script "/Applications/FxSound.app/Contents/Helpers/audio-install-guard && /bin/rm -rf /Library/Audio/Plug-Ins/HAL/FxSound.driver /Applications/FxSound.app && /usr/sbin/pkgutil --forget org.fxsound.personal && if /usr/bin/pgrep -x coreaudiod >/dev/null; then /usr/bin/killall coreaudiod; fi" with administrator privileges
APPLESCRIPT
