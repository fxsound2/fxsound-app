/*
FxSound
Copyright (C) 2026 FxSound LLC
SPDX-License-Identifier: AGPL-3.0-or-later
*/
// Runs the real processTimer with no endpoint, audio worker or user settings.
#include <windows.h>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

int WINAPI testMessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT);
int WINAPI testMessageBoxA(HWND, LPCSTR, LPCSTR, UINT);
void WINAPI testDebugBreak();
// Any unexpected modal diagnostic becomes a failed check, never an open dialog.
#define MessageBoxW testMessageBoxW
#define MessageBoxA testMessageBoxA
#define DebugBreak testDebugBreak
#include "codedefs.h"
#include "u_AudioPassthru.h"
#include "reg.h"

int testReInit(PT_HANDLE*, int, int*, int*, int*);
int testRestoreDefault(PT_HANDLE*, int*);
int testFree(PT_HANDLE*);
int testSetDeviceType(PT_HANDLE*, int, wchar_t*, int*);
ULONGLONG WINAPI testClock();
HANDLE WINAPI testCreateThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
BOOL WINAPI testGetExitCodeThread(HANDLE, LPDWORD);
BOOL WINAPI testCloseHandle(HANDLE);
void WINAPI testDiagnostic(LPCWSTR);

// Headers were included above; only calls in this production translation unit
// are redirected. The library supplies uncalled dependencies for other methods.
#define sndDevicesReInit testReInit
#define sndDevicesRestoreDefaultDevice testRestoreDefault
#define sndDevicesFree testFree
#define sndDevicesSetDeviceType testSetDeviceType
#define GetTickCount64 testClock
#define CreateThread testCreateThread
#define GetExitCodeThread testGetExitCodeThread
#define CloseHandle testCloseHandle
#define OutputDebugStringW testDiagnostic
#include "../src/AudioPassthru/AudioPassthruPrivate.cpp"
#undef sndDevicesReInit
#undef sndDevicesRestoreDefaultDevice
#undef sndDevicesFree
#undef sndDevicesSetDeviceType
#undef GetTickCount64
#undef CreateThread
#undef GetExitCodeThread
#undef CloseHandle
#undef OutputDebugStringW
#undef MessageBoxW
#undef MessageBoxA
#undef DebugBreak

struct ProcessingRetryTestPeer
{
    static void reset()
    {
        AudioPassthruPrivate::s_device_change_generation_.store(0);
        AudioPassthruPrivate::s_sndDevices_.playbackDeviceIsUnavailable = FALSE;
        AudioPassthruPrivate::s_callback_ = nullptr;
    }
    static void notify() { AudioPassthruPrivate::onDeviceChange(); }
    static bool delayed(const AudioPassthruPrivate& audio) { return audio.processing_retry_pending_; }
    static bool unavailable() { return AudioPassthruPrivate::s_sndDevices_.playbackDeviceIsUnavailable != FALSE; }
    static void inertTeardown(AudioPassthruPrivate& audio)
    {
        // The original destructor exits before stop/restore/Free on a null handle.
        audio.hProcessingThread_ = nullptr;
        audio.hp_sndDevices_ = nullptr;
        AudioPassthruPrivate::s_callback_ = nullptr;
    }
};

namespace
{
struct Backend
{
    ULONGLONG now = 0, callDuration = 0;
    int attempts = 0, threadAttempts = 0, selections = 0;
    int result = NOT_OKAY_NO_BREAK, status = 0x101;
    int selectionResult = OKAY, selectionStatus = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
    int realDevices = 1, dfxEnabled = IS_TRUE;
    bool threadFails = false, notifyDuringCall = false;
    DWORD threadError = ERROR_NOT_ENOUGH_MEMORY, threadExit = STILL_ACTIVE;
    std::vector<std::wstring> diagnostics;
} backend;
int checks = 0, failures = 0, modalCalls = 0, expectedModalCalls = 0, restorationCalls = 0, freeCalls = 0;
void check(bool condition, const char* name)
{
    ++checks;
    if (!condition) ++failures;
    std::printf("%s %s\n", condition ? "PASS" : "FAIL", name);
}
struct Fixture
{
    AudioPassthruPrivate audio;
    Fixture() { backend = Backend{}; ProcessingRetryTestPeer::reset(); }
    ~Fixture() { ProcessingRetryTestPeer::inertTeardown(audio); }
    int tick(ULONGLONG at) { backend.now = at; return audio.processTimer(); }
};
}

