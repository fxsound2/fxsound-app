/*
 * Portability smoke test for DfxKeyValueStore's non-Windows implementation.
 * Not a build of the full DfxDsp engine -- see dsp/CMakeLists.txt.
 */
#include "DfxKeyValueStore.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

static char g_test_home[] = "/tmp/DfxKeyValueStoreTestXXXXXX";

static void ExpectString(const wchar_t *actual, const wchar_t *expected, const char *what)
{
	if (wcscmp(actual, expected) != 0)
	{
		fwprintf(stderr, L"FAIL (%hs): expected \"%ls\", got \"%ls\"\n", what, expected, actual);
		exit(1);
	}
}

int main()
{
	char *tmp_dir = mkdtemp(g_test_home);
	assert(tmp_dir != nullptr);
	setenv("HOME", tmp_dir, 1);

	wchar_t buffer[256];
	int key_exists;

	/* Round-trip write/read, mirroring DfxDspRegistry.cpp's usage. */
	assert(dfxKvWriteString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\1\\1234\\LASTUSED_DFXG\\eq_bypass", (wchar_t *)L"42") == OKAY);
	assert(dfxKvReadString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\1\\1234\\LASTUSED_DFXG\\eq_bypass", &key_exists, buffer, 256) == OKAY);
	assert(key_exists == IS_TRUE);
	ExpectString(buffer, L"42", "round-trip write/read");

	/* Missing key. */
	assert(dfxKvReadString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\does\\not\\exist", &key_exists, buffer, 256) == OKAY);
	assert(key_exists == IS_FALSE);

	/* REG_CURRENT_USER and REG_LOCAL_MACHINE namespaces don't collide. */
	assert(dfxKvWriteString_Wide(REG_LOCAL_MACHINE, (wchar_t *)L"SOFTWARE\\FxSound\\shared_path", (wchar_t *)L"machine_value") == OKAY);
	assert(dfxKvWriteString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\shared_path", (wchar_t *)L"user_value") == OKAY);
	assert(dfxKvReadString_Wide(REG_LOCAL_MACHINE, (wchar_t *)L"SOFTWARE\\FxSound\\shared_path", &key_exists, buffer, 256) == OKAY);
	ExpectString(buffer, L"machine_value", "REG_LOCAL_MACHINE namespace");
	assert(dfxKvReadString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\shared_path", &key_exists, buffer, 256) == OKAY);
	ExpectString(buffer, L"user_value", "REG_CURRENT_USER namespace");

	/* Recursive delete only removes entries under the given prefix. */
	assert(dfxKvWriteString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\1\\1234\\LASTUSED_DFXG\\ambience", (wchar_t *)L"7") == OKAY);
	assert(dfxKvWriteString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\OtherKey", (wchar_t *)L"unaffected") == OKAY);
	assert(dfxKvRecursiveDeleteFolder_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\1\\1234\\LASTUSED_DFXG") == OKAY);
	assert(dfxKvReadString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\1\\1234\\LASTUSED_DFXG\\eq_bypass", &key_exists, buffer, 256) == OKAY);
	assert(key_exists == IS_FALSE);
	assert(dfxKvReadString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\1\\1234\\LASTUSED_DFXG\\ambience", &key_exists, buffer, 256) == OKAY);
	assert(key_exists == IS_FALSE);
	assert(dfxKvReadString_Wide(REG_CURRENT_USER, (wchar_t *)L"SOFTWARE\\FxSound\\OtherKey", &key_exists, buffer, 256) == OKAY);
	assert(key_exists == IS_TRUE);
	ExpectString(buffer, L"unaffected", "sibling key survives recursive delete");

	printf("All DfxKeyValueStore tests passed.\n");
	return 0;
}
