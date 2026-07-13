\# CLAUDE.md



Guidance for Claude when working in this repository.



\## Project overview



FxSound is a digital audio enhancement application for Windows. It acts as a

system-wide virtual soundcard, providing clean audio passthrough plus DSP-based

effects for volume, timbre, and equalization.



\## Architecture



The application has three components, each with a different risk profile:



1\. \*\*FxSound GUI\*\* (`fxsound/`) — the JUCE-based application UI. Most

&#x20;  contributions land here. Bugs are usually cosmetic, UX-related, or

&#x20;  preset/settings handling.

2\. \*\*Audiopassthru\*\* (`audiopassthru/`) — interfaces with the FxSound Audio

&#x20;  Enhancer virtual audio driver. \*\*Treat changes here with extra scrutiny\*\* —

&#x20;  bugs can affect system audio for all users, not just this app, and issues

&#x20;  here are harder for users to self-diagnose or roll back.

3\. \*\*DfxDsp\*\* (`dsp/`) — the DSP engine that processes audio in real time.

&#x20;  Changes here affect audio quality and performance directly; watch for

&#x20;  real-time-safety issues (allocations, locks, or blocking calls in the audio

&#x20;  callback path) and numerical correctness in filter/effect code.



\## Build environment



\- Visual Studio 2022, Windows SDK

\- JUCE 6.1.6 for x64/x86; latest JUCE framework build for ARM64

\- Solution: `fxsound/Project/FxSound.sln`

\- Running the app locally requires FxSound's virtual audio driver to already

&#x20; be installed (via the normal FxSound installer), since the driver isn't

&#x20; built from this source tree.

\- Note: when a solution is exported fresh from Projucer, `audiopassthru` and

&#x20; `DfxDsp` project references may need to be re-added manually — this is a

&#x20; known Projucer export quirk, not a code defect, if you see it in a PR.



\## PR review priorities



When reviewing pull requests:

\- Flag any change to `audiopassthru/` or `dsp/` explicitly as higher-risk,

&#x20; even if the diff looks small.

\- In `dsp/`, check for anything that could block or allocate on the real-time

&#x20; audio thread.

\- In `audiopassthru/`, check for driver-interface assumptions that might not

&#x20; hold across Windows versions.

\- Standard C++ concerns apply otherwise: resource leaks, undefined behavior,

&#x20; thread safety, off-by-one errors.

\- Don't flag pure formatting/style differences unless they violate patterns

&#x20; already established nearby in the same file.



\## Issue triage



When triaging an issue, report (don't apply labels unless explicitly asked to

in a separate `@claude apply labels` step):

\- \*\*Type\*\*: bug, feature request, question, or build/installation issue.

\- \*\*Priority\*\*: Urgent/High/Medium/Low, based on severity and how many users

&#x20; are likely affected — but note this is set on the GitHub Projects board's

&#x20; Priority field, not a label, so state your suggestion in the comment rather

&#x20; than assuming it can be applied automatically.

\- \*\*Related/duplicate issues\*\*: search both open and closed issues.

\- \*\*Fixed status\*\*: check recent commits on `main` and closed issues/PRs to

&#x20; see if this may already be fixed but not yet released.



\## Labels



Only apply labels that already exist in the repository. Don't create new

labels. If no suitable label exists for something you'd normally tag, say so

in your comment instead of guessing.



\## What not to do



\- Don't merge, close, or assign issues/PRs yourself.

\- Don't modify the GitHub Projects Priority field unless a workflow has been

&#x20; explicitly given permission and instruction to do so.

\- Don't make changes to `audiopassthru/` or `dsp/` in an automated fix without

&#x20; flagging it clearly for human review before merge.

