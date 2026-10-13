#!/bin/bash
set -euo pipefail
if [[ $# != 4 ]]; then echo 'Usage: sign-notarize-package.sh INSTALLER_IDENTITY NOTARY_PROFILE UNSIGNED_PKG SIGNED_OUTPUT_PKG' >&2; exit 2; fi
identity=$1 profile=$2 input=$3 output=$4
if [[ "$identity" != 'Developer ID Installer:'* || -z "$profile" || "$input" == "$output" ]]; then
    echo 'Explicit Developer ID Installer identity, profile name and distinct output path required.' >&2; exit 2
fi
productsign --sign "$identity" --timestamp "$input" "$output"
pkgutil --check-signature "$output"
result=$(mktemp /private/tmp/fxsound-notary-result.XXXXXX)
xcrun notarytool submit "$output" --keychain-profile "$profile" --wait --output-format plist > "$result"
status=$(/usr/libexec/PlistBuddy -c 'Print :status' "$result")
if [[ "$status" != Accepted ]]; then echo 'Notarization was not Accepted; inspect the local result.' >&2; exit 1; fi
xcrun stapler staple "$output"
xcrun stapler validate "$output"
spctl --assess --type install --verbose=2 "$output"
shasum -a 256 "$output"
