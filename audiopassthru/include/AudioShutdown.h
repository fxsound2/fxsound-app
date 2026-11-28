/*
FxSound
Copyright (C) 2025 FxSound LLC
Contributors: Sylvain Petitgrand (2026)

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License, version 3 or later.
This program is distributed WITHOUT ANY WARRANTY. See <https://www.gnu.org/licenses/>.
*/
#pragma once
#include <Windows.h>
#include <atomic>
#include <functional>
#include <memory>
#include <utility>

namespace fxaudio
{
// The host bounds its wait, not the duration of a possibly hung COM operation.
// Captured resources must remain alive until this task finishes. In particular,
// a host that captures an audio owner by raw pointer must retain that owner when
// safeToDestroy() is false, including when the host's wait has timed out.
class ShutdownTask final
{
public:
    struct Work
    {
        std::function<bool(DWORD)> waitWorker;
        std::function<HRESULT()> restore;
        std::function<bool()> cleanup;
        DWORD workerWaitMs = 2000;
    };
    struct Outcome
    {
        bool started = false;
        bool finished = false;
        bool workerStopped = false;
        bool cleanupCompleted = false;
        DWORD error = ERROR_SUCCESS;
        HRESULT restoreResult = E_PENDING;
        bool safeToDestroy() const noexcept
        {
            return finished && workerStopped && cleanupCompleted;
        }
    };

    ShutdownTask() = default;
    ~ShutdownTask() { if (thread_ != nullptr) CloseHandle(thread_); }
    ShutdownTask(const ShutdownTask&) = delete;
    ShutdownTask& operator=(const ShutdownTask&) = delete;

    // Called once by the owner thread. No worker begins for an invalid request.
    bool start(Work work) noexcept
    {
        if (state_ != nullptr || thread_ != nullptr)
        {
            startError_ = ERROR_ALREADY_INITIALIZED;
            return false;
        }
        if (!work.waitWorker || !work.restore || !work.cleanup)
        {
            startError_ = ERROR_INVALID_PARAMETER;
            return false;
        }
        startError_ = ERROR_SUCCESS;
        try
        {
            state_ = std::make_shared<State>(std::move(work));
            auto parameter = std::make_unique<std::shared_ptr<State>>(state_);
            thread_ = CreateThread(nullptr, 0, run, parameter.get(), 0, nullptr);
            if (thread_ == nullptr)
            {
                startError_ = GetLastError();
                state_.reset();
                return false;
            }
            parameter.release(); // The running thread owns this reference.
            return true;
        }
        catch (...)
        {
            startError_ = ERROR_NOT_ENOUGH_MEMORY;
            state_.reset();
            OutputDebugStringW(L"FxSound: could not allocate audio shutdown task\n");
            return false;
        }
    }

    // No message pumping and no thread termination. A timeout does not cancel
    // work or permit the caller to free resources captured by that work.
    Outcome wait(DWORD timeoutMs) noexcept
    {
        if (thread_ != nullptr)
        {
            if (WaitForSingleObject(thread_, timeoutMs) == WAIT_FAILED)
                recordError(*state_, GetLastError());
        }
        return status();
    }

    Outcome status() const noexcept
    {
        Outcome outcome;
        outcome.error = startError_;
        if (state_ == nullptr || thread_ == nullptr) return outcome;
        outcome.started = true;
        // Only an actually signaled thread allows destruction of its captures:
        // its final C++ destructors may still run after the work itself returned.
        outcome.finished = WaitForSingleObject(thread_, 0) == WAIT_OBJECT_0;
        outcome.workerStopped = state_->workerStopped.load(std::memory_order_acquire);
        outcome.cleanupCompleted = state_->cleanupCompleted.load(std::memory_order_acquire);
        outcome.restoreResult = state_->restoreResult.load(std::memory_order_acquire);
        if (outcome.error == ERROR_SUCCESS)
            outcome.error = state_->error.load(std::memory_order_acquire);
        return outcome;
    }

private:
    struct State
    {
        explicit State(Work requested) : work(std::move(requested)) {}
        Work work;
        std::atomic<bool> workerStopped{false}, cleanupCompleted{false};
        std::atomic<HRESULT> restoreResult{E_PENDING};
        std::atomic<DWORD> error{ERROR_SUCCESS};
    };

    static void recordError(State& state, DWORD error) noexcept
    {
        DWORD previous = ERROR_SUCCESS;
        state.error.compare_exchange_strong(previous, error, std::memory_order_release);
    }

    static DWORD WINAPI run(void* raw) noexcept
    {
        std::unique_ptr<std::shared_ptr<State>> parameter(
            static_cast<std::shared_ptr<State>*>(raw));
        const auto state = std::move(*parameter);
        parameter.reset();
        try
        {
            state->workerStopped.store(state->work.waitWorker(state->work.workerWaitMs),
                                       std::memory_order_release);
        }
        catch (...)
        {
            recordError(*state, ERROR_UNHANDLED_EXCEPTION);
            OutputDebugStringW(L"FxSound: exception while waiting for audio shutdown\n");
        }
        // Restoration does not depend on a stopped worker, and owns an immutable
        // snapshot rather than the running audio/device state.
        try
        {
            state->restoreResult.store(state->work.restore(), std::memory_order_release);
        }
        catch (...)
        {
            state->restoreResult.store(E_UNEXPECTED, std::memory_order_release);
            recordError(*state, ERROR_UNHANDLED_EXCEPTION);
            OutputDebugStringW(L"FxSound: exception restoring the default audio endpoint\n");
        }
        if (state->workerStopped.load(std::memory_order_acquire))
        {
            try
            {
                state->cleanupCompleted.store(state->work.cleanup(), std::memory_order_release);
            }
            catch (...)
            {
                recordError(*state, ERROR_UNHANDLED_EXCEPTION);
                OutputDebugStringW(L"FxSound: exception releasing stopped audio resources\n");
            }
        }
        return ERROR_SUCCESS;
    }

    HANDLE thread_ = nullptr;
    std::shared_ptr<State> state_;
    DWORD startError_ = ERROR_SUCCESS;
};
}
