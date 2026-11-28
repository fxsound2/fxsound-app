// SPDX-License-Identifier: AGPL-3.0-or-later
#include "TestSupport.h"
#include "TestDiagnostics.h"
#include <iostream>

namespace fxanalysis::test
{
void runPitchMotionTests(const Check&);
void runSpectralCorrelationTests(const Check&);
void runHarmonicCombTests(const Check&);
void runAnalysisRequestTests(const Check&);
}
void runWaterfallTests(const std::function<void(const char*, bool)>&);

int main()
{
    configureNonInteractiveDiagnostics();
    int checks = 0, failures = 0;
    std::cout << "{\"tests\":[";
    const fxanalysis::test::Check check = [&](const char* name, bool passed)
    {
        if (checks++) std::cout << ',';
        std::cout << "{\"name\":\"" << name << "\",\"passed\":" << (passed ? "true" : "false") << '}';
        if (!passed) ++failures;
    };
    fxanalysis::test::runPitchMotionTests(check);
    fxanalysis::test::runSpectralCorrelationTests(check);
    fxanalysis::test::runHarmonicCombTests(check);
    runWaterfallTests(check);
    fxanalysis::test::runAnalysisRequestTests(check);
    std::cout << "],\"checks\":" << checks << ",\"failures\":" << failures
        << ",\"valid\":" << (failures == 0 ? "true" : "false") << "}\n";
    return failures ? 1 : 0;
}
