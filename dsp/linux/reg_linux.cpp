/*
FxSound — Linux build

Linux backing for the "reg" module (audiopassthru/include/reg.h), which the DSP
uses to persist EQ/effect/session settings. On Windows this reads/writes the
Windows registry; here we use an in-process key/value store backed by
~/.config/fxsound/registry.ini (loaded once on first access, flushed on every
write).
*/

#include "codedefs.h"
#include "reg.h"

#include <map>
#include <mutex>
#include <string>
#include <cwchar>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace {

// --- UTF-8 <-> UCS-4 (wchar_t on Linux) -------------------------------------

static std::string wstrToUtf8(const std::wstring& ws)
{
    std::string out;
    out.reserve(ws.size());
    for (wchar_t wc : ws) {
        uint32_t cp = (uint32_t)wc;
        if (cp < 0x80) {
            out += (char)cp;
        } else if (cp < 0x800) {
            out += (char)(0xC0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        } else {
            out += (char)(0xF0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3F));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

static std::wstring utf8ToWstr(const std::string& s)
{
    std::wstring out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        uint32_t cp;
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80)      { cp = c; i += 1; }
        else if (c < 0xE0) { cp = (c & 0x1F) << 6  | ((unsigned char)s[i+1] & 0x3F); i += 2; }
        else if (c < 0xF0) { cp = (c & 0x0F) << 12 | ((unsigned char)s[i+1] & 0x3F) << 6
                                                     | ((unsigned char)s[i+2] & 0x3F); i += 3; }
        else               { cp = (c & 0x07) << 18 | ((unsigned char)s[i+1] & 0x3F) << 12
                                                    | ((unsigned char)s[i+2] & 0x3F) << 6
                                                    | ((unsigned char)s[i+3] & 0x3F); i += 4; }
        out += (wchar_t)cp;
    }
    return out;
}

// --- File path ---------------------------------------------------------------

static std::filesystem::path getRegPath()
{
    const char* xdg = getenv("XDG_CONFIG_HOME");
    std::filesystem::path base = xdg ? xdg
                                     : (std::filesystem::path(getenv("HOME")) / ".config");
    return base / "fxsound" / "registry.ini";
}

// --- Persistent store --------------------------------------------------------

std::map<std::wstring, std::wstring>& store()
{
    static std::map<std::wstring, std::wstring> s;
    return s;
}
std::mutex& lock() { static std::mutex m; return m; }
bool& storeLoaded() { static bool v = false; return v; }

static void loadStore()
{
    auto path = getRegPath();
    std::ifstream f(path);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        store()[utf8ToWstr(line.substr(0, eq))] = utf8ToWstr(line.substr(eq + 1));
    }
    storeLoaded() = true;
}

static void ensureLoaded()
{
    if (!storeLoaded()) loadStore();
}

static void saveStore()
{
    auto path = getRegPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream f(path, std::ios::trunc);
    if (!f) return;
    for (auto& [k, v] : store())
        f << wstrToUtf8(k) << '=' << wstrToUtf8(v) << '\n';
}

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
    ensureLoaded();
    store()[path ? path : L""] = value ? value : L"";
    saveStore();
    return OKAY;
}

int regReadKey_Wide(int, wchar_t* path, int* key_exists, wchar_t* value, uint32_t buflen)
{
    std::lock_guard<std::mutex> g(lock());
    ensureLoaded();
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
    ensureLoaded();
    store().erase(path ? path : L"");
    saveStore();
    return OKAY;
}

int regCreateKeyWithKeyname_Dword_Wide(int, wchar_t* path, wchar_t* keyname, uint32_t value)
{
    std::lock_guard<std::mutex> g(lock());
    ensureLoaded();
    store()[joinKey(path, keyname)] = std::to_wstring(value);
    saveStore();
    return OKAY;
}

int regCreateKeyWithKeyname_String_Wide(int, wchar_t* path, wchar_t* keyname, wchar_t* value)
{
    std::lock_guard<std::mutex> g(lock());
    ensureLoaded();
    store()[joinKey(path, keyname)] = value ? value : L"";
    saveStore();
    return OKAY;
}

int regReadKeyWithKeyname_String_Wide(int, wchar_t* path, wchar_t* keyname, int* key_exists,
                                      wchar_t* value, uint32_t buflen)
{
    std::lock_guard<std::mutex> g(lock());
    ensureLoaded();
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
    ensureLoaded();
    auto it = store().find(joinKey(path, keyname));
    if (it == store().end()) { if (key_exists) *key_exists = IS_FALSE; return OKAY; }
    if (key_exists) *key_exists = IS_TRUE;
    if (value) *value = (uint32_t)wcstoul(it->second.c_str(), nullptr, 10);
    return OKAY;
}

int regRecursiveDeleteFolder_Wide(int, wchar_t* path)
{
    std::lock_guard<std::mutex> g(lock());
    ensureLoaded();
    std::wstring prefix = path ? path : L"";
    for (auto it = store().begin(); it != store().end();)
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0) it = store().erase(it);
        else ++it;
    }
    saveStore();
    return OKAY;
}

int regReadRegisteredOwner_Wide(wchar_t* owner, int buflen)
{
    if (owner && buflen > 0) owner[0] = L'\0';
    return OKAY;
}
