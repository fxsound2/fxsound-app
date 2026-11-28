/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <memory>
#include <new>

namespace fxsounddevices
{
// Share the entry/close handshake across endpoint and volume callbacks. The
// cleanup owner closes first, unregisters, then waits outside callback context.
class DeviceCallbackScope
{
public:
    DeviceCallbackScope(std::atomic<unsigned>& active, const std::atomic<bool>& closed) noexcept
        : active_(active)
    {
        active_.fetch_add(1);
        accepting_ = !closed.load();
    }
    ~DeviceCallbackScope() { active_.fetch_sub(1); }
    explicit operator bool() const noexcept { return accepting_; }
    DeviceCallbackScope(const DeviceCallbackScope&) = delete;
    DeviceCallbackScope& operator=(const DeviceCallbackScope&) = delete;
private:
    std::atomic<unsigned>& active_;
    bool accepting_ = false;
};
// MMDevice may call several producers concurrently. Only the owner drains the
// queue; callbacks never allocate, enumerate devices, wait, or touch UI state.
class DeviceChangeQueue
{
public:
    static constexpr std::size_t capacity = 64;
    static constexpr std::size_t idCapacity = 512;
    enum class Kind { defaultChanged, added, removed, stateChanged };
    struct Event
    {
        Kind kind = Kind::added;
        wchar_t id[idCapacity]{};
        std::uint32_t state = 0;
        bool missingId = false;
    };

