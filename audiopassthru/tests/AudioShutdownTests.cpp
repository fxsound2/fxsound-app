/*
FxSound
Copyright (C) 2025 FxSound LLC
Contributors: Sylvain Petitgrand (2026)

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License, version 3 or later.
This program is distributed WITHOUT ANY WARRANTY. See <https://www.gnu.org/licenses/>.
*/
#include "AudioShutdown.h"
#include "EndpointRestore.h"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
class Event final
{
public:
    Event() : handle(CreateEventW(nullptr, TRUE, FALSE, nullptr))
    {
        if (handle == nullptr) throw std::runtime_error("CreateEventW failed");
    }
    ~Event() { CloseHandle(handle); }
    Event(const Event&) = delete;
    void set() const { SetEvent(handle); }
    bool wait(DWORD ms) const { return WaitForSingleObject(handle, ms) == WAIT_OBJECT_0; }
    HANDLE handle;
};
struct TestState
{
    std::atomic<int> restores{0}, cleanups{0};
    std::atomic<DWORD> requestedBudget{0};
};
struct FakeEndpointBackend
{
    std::wstring current = L"virtual";
    HRESULT queryResult = S_OK, muteResult = S_OK;
    std::vector<std::wstring> activeIds, queried, selected, muted;
    std::vector<std::pair<std::wstring, HRESULT>> failures;
    std::vector<std::wstring> operations;
    bool muteValue = false;
    HRESULT defaultEndpoint(std::wstring& id)
    {
        operations.push_back(L"query");
        id = current;
        return queryResult;
    }
    bool active(const std::wstring& id)
    {
        queried.push_back(id);
        return std::any_of(activeIds.begin(), activeIds.end(), [&](const auto& activeId)
        { return fxaudio::sameEndpointId(id, activeId); });
    }
    HRESULT restoreMute(const std::wstring& id, bool value)
    {
        muted.push_back(id);
        muteValue = value;
        operations.push_back(L"mute");
        return muteResult;
    }
    HRESULT setDefault(const std::wstring& id)
    {
        selected.push_back(id);
        operations.push_back(L"default");
        for (const auto& failure : failures)
            if (fxaudio::sameEndpointId(id, failure.first)) return failure.second;
        return S_OK;
    }
};
}

