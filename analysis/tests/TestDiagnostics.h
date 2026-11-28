// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <initializer_list>
#ifdef _MSC_VER
#include <crtdbg.h>
#include <cstdlib>
#endif

inline void configureNonInteractiveDiagnostics()
{
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(_WRITE_ABORT_MSG, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    for (const int report : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
    {
        _CrtSetReportMode(report, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(report, _CRTDBG_FILE_STDERR);
    }
#endif
}
