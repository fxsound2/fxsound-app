#!/usr/bin/env python3
from pathlib import Path
import shlex
import subprocess
import sys
root=Path(__file__).resolve().parents[2]
build=Path(sys.argv[1] if len(sys.argv)>1 else '/private/tmp/fxsound-personal-gui-8-build/gui')
installed=Path(sys.argv[2] if len(sys.argv)>2 else '/Applications/FxSound.app')
fixture=Path('/private/tmp/fxsound-preset-tests/FxSound.app/Contents')
(fixture/'MacOS').mkdir(parents=True,exist_ok=True)
resource=fixture/'Resources'
if resource.is_symlink():resource.unlink()
resource.symlink_to(installed/'Contents/Resources',target_is_directory=True)
values={}
for line in (build/'CMakeFiles/FxSound.dir/flags.make').read_text().splitlines():
 if ' = ' in line:
  name,value=line.split(' = ',1);values[name]=shlex.split(value)
obj=fixture/'MacOS/preset-dropdown-test.o'
command=['/usr/bin/c++',*values['CXX_DEFINES'],*values['CXX_INCLUDES'],*values['CXX_FLAGS'],'-c',str(root/'macos/tests/preset-dropdown-test.cpp'),'-o',str(obj)]
subprocess.run(command,check=True)
helper=fixture/'MacOS/preset-audio-discovery.o'
subprocess.run(['/usr/bin/c++','-std=c++20','-arch','arm64','-mmacosx-version-min=14.0','-c',str(root/'macos/tests/preset-audio-discovery.cpp'),'-o',str(helper)],check=True)
link=shlex.split((build/'CMakeFiles/FxSound.dir/link.txt').read_text())
link=[str(obj) if x=='CMakeFiles/FxSound.dir/app.cpp.o' else x for x in link]
link.insert(link.index('-o'),str(helper))
link[link.index('-o')+1]=str(fixture/'MacOS/preset-dropdown-test')
subprocess.run(link,cwd=build,check=True)
subprocess.run(['codesign','--sign','-','--timestamp=none',str(fixture/'MacOS/preset-dropdown-test')],check=True)
subprocess.run([str(fixture/'MacOS/preset-dropdown-test')],check=True)
