/*
FxSound — Linux build: realtime process-block self-test

Drives FxProcessor (the body of the PipeWire realtime callback) with the real
DfxDsp and synthetic float signals. Checks the block never allocates (malloc
is counted while processing), that output stays finite and within [-1, 1], and
the plain-copy / silence paths. Exit code 0 = pass.
*/

#include "PipeWireProcess.h"
#include "DfxDsp.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// ---- allocation counter -------------------------------------------------
// Replaced malloc forwards to glibc. Sanitizers own malloc, so under ASan the
// counter is compiled out and the allocation checks are reported as skipped.
static thread_local bool g_counting = false;
static thread_local int  g_allocs = 0;
#if defined(__SANITIZE_ADDRESS__)
static constexpr bool kCountAllocs = false;
#else
static constexpr bool kCountAllocs = true;
extern "C" void* __libc_malloc(size_t);
extern "C" void* malloc(size_t n)
{
    if (g_counting) ++g_allocs;
    return __libc_malloc(n);
}
#endif

static bool check(bool cond, const char* what)
{
    if (!kCountAllocs && std::strstr(what, "allocate")) {
        std::printf("skip: %s (allocation counter disabled under sanitizer)\n", what);
        return true;
    }
    std::printf("%s: %s\n", cond ? "ok  " : "FAIL", what);
    return cond;
}

struct Stereo {
    std::vector<float> l, r;
    explicit Stereo(uint32_t n, float amp = 0.f, double freq = 440.0, int rate = 48000) : l(n), r(n)
    {
        for (uint32_t i = 0; i < n; ++i)
            l[i] = r[i] = (float)(amp * std::sin(2.0 * M_PI * freq * i / rate));
    }
};

static int run(FxProcessor& p, Stereo& in, Stereo& out, uint32_t n, int rate)
{
    const float* ip[2] = {in.l.data(), in.r.data()};
    float* op[2] = {out.l.data(), out.r.data()};
    g_allocs = 0;
    g_counting = true;
    p.process(ip, op, n, rate);
    g_counting = false;
    return g_allocs;
}

static bool finiteAndBounded(const Stereo& s)
{
    for (size_t i = 0; i < s.l.size(); ++i)
        for (float v : {s.l[i], s.r[i]})
            if (!std::isfinite(v) || v > 1.f || v < -1.f) return false;
    return true;
}

int main()
{
    bool ok = true;

    // No DSP attached: output is an exact copy of the input.
    {
        FxProcessor p;
        Stereo in(256, 0.5f), out(256);
        run(p, in, out, 256, 48000);
        ok &= check(out.l == in.l && out.r == in.r, "no dsp: output copies input");
    }

    DfxDsp dsp;
    dsp.powerOn(true);
    FxProcessor p;
    p.setDsp(&dsp);

    // Warm up at 48 kHz, then no allocation on steady-state blocks.
    {
        Stereo in(1024, 0.5f), out(1024);
        run(p, in, out, 1024, 48000);
        int a = run(p, in, out, 1024, 48000);
        std::printf("allocations in steady-state block: %d\n", a);
        ok &= check(a == 0, "steady-state block does not allocate");
    }

    // A quantum larger than any seen before must not allocate either.
    {
        Stereo in(8192, 0.5f), out(8192);
        int a = run(p, in, out, 8192, 48000);
        std::printf("allocations in first 8192-frame block: %d\n", a);
        ok &= check(a == 0, "larger quantum does not allocate");
        ok &= check(finiteAndBounded(out), "8192-frame output finite and within [-1, 1]");
    }

    // Graph rate change handled without allocating.
    {
        Stereo in(512, 0.5f, 440.0, 44100), out(512);
        int a = run(p, in, out, 512, 44100);
        std::printf("allocations on rate change: %d\n", a);
        ok &= check(a == 0, "rate change does not allocate");
    }

    // Full-scale input with DSP gain: hard clip keeps output in range.
    {
        Stereo in(4096, 1.0f, 60.0), out(4096);
        for (int k = 0; k < 4; ++k) run(p, in, out, 4096, 48000);
        ok &= check(finiteAndBounded(out), "full-scale input stays finite and within [-1, 1]");
    }

    // Silence in, silence (or near-silence) out once the DSP has settled.
    {
        Stereo in(4096), out(4096);
        for (int k = 0; k < 16; ++k) run(p, in, out, 4096, 48000);
        float peak = 0.f;
        for (size_t i = 0; i < out.l.size(); ++i)
            peak = std::fmax(peak, std::fmax(std::fabs(out.l[i]), std::fabs(out.r[i])));
        std::printf("peak after silence: %g\n", peak);
        ok &= check(peak < 1e-4f, "silence in gives silence out");
    }

    // Muted: zeros regardless of input.
    {
        Stereo in(256, 0.5f), out(256, 0.25f);
        p.setMuted(true);
        run(p, in, out, 256, 48000);
        p.setMuted(false);
        bool zero = true;
        for (size_t i = 0; i < 256; ++i) zero &= out.l[i] == 0.f && out.r[i] == 0.f;
        ok &= check(zero, "muted output is silent");
    }

    std::printf("PROCESS SELFTEST: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
