/*
FxSound — Linux build: DSP settings self-test

Settings are persisted per key in registry.ini. Each control must have its own
key: turning one effect off must not change the power (bypass) state or any
other effect, and every value must read back as written. Exit code 0 = pass.
*/

#include "DfxDsp.h"
#include "PipeWireProcess.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <string>
#include <vector>

static bool check(bool cond, const char* what)
{
    std::printf("%s: %s\n", cond ? "ok  " : "FAIL", what);
    return cond;
}

// RMS ratio (dB) of processed vs input for a 1 kHz tone at -15 dBFS.
static double gainDb(FxProcessor& p)
{
    const uint32_t n = 4096;
    std::vector<float> l(n), r(n), ol(n), orr(n);
    for (uint32_t i = 0; i < n; ++i) l[i] = r[i] = 0.25f * (float)std::sin(2.0 * M_PI * 1000.0 * i / 48000);
    const float* in[2] = {l.data(), r.data()};
    float* out[2] = {ol.data(), orr.data()};
    for (int k = 0; k < 24; ++k) p.process(in, out, n, 48000);   // let the DSP settle
    double a = 0, b = 0;
    for (uint32_t i = n / 2; i < n; ++i) { a += (double)ol[i] * ol[i]; b += (double)l[i] * l[i]; }
    return 10.0 * std::log10(a / b);
}

int main()
{
    if (!getenv("XDG_CONFIG_HOME")) { std::printf("XDG_CONFIG_HOME not set\n"); return 2; }
    bool ok = true;

    DfxDsp dsp;
    FxProcessor p;
    p.setDsp(&dsp);
    dsp.powerOn(true);
    for (int e = 0; e < DfxDsp::NumEffects; ++e) dsp.setEffectValue((DfxDsp::Effect)e, 5.0f);
    double on_db = gainDb(p);
    std::printf("all effects at 5, power on: %+.2f dB\n", on_db);

    // Turning effects off one by one must leave power on.
    for (int e = 0; e < DfxDsp::NumEffects; ++e) {
        dsp.setEffectValue((DfxDsp::Effect)e, 0.0f);
        char what[96];
        std::snprintf(what, sizeof(what), "power stays on after turning effect %d off", e);
        ok &= check(dsp.isPowerOn(), what);
    }
    // ... and the remaining processing (EQ, etc.) still runs: not bit-transparent.
    dsp.setEffectValue(DfxDsp::Fidelity, 5.0f);
    double partial_db = gainDb(p);
    std::printf("fidelity only, power on: %+.2f dB\n", partial_db);
    ok &= check(std::fabs(partial_db) > 0.05, "processing active with one effect on");

    // Power off is a clean bypass; power on restores processing.
    dsp.powerOn(false);
    double off_db = gainDb(p);
    ok &= check(!dsp.isPowerOn() && std::fabs(off_db) < 0.05, "power off is unity-gain bypass");
    dsp.powerOn(true);
    ok &= check(dsp.isPowerOn() && std::fabs(gainDb(p) - partial_db) < 0.1, "power on restores processing");

    // Each effect keeps its own value (getEffectValue is 0..1; the GUI scales by 10).
    const float vals[DfxDsp::NumEffects] = {1.0f, 2.0f, 3.0f, 4.0f, 6.0f};
    for (int e = 0; e < DfxDsp::NumEffects; ++e) dsp.setEffectValue((DfxDsp::Effect)e, vals[e]);
    for (int e = 0; e < DfxDsp::NumEffects; ++e) {
        char what[96];
        float got = dsp.getEffectValue((DfxDsp::Effect)e) * 10.0f;
        std::snprintf(what, sizeof(what), "effect %d reads back %.0f (got %.2f)", e, vals[e], got);
        ok &= check(std::fabs(got - vals[e]) < 0.06, what);
    }

    // Distinct controls are stored under distinct keys (no one-letter names).
    std::ifstream f(std::string(getenv("XDG_CONFIG_HOME")) + "/fxsound/registry.ini");
    std::set<std::string> leaf;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('='), bs = line.rfind('\\', eq);
        if (eq != std::string::npos && bs != std::string::npos) leaf.insert(line.substr(bs + 1, eq - bs - 1));
    }
    size_t one_char = 0;
    for (auto& k : leaf) one_char += k.size() == 1;
    std::printf("registry leaf keys: %zu, one-character: %zu\n", leaf.size(), one_char);
    ok &= check(leaf.size() >= 10 && one_char == 0, "settings use full, distinct key names");

    // A saved user preset keeps its full name and values.
    std::string dir = std::string(getenv("XDG_CONFIG_HOME")) + "/fxsound";
    std::wstring wdir(dir.begin(), dir.end());           // savePreset takes the directory,
    std::wstring path = wdir + L"/My Late Night Mix.fac";  // like FxController::savePreset
    dsp.setEffectValue(DfxDsp::Bass, 7.0f);
    dsp.setEqBandBoostCut(2, 4.5f);
    ok &= check(dsp.savePreset(L"My Late Night Mix", wdir) == 0, "savePreset returns OK");
    ok &= check(std::ifstream(std::string(path.begin(), path.end())).good(), "preset file is <dir>/<name>.fac");
    DfxPreset info = dsp.getPresetInfo(path);
    std::printf("saved preset name read back: '%ls'\n", info.name.c_str());
    ok &= check(info.name == L"My Late Night Mix", "preset name round-trips");
    dsp.setEffectValue(DfxDsp::Bass, 1.0f);
    dsp.setEqBandBoostCut(2, 0.0f);
    dsp.loadPreset(path);
    ok &= check(std::fabs(dsp.getEffectValue(DfxDsp::Bass) * 10.0f - 7.0f) < 0.06, "preset restores effect value");
    ok &= check(std::fabs(dsp.getEqBandBoostCut(2) - 4.5f) < 0.06, "preset restores eq band");

    // Non-ASCII names are stored as UTF-8 and read back unchanged.
    const std::wstring intl = L"Soirée ♫ café";
    dsp.savePreset(intl, wdir);
    DfxPreset intl_info = dsp.getPresetInfo(wdir + L"/" + intl + L".fac");
    ok &= check(intl_info.name == intl, "non-ASCII preset name round-trips");

    std::printf("DSP SETTINGS SELFTEST: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