int WINAPI testMessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT) { ++modalCalls; return IDOK; }
int WINAPI testMessageBoxA(HWND, LPCSTR, LPCSTR, UINT) { ++modalCalls; return IDOK; }
void WINAPI testDebugBreak() { ++modalCalls; }
ULONGLONG WINAPI testClock() { return backend.now; }
void WINAPI testDiagnostic(LPCWSTR text) { backend.diagnostics.emplace_back(text); }
int testRestoreDefault(PT_HANDLE*, int*) { ++restorationCalls; return OKAY; }
int testFree(PT_HANDLE*) { ++freeCalls; return OKAY; }
int testSetDeviceType(PT_HANDLE*, int, wchar_t*, int* status)
{
    ++backend.selections;
    *status = backend.selectionStatus;
    return backend.selectionResult;
}
int testReInit(PT_HANDLE* handle, int, int* devices, int* enabled, int* status)
{
    ++backend.attempts;
    *devices = backend.realDevices;
    *enabled = backend.dfxEnabled;
    *status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
    reinterpret_cast<sndDevicesHdlType*>(handle)->function_status = backend.status;
    backend.now += backend.callDuration;
    if (backend.notifyDuringCall)
    {
        backend.notifyDuringCall = false;
        std::thread notice([] { ProcessingRetryTestPeer::notify(); });
        notice.join();
    }
    return backend.result;
}
HANDLE WINAPI testCreateThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)
{
    ++backend.threadAttempts;
    if (backend.threadFails) { SetLastError(backend.threadError); return nullptr; }
    return reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(1)); // Never passed to a real Win32 API.
}
BOOL WINAPI testGetExitCodeThread(HANDLE, LPDWORD code) { *code = backend.threadExit; return TRUE; }
BOOL WINAPI testCloseHandle(HANDLE) { return TRUE; }

void failedReinitCadence()
{
    Fixture f;
    check(f.tick(0) == NOT_OKAY_NO_BREAK && backend.attempts == 1, "first hard failure is immediate and nonmodal");
    for (ULONGLONG now = 100; now < 1000; now += 100) f.tick(now);
    f.tick(999);
    check(backend.attempts == 1, "100 ms ticks and 999 ms do not repeat failed ReInit");
    f.tick(1000);
    check(backend.attempts == 2, "failed ReInit retries exactly at 1000 ms");
    f.tick(1999);
    check(backend.attempts == 2, "second failure has the same constant delay");
    f.tick(2000);
    check(backend.attempts == 3 && backend.diagnostics.size() == 1, "constant failure cadence does not flood diagnostics");
    ProcessingRetryTestPeer::notify();
    f.tick(2001);
    check(backend.attempts == 4, "device notification bypasses remaining failure delay");
}

void durationAndConcurrentNotice()
{
    {
        Fixture f;
        backend.callDuration = 250;
        f.tick(0);
        f.tick(1249);
        check(backend.attempts == 1, "retry deadline begins after the failed call completes");
        f.tick(1250);
        check(backend.attempts == 2, "failed call duration is not subtracted from the delay");
    }
    {
        Fixture f;
        backend.notifyDuringCall = true;
        f.tick(0);
        f.tick(1);
        check(backend.attempts == 2, "notification arriving during failure is not overwritten");
        f.tick(2);
        check(backend.attempts == 2, "consumed generation does not repeatedly bypass the deadline");
    }
}

void explicitSelectionAndRecovery()
{
    Fixture f;
    f.tick(0);
    backend.result = OKAY;
    check(f.audio.setTargetedRealPlaybackDevice(L"fixture-output") == OKAY && backend.selections == 1,
        "explicit selection uses the redirected device operation");
    f.tick(1);
    check(backend.attempts == 2 && backend.threadAttempts == 1 && !ProcessingRetryTestPeer::delayed(f.audio),
        "explicit selection retries immediately and clears failed state on success");
    check(backend.diagnostics.size() == 2 && backend.diagnostics.back().find(L"recovered") != std::wstring::npos,
        "successful recovery emits one transition diagnostic");
    f.tick(2);
    check(backend.attempts == 2, "healthy worker does not trigger reinitialization");
    backend.threadExit = 0;
    f.tick(3);
    check(backend.attempts == 3, "successful initialization has no unconditional one-second throttle");
}

void existingUnavailableSentinel()
{
    Fixture f;
    backend.result = OKAY;
    backend.realDevices = 0;
    f.tick(0);
    for (ULONGLONG now = 100; now <= 3000; now += 100) f.tick(now);
    check(backend.attempts == 1 && ProcessingRetryTestPeer::unavailable() && !ProcessingRetryTestPeer::delayed(f.audio),
        "zero-output success preserves the existing unavailable sentinel");
    ProcessingRetryTestPeer::notify();
    f.tick(3001); // Existing sentinel consumes one tick before a new attempt.
    f.tick(3101);
    check(backend.attempts == 2, "existing unavailable sentinel still recovers after notification");
}

