#!/bin/bash
set -euo pipefail
if [[ $# < 2 || $# > 3 ]]; then echo 'Usage: sign-bundles.sh APPLICATION_IDENTITY DRIVER_BUNDLE [APP_BUNDLE]' >&2; exit 2; fi
identity=$1 driver=$2
if [[ "$identity" != 'Developer ID Application:'* ]]; then echo 'An explicit Developer ID Application identity is required.' >&2; exit 2; fi
for bundle in "$driver" "${3:-}"; do
    [[ -n "$bundle" ]] || continue
    if [[ ! -d "$bundle/Contents" ]]; then echo 'Expected a complete bundle.' >&2; exit 2; fi
    while IFS= read -r -d '' binary; do
        if file -b "$binary" | grep -q 'Mach-O'; then
            codesign --force --sign "$identity" --timestamp --options runtime "$binary"
        fi
    done < <(find "$bundle/Contents" -type f -print0)
    while IFS= read -r -d '' nested; do
        codesign --force --sign "$identity" --timestamp --options runtime "$nested"
    done < <(find "$bundle/Contents" -depth -type d \( -name '*.framework' -o -name '*.bundle' -o -name '*.app' -o -name '*.xpc' \) -print0)
    codesign --force --sign "$identity" --timestamp --options runtime "$bundle"
    codesign --verify --deep --strict --verbose=2 "$bundle"
done
