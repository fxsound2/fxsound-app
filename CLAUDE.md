# CLAUDE.md

Guidance for Claude when working in this repository.

## Project overview

FxSound is a digital audio enhancement application for Windows. It acts as a
system-wide virtual soundcard, providing clean audio passthrough plus DSP-based
effects for volume, timbre, and equalization.

## Architecture

The application has three components, each with a different risk profile:

1. **FxSound GUI** (`fxsound/`) — the JUCE-based application UI. Most
   contributions land here. Bugs are usually cosmetic, UX-related, or
   preset/settings handling.
2. **Audiopassthru** (`audiopassthru/`) — interfaces with the FxSound Audio
   Enhancer virtual audio driver. **Treat changes here with extra scrutiny** —
   bugs can affect system audio for all users, not just this app, and issues
   here are harder for users to self-diagnose or roll back.
3. **DfxDsp** (`dsp/`) — the DSP engine that processes audio in real time.
   Changes here affect audio quality and performance directly; watch for
   real-time-safety issues (allocations, locks, or blocking calls in the audio
   callback path) and numerical correctness in filter/effect code.

## Build environment

- Visual Studio 2022, Windows SDK
- JUCE 6.1.6 for x64/x86; latest JUCE framework build for ARM64
- Solution: `fxsound/Project/FxSound.sln`
- Running the app locally requires FxSound's virtual audio driver to already
  be installed (via the normal FxSound installer), since the driver isn't
  built from this source tree.
- Note: when a solution is exported fresh from Projucer, `audiopassthru` and
  `DfxDsp` project references may need to be re-added manually — this is a
  known Projucer export quirk, not a code defect, if you see it in a PR.

## PR review priorities

When reviewing pull requests:
- Flag any change to `audiopassthru/` or `dsp/` explicitly as higher-risk,
  even if the diff looks small.
- In `dsp/`, check for anything that could block or allocate on the real-time
  audio thread.
- In `audiopassthru/`, check for driver-interface assumptions that might not
  hold across Windows versions.
- Standard C++ concerns apply otherwise: resource leaks, undefined behavior,
  thread safety, off-by-one errors.
- Don't flag pure formatting/style differences unless they violate patterns
  already established nearby in the same file.

## Issue triage

When triaging an issue, report (don't apply labels unless explicitly asked to
in a separate `@claude apply labels` step):
- **Type**: bug, feature request, question, or build/installation issue.
- **Priority**: Urgent/High/Medium/Low, based on severity and how many users
  are likely affected — but note this is set on the GitHub Projects board's
  Priority field, not a label, so state your suggestion in the comment rather
  than assuming it can be applied automatically.
- **Related/duplicate issues**: search both open and closed issues.
- **Fixed status**: check recent commits on `main` and closed issues/PRs to
  see if this may already be fixed but not yet released.

## Labels

Only apply labels that already exist in the repository. Don't create new
labels. If no suitable label exists for something you'd normally tag, say so
in your comment instead of guessing.

## What not to do

- Don't merge, close, or assign issues/PRs yourself.
- Don't modify the GitHub Projects Priority field unless a workflow has been
  explicitly given permission and instruction to do so.
- Don't make changes to `audiopassthru/` or `dsp/` in an automated fix without
  flagging it clearly for human review before merge.
