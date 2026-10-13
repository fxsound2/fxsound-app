#include "portable-runtime.h"
#include "codedefs.h"
#include "reg.h"
#include <map>
#include <mutex>
// Legacy keys become an in-memory control-side settings store. Persistence belongs to the host.
namespace {
using Key = std::pair<int, std::wstring>;
std::map<Key, std::wstring> settings;
std::mutex settingsMutex;
}
int regCreateKey_Wide(int domain, wchar_t *name, wchar_t *value) {
    if (!name || !value) return NOT_OKAY;
    std::lock_guard<std::mutex> lock(settingsMutex);
    settings[{domain, name}] = value;
    return OKAY;
}
int regCreateKey(int domain, char *name, char *value) {
    if (!name || !value) return NOT_OKAY;
    auto wideName = dspWide(name), wideValue = dspWide(value);
    return regCreateKey_Wide(domain, wideName.data(), wideValue.data());
}
int regReadKey_Wide(int domain, wchar_t *name, int *exists, wchar_t *value, unsigned long capacity) {
    if (!name || !exists || !value || capacity == 0) return NOT_OKAY;
    std::lock_guard<std::mutex> lock(settingsMutex);
    auto found = settings.find({domain, name});
    *exists = found != settings.end();
    value[0] = 0;
    if (!*exists) return OKAY;
    if (found->second.size() >= capacity) return NOT_OKAY;
    std::copy(found->second.begin(), found->second.end(), value);
    value[found->second.size()] = 0;
    return OKAY;
}
