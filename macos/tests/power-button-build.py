#!/usr/bin/env python3
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
build = Path(sys.argv[1] if len(sys.argv) > 1 else '/private/tmp/fxsound-personal-gui-8-build/gui').resolve()
fixture = Path('/private/tmp/fxsound-power-button-tests/FxSound.app/Contents/MacOS')
fixture.mkdir(parents=True, exist_ok=True)
values = {}
for line in (build / 'CMakeFiles/FxSound.dir/flags.make').read_text().splitlines():
    if ' = ' in line:
        name, value = line.split(' = ', 1)
        values[name] = shlex.split(value)
obj = fixture / 'power-button-test.o'
subprocess.run(['/usr/bin/c++', *values['CXX_DEFINES'], *values['CXX_INCLUDES'],
                *values['CXX_FLAGS'], '-c', str(root / 'macos/tests/power-button-test.cpp'),
                '-o', str(obj)], check=True)
helper = fixture / 'power-audio-discovery.o'
subprocess.run(['/usr/bin/c++', '-std=c++20', '-arch', 'arm64',
                '-mmacosx-version-min=14.0', '-c',
                str(root / 'macos/tests/preset-audio-discovery.cpp'), '-o', str(helper)], check=True)
link = shlex.split((build / 'CMakeFiles/FxSound.dir/link.txt').read_text())
link = [str(obj) if item == 'CMakeFiles/FxSound.dir/app.cpp.o' else item for item in link]
link.insert(link.index('-o'), str(helper))
link[link.index('-o') + 1] = str(fixture / 'power-button-test')
subprocess.run(link, cwd=build, check=True)
subprocess.run(['codesign', '--sign', '-', '--timestamp=none',
                str(fixture / 'power-button-test')], check=True)
subprocess.run([str(fixture / 'power-button-test')], check=True)