int main()
{
    unsigned failures = 0, count = 0;
    std::cout << "{\"tests\":[";
    const auto check = [&](const char* name, bool passed)
    {
        if (count++) std::cout << ',';
        std::cout << "{\"name\":\"" << name << "\",\"passed\":" << (passed ? "true" : "false") << '}';
        if (!passed) ++failures;
    };

    {
        fxaudio::ShutdownTask task;
        check("invalid_work_never_starts_thread", !task.start({}) && !task.status().started
              && task.status().error == ERROR_INVALID_PARAMETER && !task.status().safeToDestroy());
    }
    {
        fxaudio::ShutdownTask task;
        const auto state = std::make_shared<TestState>();
        const bool started = task.start({[state](DWORD budget)
            { state->requestedBudget = budget; return true; },
            [state] { ++state->restores; return S_OK; },
            [state] { ++state->cleanups; return true; }, 75});
        const auto result = task.wait(1000);
        check("normal_stop_restores_and_cleans_once", started && result.safeToDestroy()
            && result.error == ERROR_SUCCESS && result.restoreResult == S_OK
            && state->restores == 1 && state->cleanups == 1 && state->requestedBudget == 75);
        check("repeated_wait_does_not_repeat_work", task.wait(0).safeToDestroy()
              && state->restores == 1 && state->cleanups == 1);
    }
    {
        Event entered, release;
        const auto state = std::make_shared<TestState>();
        fxaudio::ShutdownTask task;
        const bool started = task.start({[](DWORD) { return false; }, [&]
            { ++state->restores; entered.set(); return release.wait(5000) ? S_OK : E_ABORT; },
            [state] { ++state->cleanups; return true; }});
        const bool restoring = entered.wait(1000);
        const auto begin = GetTickCount64();
        const auto pending = task.wait(40);
        const auto elapsed = GetTickCount64() - begin;
        check("hung_restore_does_not_extend_host_wait", started && restoring && elapsed < 500
              && !pending.finished && !pending.safeToDestroy() && pending.restoreResult == E_PENDING);
        release.set();
        const auto done = task.wait(1000);
        check("worker_timeout_still_restores_but_never_frees_live_audio", done.finished
              && !done.workerStopped && !done.cleanupCompleted && !done.safeToDestroy()
              && done.restoreResult == S_OK && state->restores == 1 && state->cleanups == 0);
    }
    {
        Event entered, release;
        fxaudio::ShutdownTask task;
        const bool started = task.start({[](DWORD) { return true; }, [] { return S_OK; }, [&]
            { entered.set(); return release.wait(5000); }});
        const bool cleaning = entered.wait(1000);
        const auto begin = GetTickCount64();
        const auto pending = task.wait(40);
        check("hung_cleanup_does_not_extend_host_wait", started && cleaning
            && GetTickCount64() - begin < 500 && pending.workerStopped
            && !pending.finished && !pending.cleanupCompleted && !pending.safeToDestroy());
        release.set();
        check("cleanup_completion_allows_destruction", task.wait(1000).safeToDestroy());
    }
    {
        Event entered, release;
        auto state = std::make_shared<TestState>();
        const std::weak_ptr<TestState> lifetime = state;
        bool started = false, restoring = false;
        {
            fxaudio::ShutdownTask task;
            started = task.start({[state](DWORD) { return false; }, [state, &entered, &release]
                { ++state->restores; entered.set(); return release.wait(5000) ? S_OK : E_ABORT; },
                [state] { ++state->cleanups; return true; }});
            restoring = entered.wait(1000);
            state.reset();
        }
        check("dropping_host_handle_keeps_running_work_captures_alive", started && restoring
              && !lifetime.expired());
        release.set();
        const auto deadline = GetTickCount64() + 1000;
        while (!lifetime.expired() && GetTickCount64() < deadline) Sleep(1);
        check("detached_work_releases_own_state_after_return", lifetime.expired());
    }
    {
        const auto state = std::make_shared<TestState>();
        fxaudio::ShutdownTask task;
        task.start({[](DWORD) { return true; }, []() -> HRESULT { throw std::runtime_error("expected restore error"); },
            [state] { ++state->cleanups; return true; }});
        const auto result = task.wait(1000);
        check("restore_exception_recorded_without_skipping_safe_cleanup", result.safeToDestroy()
              && result.restoreResult == E_UNEXPECTED && result.error == ERROR_UNHANDLED_EXCEPTION
              && state->cleanups == 1);
    }
    {
        const auto state = std::make_shared<TestState>();
        fxaudio::ShutdownTask task;
        task.start({[](DWORD) -> bool { throw std::runtime_error("expected wait error"); },
            [state] { ++state->restores; return S_OK; }, [state] { ++state->cleanups; return true; }});
        const auto result = task.wait(1000);
        check("wait_exception_still_restores_without_cleaning_live_audio", result.finished
              && !result.safeToDestroy() && result.error == ERROR_UNHANDLED_EXCEPTION
              && state->restores == 1 && state->cleanups == 0);
    }

    fxaudio::EndpointRestoreSnapshot snapshot{{L"", L"virtual", L"missing", L"first", L"FIRST", L"second"},
                                             L"virtual", L"first", true, true};
    {
        FakeEndpointBackend backend;
        backend.current = L"external-choice";
        backend.activeIds = {L"first", L"second"};
        check("preserves_external_default_changed_before_quit",
            fxaudio::restoreEndpointWithBackend(snapshot, backend) == S_FALSE
            && backend.selected.empty());
        check("external_default_does_not_skip_restoring_previous_real_mute",
            backend.muted == std::vector<std::wstring>({L"first"}) && backend.muteValue
            && backend.queried == std::vector<std::wstring>({L"first"}));
    }
    {
        FakeEndpointBackend backend;
        auto unknown = snapshot;
        unknown.virtualId.clear();
        check("unknown_virtual_id_never_changes_device_policy",
            fxaudio::restoreEndpointWithBackend(unknown, backend) == S_FALSE && backend.operations.empty());
    }
    {
        FakeEndpointBackend backend;
        backend.activeIds = {L"first", L"second"};
        backend.failures = {{L"first", E_FAIL}};
        check("selection_skips_virtual_inactive_duplicate_and_failed_candidates",
            fxaudio::restoreEndpointWithBackend(snapshot, backend) == S_OK
            && backend.selected == std::vector<std::wstring>({L"first", L"second"})
            && std::count(backend.queried.begin(), backend.queried.end(), L"FIRST") == 0
            && std::count(backend.queried.begin(), backend.queried.end(), L"virtual") == 0);
        check("restores_saved_real_mute_before_handing_back_default",
            backend.muted == std::vector<std::wstring>({L"first"}) && backend.muteValue
            && backend.operations.size() >= 3 && backend.operations[0] == L"mute"
            && backend.operations[2] == L"default");
    }
    {
        FakeEndpointBackend backend;
        backend.current = L"external-choice";
        backend.activeIds = {L"first"};
        backend.muteResult = E_ACCESSDENIED;
        check("external_default_preserved_when_saved_mute_restore_fails",
            fxaudio::restoreEndpointWithBackend(snapshot, backend) == E_ACCESSDENIED
            && backend.selected.empty() && backend.muted == std::vector<std::wstring>({L"first"}));
    }
    {
        FakeEndpointBackend backend;
        backend.queryResult = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        backend.activeIds = {L"first"};
        check("missing_default_after_driver_removal_uses_active_saved_candidate",
            fxaudio::restoreEndpointWithBackend(snapshot, backend) == S_OK
            && backend.selected == std::vector<std::wstring>({L"first"}));
    }
    {
        FakeEndpointBackend backend;
        backend.activeIds = {L"first"};
        backend.muteResult = E_ACCESSDENIED;
        check("mute_failure_does_not_prevent_default_recovery",
            fxaudio::restoreEndpointWithBackend(snapshot, backend) == E_ACCESSDENIED
            && backend.selected == std::vector<std::wstring>({L"first"}));
    }
    {
        FakeEndpointBackend backend;
        check("no_active_candidate_is_reported_without_policy_write",
            fxaudio::restoreEndpointWithBackend(snapshot, backend) == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
            && backend.selected.empty());
    }
    {
        FakeEndpointBackend backend;
        backend.queryResult = E_FAIL;
        check("cannot_read_current_default_does_not_overwrite_unknown_choice",
            fxaudio::restoreEndpointWithBackend(snapshot, backend) == E_FAIL
            && backend.selected.empty() && backend.muted.empty());
    }
    std::cout << "],\"valid\":" << (failures == 0 ? "true" : "false") << "}\n";
    return failures == 0 ? 0 : 1;
}
