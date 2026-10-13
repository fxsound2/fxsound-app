#!/usr/bin/env python3
import gzip
import hashlib
from io import BytesIO
from pathlib import Path
import sys
import tarfile

root = Path(__file__).resolve().parents[2]
output = Path(sys.argv[1]).resolve()
juce = Path(sys.argv[2]).resolve()
output.mkdir(parents=True, exist_ok=True)
extensions = {'.c', '.cpp', '.h', '.hpp', '.mm', '.m', '.inl', '.inc', '.txt', '.md',
              '.cmake', '.plist', '.py', '.sh', '.command', '.jucer', '.xml', '.json',
              '.png', '.jpg', '.jpeg', '.svg', '.ico', '.icns', '.ttf', '.otf', '.fac',
              '.r', '.rc', '.in', '.metal', '.metalh', '.manifest', '.strings'}
sources = [(root / 'LICENSE', 'LICENSE'), (root / 'README.md', 'README.md')]
def collect(directory, prefix):
    for path in sorted(directory.rglob('*')):
        relative = path.relative_to(directory)
        if path.name.startswith('.env') or path.name.lower().startswith('credentials') or path.suffix in {'.pem', '.key', '.p12'}:
            continue
        if any(part.startswith('.') or part in {'build', 'Builds', 'node_modules', '__pycache__'}
               or part.endswith(('.app', '.driver')) for part in relative.parts):
            continue
        if path.is_symlink():
            raise SystemExit(f'Source snapshot does not accept symlinks: {prefix}/{relative}')
        if path.is_file() and (path.suffix in extensions or path.name in {'CMakeLists.txt', 'LICENSE', 'COPYING', 'preinstall', 'postinstall', 'personal-postinstall'}):
            sources.append((path, f'{prefix}/{relative.as_posix()}'))
for folder in ['dsp', 'fxsound', 'macos', 'audiopassthru', 'Resources', 'Installer/Resources/Factsoft']:
    collect(root / folder, folder)
for folder in ['modules', 'extras/Build', 'extras/Tools']:
    collect(juce / folder, f'JUCE/{folder}')
for filename in ['CMakeLists.txt', 'LICENSE.md', 'README.md']:
    sources.append((juce / filename, f'JUCE/{filename}'))
manifest = []
with (output / 'fxsound-personal-source.tar.gz').open('wb') as raw:
    with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as zipped:
        with tarfile.open(fileobj=zipped, mode='w') as archive:
            for path, name in sorted(sources, key=lambda item: item[1]):
                if not path.is_file():
                    raise SystemExit(f'Missing required source: {name}')
                content = path.read_bytes()
                manifest.append(f'{hashlib.sha256(content).hexdigest()}  {name}')
                entry = tarfile.TarInfo(name)
                entry.size = len(content)
                entry.mode = 0o755 if path.suffix in {'.sh', '.command'} else 0o644
                entry.uid = entry.gid = entry.mtime = 0
                archive.addfile(entry, BytesIO(content))
(output / 'SOURCE_SHA256SUMS').write_text('\n'.join(manifest) + '\n')
(output / 'LICENSES.txt').write_text(
    'FxSound app and DSP: LICENSE (AGPL-3.0).\n'
    'HAL driver scaffolding: macos/driver/apple-sample-license.txt.\n'
    'JUCE 8.0.15: JUCE/LICENSE.md and bundled module third-party notices.\n'
    'JUCE pinned commit: 91ad83ae34a81e0833b1a2b0866f54846370ae53.\n'
    'FxSound resource submodule: 8f0385f015cd6b10026b2db6a0625a6db4de89b0.\n'
    'Original resource files retain their bundled notices and ownership.\n')
print(f'Matching source snapshot: {len(sources)} files')