    DeviceChangeQueue() noexcept
    {
        for (std::size_t i = 0; i < capacity; ++i)
            cells_[i].sequence.store(i, std::memory_order_relaxed);
    }
    // MMDevice producer only: one try, never wait for another callback. An
    // overlap invalidates the cache epoch, so contention cannot hide A -> B -> A.
    bool isRepeatedState(const wchar_t* id, std::uint32_t state) noexcept
    {
        std::size_t length = 0;
        if (id != nullptr) while (length < idCapacity && id[length] != L'\0') ++length;
        if (id == nullptr || length == 0 || length == idCapacity)
        {
            invalidateStateCache();
            return false;
        }
        const auto epoch = stateEpoch_.load(std::memory_order_acquire);
        if (stateCacheBusy_.test_and_set(std::memory_order_acquire))
        {
            invalidateStateCache();
            return false;
        }
        const bool repeated = haveState_ && cachedStateEpoch_ == epoch
            && stateEpoch_.load(std::memory_order_acquire) == epoch
            && cachedState_ == state && std::wcscmp(cachedStateId_.data(), id) == 0;
        for (std::size_t i = 0; i < length; ++i) cachedStateId_[i] = id[i];
        cachedStateId_[length] = L'\0';
        cachedState_ = state;
        cachedStateEpoch_ = epoch;
        haveState_ = true;
        stateCacheBusy_.clear(std::memory_order_release);
        return repeated;
    }
    void invalidateStateCache() noexcept { stateEpoch_.fetch_add(1, std::memory_order_acq_rel); }
    bool enqueue(Kind kind, const wchar_t* id, std::uint32_t state = 0) noexcept
    {
        Event event;
        event.kind = kind;
        event.state = state;
        event.missingId = id == nullptr;
        if (id != nullptr)
        {
            std::size_t i = 0;
            while (i + 1 < idCapacity && id[i] != L'\0')
            {
                event.id[i] = id[i];
                ++i;
            }
            if (id[i] != L'\0') return lostEvent(); // Never compare a truncated ID.
        }
        auto position = write_.load(std::memory_order_relaxed);
        // Bound contention as well as capacity: the MMDevice thread must return.
        for (unsigned attempt = 0; attempt < capacity; ++attempt)
        {
            auto& cell = cells_[position % capacity];
            const auto sequence = cell.sequence.load(std::memory_order_acquire);
            if (sequence == position)
            {
                if (write_.compare_exchange_weak(position, position + 1, std::memory_order_relaxed))
                {
                    cell.event = event;
                    cell.sequence.store(position + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (sequence < position) return lostEvent();
            else position = write_.load(std::memory_order_relaxed);
        }
        return lostEvent();
    }
    bool tryPop(Event& event) noexcept
    {
        const auto position = read_.load(std::memory_order_relaxed);
        auto& cell = cells_[position % capacity];
        if (cell.sequence.load(std::memory_order_acquire) != position + 1) return false;
        event = cell.event;
        cell.sequence.store(position + capacity, std::memory_order_release);
        read_.store(position + 1, std::memory_order_release);
        return true;
    }
    bool hasPending() const noexcept
    {
        return write_.load(std::memory_order_acquire) != read_.load(std::memory_order_acquire)
            || reconcile_.load(std::memory_order_acquire);
    }
    bool takeReconcileRequired() noexcept { return reconcile_.exchange(false, std::memory_order_acq_rel); }
    std::uint64_t droppedEvents() const noexcept { return dropped_.load(std::memory_order_relaxed); }

    // Owner publishes one coherent route. Old nodes stay alive for MMDevice
    // readers; repeated routes reuse their existing node.
    bool setUsedEndpoints(const wchar_t* virtualId, const wchar_t* captureId, const wchar_t* outputId) noexcept
    {
        const wchar_t* ids[] = {virtualId, captureId, outputId};
        std::size_t lengths[3]{};
        for (int i = 0; i < 3; ++i)
        {
            if (ids[i] == nullptr) ids[i] = L"";
            while (lengths[i] < idCapacity && ids[i][lengths[i]] != L'\0') ++lengths[i];
            if (lengths[i] == idCapacity) return invalidateUsedEndpoints();
        }
        if (lengths[0] == 0 || lengths[1] == 0 || lengths[2] == 0) return invalidateUsedEndpoints();
        for (auto* prior = routes_.get(); prior != nullptr; prior = prior->previous.get())
            if (std::wcscmp(prior->ids[0].data(), ids[0]) == 0
                && std::wcscmp(prior->ids[1].data(), ids[1]) == 0
                && std::wcscmp(prior->ids[2].data(), ids[2]) == 0)
            { usedRoute_.store(prior, std::memory_order_release); return true; }
        auto next = std::unique_ptr<UsedRoute>(new (std::nothrow) UsedRoute);
        if (!next) return invalidateUsedEndpoints();
        for (int i = 0; i < 3; ++i)
            for (std::size_t j = 0; j < lengths[i]; ++j) next->ids[i][j] = ids[i][j];
        next->previous = std::move(routes_);
        routes_ = std::move(next);
        usedRoute_.store(routes_.get(), std::memory_order_release);
        return true;
    }
    bool invalidateUsedEndpoints() noexcept
    {
        usedRoute_.store(nullptr, std::memory_order_release);
        return lostEvent();
    }
    void forgetUsedEndpoints() noexcept { usedRoute_.store(nullptr, std::memory_order_release); }
    bool usesEndpoint(const wchar_t* id) const noexcept
    {
        const auto* route = usedRoute_.load(std::memory_order_acquire);
        if (route == nullptr || id == nullptr || id[0] == L'\0') return true;
        for (const auto& used : route->ids) if (std::wcscmp(used.data(), id) == 0) return true;
        return false;
    }

    // Owner only. Retain old immutable identities until the notification backing
    // state dies, so callback readers need neither a lock nor reference counting.
    bool setVirtualDeviceId(const wchar_t* id) noexcept
    {
        const auto* prior = virtualId_.load(std::memory_order_acquire);
        if (id == nullptr || id[0] == L'\0')
        {
            virtualId_.store(nullptr, std::memory_order_release);
            return true;
        }
        if (prior != nullptr && std::wcscmp(prior, id) == 0) return true;
        std::size_t length = 0;
        while (length < idCapacity && id[length] != L'\0') ++length;
        if (length == idCapacity) return lostEvent();
        auto next = std::unique_ptr<Identity>(new (std::nothrow) Identity);
        if (!next) return lostEvent();
        for (std::size_t i = 0; i < length; ++i) next->id[i] = id[i];
        next->previous = std::move(identities_);
        identities_ = std::move(next);
        virtualId_.store(identities_->id.data(), std::memory_order_release);
        return true;
    }
    bool isVirtualDevice(const wchar_t* id) const noexcept
    {
        const auto* own = virtualId_.load(std::memory_order_acquire);
        return id != nullptr && own != nullptr && std::wcscmp(id, own) == 0;
    }
    // Owner only; suppress repeated reports without losing A -> B -> A changes.
    bool isDuplicate(const Event& event) noexcept
    {
        const bool duplicate = haveLast_ && last_.kind == event.kind && last_.state == event.state
            && last_.missingId == event.missingId && std::wcscmp(last_.id, event.id) == 0;
        last_ = event;
        haveLast_ = true;
        return duplicate;
    }
private:
    friend struct DeviceChangeQueueTestPeer;
    struct Cell { std::atomic<std::size_t> sequence{0}; Event event; };
    struct Identity { std::array<wchar_t, idCapacity> id{}; std::unique_ptr<Identity> previous; };
    struct UsedRoute { std::array<std::array<wchar_t, idCapacity>, 3> ids{}; std::unique_ptr<UsedRoute> previous; };
    bool lostEvent() noexcept
    {
        dropped_.fetch_add(1, std::memory_order_relaxed);
        reconcile_.store(true, std::memory_order_release);
        return false;
    }
    std::atomic_flag stateCacheBusy_ = ATOMIC_FLAG_INIT;
    std::atomic<std::uint64_t> stateEpoch_{0};
    std::uint64_t cachedStateEpoch_ = 0;
    std::array<wchar_t, idCapacity> cachedStateId_{};
    std::uint32_t cachedState_ = 0;
    bool haveState_ = false;
    std::array<Cell, capacity> cells_;
    std::atomic<std::size_t> write_{0}, read_{0};
    std::atomic<bool> reconcile_{false};
    std::atomic<std::uint64_t> dropped_{0};
    std::unique_ptr<Identity> identities_;
    std::atomic<const wchar_t*> virtualId_{nullptr};
    std::unique_ptr<UsedRoute> routes_;
    std::atomic<const UsedRoute*> usedRoute_{nullptr};
    Event last_;
    bool haveLast_ = false;
};
}