void rejectedSelectionKeepsDeadline()
{
    Fixture f;
    f.tick(0);
    backend.selectionStatus = SND_DEVICES_RULES_NOT_POSSIBLE;
    check(f.audio.setTargetedRealPlaybackDevice(L"rejected-output") == OKAY,
        "noncompleted selection preserves the original public return contract");
    f.tick(1);
    f.tick(999);
    check(backend.attempts == 1, "selection rejected by status does not bypass the failure deadline");

    backend.selectionResult = NOT_OKAY_NO_BREAK;
    const int before = modalCalls;
    const int result = f.audio.setTargetedRealPlaybackDevice(L"failed-output");
    // The unchanged selection error path uses historical NOT_OKAY. Its modal
    // diagnostic is intercepted, explicitly expected, and never shown to a user.
    ++expectedModalCalls;
    check(result == NOT_OKAY_NO_BREAK && modalCalls == before + 1,
        "hard selection error is returned with its historical diagnostic intercepted");
    f.tick(999);
    check(backend.attempts == 1, "hard selection error also leaves the failure deadline intact");
    f.tick(1000);
    check(backend.attempts == 2, "rejected selections do not postpone the original due retry");
}

void failureToUnavailableSentinel()
{
    Fixture f;
    f.tick(0);
    backend.result = OKAY;
    backend.realDevices = 0;
    f.tick(1000);
    check(backend.attempts == 2 && ProcessingRetryTestPeer::unavailable() && !ProcessingRetryTestPeer::delayed(f.audio),
        "hard failure can transition to the existing unavailable sentinel");
    for (ULONGLONG now = 1100; now <= 5000; now += 100) f.tick(now);
    check(backend.attempts == 2 && backend.diagnostics.size() == 2,
        "unavailable sentinel stops periodic retries after a hard failure");
    ProcessingRetryTestPeer::notify();
    f.tick(5001);
    f.tick(5101);
    check(backend.attempts == 3, "failure-to-unavailable transition still permits notification recovery");
}

void threadFailureAndDiagnosticTransitions()
{
    Fixture f;
    backend.result = OKAY;
    backend.threadFails = true;
    check(f.tick(0) == NOT_OKAY_NO_BREAK && backend.threadAttempts == 1, "CreateThread failure is returned without a modal diagnostic");
    f.tick(999);
    check(backend.attempts == 1 && backend.threadAttempts == 1, "thread creation failure also delays expensive ReInit");
    f.tick(1000);
    check(backend.threadAttempts == 2 && backend.diagnostics.size() == 1, "repeated thread failure is rate limited");
    backend.threadError = ERROR_ACCESS_DENIED;
    f.tick(2000);
    check(backend.diagnostics.size() == 2 && backend.diagnostics.back().find(L"00000005") != std::wstring::npos,
        "changed thread error is diagnosed with GetLastError");
    backend.result = NOT_OKAY_NO_BREAK;
    backend.status = 0x202;
    f.tick(3000);
    check(backend.diagnostics.size() == 3 && backend.diagnostics.back().find(L"sndDevicesReInit") != std::wstring::npos,
        "failure-stage change is diagnosed");
    backend.status = 0x203;
    f.tick(4000);
    check(backend.diagnostics.size() == 4, "changed ReInit status is diagnosed");
    backend.result = OKAY;
    backend.threadFails = false;
    f.tick(5000);
    f.tick(5001);
    check(backend.diagnostics.size() == 5 && !ProcessingRetryTestPeer::delayed(f.audio), "recovery diagnostic is not repeated by ordinary ticks");
}

void longUptime()
{
    Fixture f;
    const ULONGLONG start = (static_cast<ULONGLONG>(1) << 32) + 17;
    f.tick(start);
    f.tick(start + 999);
    check(backend.attempts == 1, "deadline remains valid beyond 32-bit tick wrap");
    f.tick(start + 1000);
    check(backend.attempts == 2, "64-bit deadline expires at the correct long uptime");
}

int main()
{
    failedReinitCadence();
    durationAndConcurrentNotice();
    explicitSelectionAndRecovery();
    existingUnavailableSentinel();
    rejectedSelectionKeepsDeadline();
    failureToUnavailableSentinel();
    threadFailureAndDiagnosticTransitions();
    longUptime();
    check(modalCalls == expectedModalCalls, "no unexpected modal diagnostic or debugger break was invoked");
    check(restorationCalls == 0 && freeCalls == 0, "fixture teardown never restores or frees a real audio runtime");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
