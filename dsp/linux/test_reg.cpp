/*
FxSound — Linux build: reg_linux self-test

Exercises the registry.ini-backed key/value store. The store is loaded once per
process, so each scenario runs in its own process: `reg_selftest <scenario>`.
XDG_CONFIG_HOME must point at a scratch directory (set by CTest).
Exit code 0 = pass.
*/

#include "codedefs.h"
#include "reg.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>

static std::filesystem::path regFile()
{
    return std::filesystem::path(getenv("XDG_CONFIG_HOME")) / "fxsound" / "registry.ini";
}

static void writeRegFile(const std::string& contents)
{
    std::filesystem::create_directories(regFile().parent_path());
    std::ofstream f(regFile(), std::ios::trunc | std::ios::binary);
    f << contents;
}

static bool readKey(const wchar_t* key, std::wstring* value)
{
    wchar_t buf[256] = {0};
    int exists = IS_FALSE;
    regReadKey_Wide(0, const_cast<wchar_t*>(key), &exists, buf, 256);
    if (value) *value = buf;
    return exists == IS_TRUE;
}

static bool check(bool cond, const char* what)
{
    std::printf("%s: %s\n", cond ? "ok  " : "FAIL", what);
    return cond;
}

// Values written in one process are read back, including non-ASCII text.
static bool roundtrip()
{
    writeRegFile("");
    regCreateKey_Wide(0, const_cast<wchar_t*>(L"Test\\Name"), const_cast<wchar_t*>(L"café ♫"));
    std::wstring v;
    bool ok = check(readKey(L"Test\\Name", &v), "key exists after create");
    ok &= check(v == L"café ♫", "non-ASCII value round-trips");
    return ok;
}

// A previously written file is parsed on first access.
static bool persisted()
{
    writeRegFile("Test\\Name=caf\xc3\xa9\n");
    std::wstring v;
    bool ok = check(readKey(L"Test\\Name", &v), "key loaded from file");
    ok &= check(v == L"café", "UTF-8 value decoded");
    return ok;
}

// Truncated multi-byte sequences at end of line must not read past the
// string; the rest of the file must still load.
static bool malformedUtf8()
{
    writeRegFile("Bad2=\xc3\n"
                 "Bad3=\xe2\n"
                 "Bad4=\xf0\n"
                 "Good=yes\n");
    std::wstring v;
    bool ok = check(readKey(L"Good", &v) && v == L"yes", "valid line after malformed lines loads");
    ok &= check(readKey(L"Bad3", &v), "malformed line still yields a key");
    return ok;
}

// Deleting folder "P\1" must not delete sibling "P\10".
static bool recursiveDeleteBoundary()
{
    writeRegFile("");
    regCreateKey_Wide(0, const_cast<wchar_t*>(L"P\\1"), const_cast<wchar_t*>(L"a"));
    regCreateKey_Wide(0, const_cast<wchar_t*>(L"P\\1\\x"), const_cast<wchar_t*>(L"b"));
    regCreateKey_Wide(0, const_cast<wchar_t*>(L"P\\10\\x"), const_cast<wchar_t*>(L"c"));
    regRecursiveDeleteFolder_Wide(0, const_cast<wchar_t*>(L"P\\1"));
    bool ok = check(!readKey(L"P\\1", nullptr), "folder key removed");
    ok &= check(!readKey(L"P\\1\\x", nullptr), "child key removed");
    ok &= check(readKey(L"P\\10\\x", nullptr), "sibling with shared prefix kept");
    regRecursiveDeleteFolder_Wide(0, const_cast<wchar_t*>(L"P\\"));
    ok &= check(!readKey(L"P\\10\\x", nullptr), "trailing-separator folder path removes children");
    return ok;
}

int main(int argc, char** argv)
{
    if (!getenv("XDG_CONFIG_HOME")) { std::printf("XDG_CONFIG_HOME not set\n"); return 2; }
    const char* s = argc > 1 ? argv[1] : "";
    bool pass;
    if      (!std::strcmp(s, "roundtrip"))  pass = roundtrip();
    else if (!std::strcmp(s, "persisted"))  pass = persisted();
    else if (!std::strcmp(s, "malformed"))  pass = malformedUtf8();
    else if (!std::strcmp(s, "rmdir"))      pass = recursiveDeleteBoundary();
    else { std::printf("unknown scenario '%s'\n", s); return 2; }
    std::printf("REG SELFTEST %s: %s\n", s, pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
