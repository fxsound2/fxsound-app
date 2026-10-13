#!/usr/bin/env python3
import gzip
import hashlib
from pathlib import Path
import sys
import tarfile

root = Path(__file__).resolve().parents[2]
output = Path(sys.argv[1]).resolve()
if any(output==root / d or root / d in output.parents for d in ['macos/driver', 'macos/packaging']):
    raise SystemExit('Source archive output must be outside the selected source directories.')
output.mkdir(parents=True, exist_ok=True)
files = [root / 'LICENSE']
allowed = {'.c', '.h', '.sh', '.plist', '.md', '.txt', '.py'}
for directory in ['macos/driver', 'macos/packaging']:
    for path in (root / directory).rglob('*'):
        if any(part.startswith('.') or part.endswith(('.driver', '.app')) for part in path.relative_to(root).parts) or path.name.lower().startswith('credentials'):
            continue
        if path.is_symlink():
            raise SystemExit(f'Symlinks are excluded from source snapshot: {path.relative_to(root)}')
        if path.is_file() and path.suffix in allowed:
            files.append(path)
files.sort(key=lambda p: p.relative_to(root).as_posix())
archive = output / 'fxsound-driver-spike-source.tar.gz'
manifest = []
with archive.open('wb') as raw:
    with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as zipped:
        with tarfile.open(fileobj=zipped, mode='w') as tar:
            for path in files:
                name = path.relative_to(root).as_posix()
                data = path.read_bytes()
                manifest.append(f'{hashlib.sha256(data).hexdigest()}  {name}')
                entry = tarfile.TarInfo(name)
                entry.size, entry.uid, entry.gid, entry.mtime = len(data), 0, 0, 0
                entry.mode = 0o755 if path.suffix == '.sh' else 0o644
                from io import BytesIO
                tar.addfile(entry, BytesIO(data))
(output / 'SOURCE_SHA256SUMS').write_text('\n'.join(manifest) + '\n')
(output / 'LICENSES.txt').write_text('Project sources: LICENSE (AGPL-3.0).\n'
    'Apple-derived HAL scaffolding: macos/driver/apple-sample-license.txt.\n'
    'Driver-only spike excludes JUCE, DSP, GUI and optional MCP dependencies.\n')
print(f'Source archive: {archive}; selected files: {len(files)}')
