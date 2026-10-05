# Driver-only packaging spike

This directory builds an unsigned development installer for the HAL driver only. It is a validation artifact, not the FxSound product distribution: no GUI, engine, supervisor registration or physical audio routing is installed. The pre-GUI runtime/signing gate remains pending. Installing an unverified HAL plug-in can disrupt system audio; run the install/load/uninstall spike only in a controlled target-system validation session.

## Offline build

1. Build `macos/driver/build.sh`; default bundle is `/private/tmp/fxsound-driver-artifacts/FxSound.driver`.
2. Run `macos/packaging/build-spike.sh DRIVER_BUNDLE OUTPUT_DIRECTORY [APPLICATION_IDENTITY]`.
3. Inspect `OUTPUT_DIRECTORY/FxSound-driver-development-spike.pkg`, `Distribution.xml`, component payload, hashes and matching source archive before any installation.

The build uses real pkgbuild/productbuild. Payload is fixed to `/Library/Audio/Plug-Ins/HAL/FxSound.driver`, receipt `org.fxsound.driver.spike`, version 0.1.0. Product requirements enforce macOS >=14.0 and x86_64/arm64; driver slices are verified independently. The package is unsigned unless the explicit later signing procedure is used. Default driver/helper signing is development ad-hoc and does not establish coreaudiod loading policy.

A read-only Universal 2 `audio-install-guard` is embedded with package scripts. Preinstall rejects offline destination volumes, active engine/supervisor processes, virtual default/system output, unknown device state and active HAL clients. Postinstall does not restart coreaudiod, activate routing, resolve root HOME or install agents for arbitrary users. A controlled audio-service reload is a separate validation-session action. Staging directories are retained for inspection.

## Coordination and removal

Before replacing/removing a loaded driver, every logged-in user must disable supervisor restart/registration, stop its supervisor, and restore physical routing while its engine is still alive. Run as that user:

```sh
macos/packaging/coordinate-session.sh /absolute/fxsound-engine /private/user/runtime/fxsound-engine.sock ENGINE_PID /absolute/audio-install-guard
```

The script checks the process owner/path, sends versioned `activateRouting=false`, verifies physical default/system output while capture is still running, then sends TERM to that engine and verifies it stopped. It refuses root execution and does not force-kill. The final guard check refuses all active HAL clients, so other capturing/playing applications must release the virtual device before privileged removal; the defaults-only check deliberately permits the engine to remain alive during routing restoration. No explicit user-session shutdown/restart interface is invented for an unfinished supervisor.

After all sessions are coordinated, an administrator may explicitly invoke:

```sh
sudo macos/packaging/uninstall-driver-spike.sh --remove-driver-spike /absolute/audio-install-guard
```

The remover rechecks all sessions, validates the fixed bundle identifier, refuses a symlink at the fixed driver path, removes only that bundle and its spike receipt, and preserves user presets/directories. It does not restart audio or remove app/helper/agent registration. The spike does not implement product upgrade/rollback or clean no-login/later-user flows. If the app was deleted and the virtual device remains default, first choose a physical device in Sound Settings/Audio MIDI Setup and release virtual audio clients; the guard remains conservative until both default and system output are physical.

## Explicit distribution signing

Do not enumerate keychains or put certificate passwords, API keys or profile contents in files. Identity/profile names below reference credentials already configured by the operator. Application and Installer identities are separately required; scripts reject ad-hoc identities for distribution signing.

```sh
macos/packaging/sign-bundles.sh "$FX_APPLICATION_IDENTITY" DRIVER_BUNDLE
macos/packaging/build-spike.sh DRIVER_BUNDLE OUTPUT_DIRECTORY "$FX_APPLICATION_IDENTITY"
macos/packaging/sign-notarize-package.sh "$FX_INSTALLER_IDENTITY" "$FX_NOTARY_PROFILE" UNSIGNED_PKG SIGNED_PKG
```

`sign-bundles.sh` also accepts an optional complete app bundle for the future product: signs individual Mach-O helpers/libraries first, nested bundles from deepest to outermost, then the enclosing app/driver. It never uses --deep to sign; recursive strict verification follows signing. No additional entitlements are granted by default; future GUI/helper entitlements require audited task-specific additions.

The package helper must also use the supplied Application identity before pkgbuild. The package signing script uses Developer ID Installer, checks package signature, requires notarization status Accepted using only a supplied profile name, staples/validates the ticket, and assesses Gatekeeper install policy. It does not read/export credential values or enumerate signing identities. A complete app distribution additionally needs exec Gatekeeper assessment, helper/launch registration validation and quarantined clean-install evidence. No Developer ID, notarization or Gatekeeper success is claimed by the development spike.

## Source and verification record

`source-snapshot.py` selects only driver/packaging code, docs and license files plus repository LICENSE, including new local implementation files. Hidden paths, credential filenames, key formats and symlinks are excluded/refused. It produces deterministic gzip/tar metadata, per-file SHA256, an explicit license manifest and package/source checksums. It does not archive the whole repository or environment. Unsigned PKGs contain installer timestamps and are not claimed byte-reproducible; the matching source archive is deterministic for identical selected inputs.

The driver-only archive includes AGPL project LICENSE and retained Apple sample license; it excludes DSP/JUCE/app assets and therefore is not sufficient corresponding source for a future full product. Extend the manifest when those payloads ship.

Shell verification uses bash -n and sh -n when shellcheck is unavailable; syntax checks do not replace semantic security review. Actual supported-runtime clean install/load/uninstall, CoreAudio process health, Developer ID signing, notarization, Gatekeeper, upgrade/rollback and source publication remain pending.
