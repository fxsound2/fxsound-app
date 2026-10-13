#include "codedefs.h"
#include "slout.h"
#include "dfxSharedUtil.h"
#include <array>
#include <algorithm>
#include <atomic>
namespace {
struct Publication { std::array<std::atomic<uint32_t>, 10> spectrum{}; std::atomic<unsigned long> milliseconds{0}; };
static_assert(std::atomic<uint32_t>::is_always_lock_free);
static_assert(std::atomic<unsigned long>::is_always_lock_free);
Publication *state(PT_HANDLE *handle) { return reinterpret_cast<Publication *>(handle); }
}
int dfxSharedUtilInit(PT_HANDLE **handle, int, CSlout *) {
    if (!handle) return NOT_OKAY;
    *handle = reinterpret_cast<PT_HANDLE *>(new Publication);
    return OKAY;
}
int dfxSharedUtilFreeUp(PT_HANDLE **handle) {
    if (!handle) return NOT_OKAY;
    delete state(*handle);
    *handle = nullptr;
    return OKAY;
}
int dfxSharedUtilSetSpectrumValues(PT_HANDLE *handle, float *values, int count) {
    if (!handle || !values || count < 0 || count > 10) return NOT_OKAY;
    for (int i = 0; i < count; ++i) state(handle)->spectrum[i].store(dspFloatBits(values[i]), std::memory_order_relaxed);
    return OKAY;
}
int dfxSharedUtilGetSpectrumValues(PT_HANDLE *handle, float *values, int count) {
    if (!handle || !values || count < 0 || count > 10) return NOT_OKAY;
    for (int i = 0; i < count; ++i) values[i] = dspBitsFloat(state(handle)->spectrum[i].load(std::memory_order_relaxed));
    return OKAY;
}
int dfxSharedUtilSetTotalProcessedTime(PT_HANDLE *handle, unsigned long milliseconds) {
    if (!handle) return NOT_OKAY;
    state(handle)->milliseconds.store(milliseconds, std::memory_order_relaxed);
    return OKAY;
}
int dfxSharedUtilGetTotalProcessedTime(PT_HANDLE *handle, unsigned long *milliseconds) {
    if (!handle || !milliseconds) return NOT_OKAY;
    *milliseconds = state(handle)->milliseconds.load(std::memory_order_relaxed);
    return OKAY;
}
