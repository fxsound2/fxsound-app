/*
FxSound — Linux build

Linux backing for the "reg" module (audiopassthru/include/reg.h), which the DSP
uses to persist EQ/effect/session settings. On Windows this reads/writes the
Windows registry; here we provide an in-process key/value store so settings work
within a session. Persisting to a config file under ~/.config/fxsound is a
follow-up; the API surface below is the integration point for it.
*/

#include "codedefs.h"
#include "reg.h"

#include <map>
#include <mutex>
#include <string>
#include <cwchar>

namespace {
std::map<std::wstring, std::wstring>& store()
{
    static std::map<std::wstring, std::wstring> s;
    return s;
}
std::mutex& lock() { static std::mutex m; return m; }

std::wstring joinKey(const wchar_t* path, const wchar_t* keyname)
{
    std::wstring k = path ? path : L"";
    if (keyname && *keyname) { k += L"\\"; k += keyname; }
    return k;
}

void copyOut(const std::wstring& v, wchar_t* buf, uint32_t buflen)
{
    if (!buf || buflen == 0) return;
    uint32_t n = (uint32_t)v.size();
    if (n >= buflen) n = buflen - 1;
    wmemcpy(buf, v.c_str(), n);
    buf[n] = L'\0';
}
} // namespace

int regCreateKey_Wide(int, wchar_t* path, wchar_t* value)
{
    std::lock_guard<std::mutex> g(lock());
    store()[path ? path : L""] = value ? value : L"";
    return OKAY;
}

int regReadKey_Wide(int, wchar_t* path, int* key_exists, wchar_t* value, uint32_t buflen)
{
    std::lock_guard<std::mutex> g(lock());
    auto it = store().find(path ? path : L"");
    if (it == store().end()) { if (key_exists) *key_exists = IS_FALSE; return OKAY; }
    if (key_exists) *key_exists = IS_TRUE;
    copyOut(it->second, value, buflen);
    return OKAY;
}

int regCreateKeyTest_Wide(int, wchar_t* path, wchar_t* value, int* created)
{
    regCreateKey_Wide(0, path, value);
    if (created) *created = IS_TRUE;
    return OKAY;
}

int regRemoveKey_Wide(int, wchar_t* path)
{
    std::lock_guard<std::mutex> g(lock());
    store().erase(path ? path : L"");
    return OKAY;
}

int regCreateKeyWithKeyname_Dword_Wide(int, wchar_t* path, wchar_t* keyname, uint32_t value)
{
    std::lock_guard<std::mutex> g(lock());
    store()[joinKey(path, keyname)] = std::to_wstring(value);
    return OKAY;
}

int regCreateKeyWithKeyname_String_Wide(int, wchar_t* path, wchar_t* keyname, wchar_t* value)
{
    std::lock_guard<std::mutex> g(lock());
    store()[joinKey(path, keyname)] = value ? value : L"";
    return OKAY;
}

int regReadKeyWithKeyname_String_Wide(int, wchar_t* path, wchar_t* keyname, int* key_exists,
                                      wchar_t* value, uint32_t buflen)
{
    std::lock_guard<std::mutex> g(lock());
    auto it = store().find(joinKey(path, keyname));
    if (it == store().end()) { if (key_exists) *key_exists = IS_FALSE; return OKAY; }
    if (key_exists) *key_exists = IS_TRUE;
    copyOut(it->second, value, buflen);
    return OKAY;
}

int regReadKeyWithKeyname_Dword_Wide(int, wchar_t* path, wchar_t* keyname, int* key_exists,
                                     uint32_t* value)
{
    std::lock_guard<std::mutex> g(lock());
    auto it = store().find(joinKey(path, keyname));
    if (it == store().end()) { if (key_exists) *key_exists = IS_FALSE; return OKAY; }
    if (key_exists) *key_exists = IS_TRUE;
    if (value) *value = (uint32_t)wcstoul(it->second.c_str(), nullptr, 10);
    return OKAY;
}

int regRecursiveDeleteFolder_Wide(int, wchar_t* path)
{
    std::lock_guard<std::mutex> g(lock());
    std::wstring prefix = path ? path : L"";
    for (auto it = store().begin(); it != store().end();)
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0) it = store().erase(it);
        else ++it;
    }
    return OKAY;
}

int regReadRegisteredOwner_Wide(wchar_t* owner, int buflen)
{
    if (owner && buflen > 0) owner[0] = L'\0';
    return OKAY;
}
