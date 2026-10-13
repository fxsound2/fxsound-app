#!/bin/bash
set -euo pipefail
if [[ $# != 2 || "$1" != --remove-driver-spike ]]; then
    echo 'Usage: sudo uninstall-driver-spike.sh --remove-driver-spike ABSOLUTE_AUDIO_GUARD' >&2; exit 2
fi
if [[ $(id -u) != 0 ]]; then echo 'Driver removal requires an explicit administrator invocation.' >&2; exit 1; fi
guard=$2
if [[ "$guard" != /* || ! -x "$guard" ]]; then echo 'Provide the inspected absolute audio guard executable.' >&2; exit 2; fi
for process in fxsound-engine fxsound-supervisor; do
    if pgrep -f "(^|/)$process([[:space:]]|$)" >/dev/null; then
        echo 'Coordinate and stop all user engines/supervisors before removal.' >&2; exit 1
    fi
done
"$guard"
driver=/Library/Audio/Plug-Ins/HAL/FxSound.driver
if [[ -L "$driver" ]]; then echo 'Refusing a symlink at the fixed driver path.' >&2; exit 1; fi
if [[ -d "$driver" ]]; then
    identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$driver/Contents/Info.plist")
    if [[ "$identifier" != org.fxsound.virtual-audio ]]; then echo 'Unexpected driver bundle identifier.' >&2; exit 1; fi
    /bin/rm -rf -- "$driver"
fi
if pkgutil --pkg-info org.fxsound.driver.spike >/dev/null 2>&1; then pkgutil --forget org.fxsound.driver.spike; fi
printf '%s\n' 'Driver spike removed. User presets, app and user directories were preserved.'
printf '%s\n' 'Audio service was not restarted; verify stock audio and perform controlled reload if required.'
