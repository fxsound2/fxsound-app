/*
FxSound — Linux build: DfxDsp self-test

Drives the ported DSP without any audio I/O: feeds a synthetic signal through
processAudio() and checks the engine is actually running (power on/off changes
the output, EQ boost changes the output). Exit code 0 = pass.
*/

#include "DfxDsp.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static std::vector<short> makeSine(int frames, double freq, int rate)
{
    std::vector<short> buf(frames * 2);
    for (int i = 0; i < frames; ++i)
    {
        double s = std::sin(2.0 * M_PI * freq * i / rate);
        short v = (short)(s * 12000.0);
        buf[i * 2] = v;
        buf[i * 2 + 1] = v;
    }
    return buf;
}

static double rms(const std::vector<short>& a, const std::vector<short>& b)
{
    double acc = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
    {
        double d = (double)a[i] - (double)b[i];
        acc += d * d;
    }
    return std::sqrt(acc / a.size());
}

int main()
{
    const int rate = 48000, frames = 4096;
    DfxDsp dsp;
    int rc = dsp.setSignalFormat(16, 2, rate, 16);
    std::printf("setSignalFormat rc=%d, eq bands=%d\n", rc, dsp.getNumEqBands());

    auto in = makeSine(frames, 440.0, rate);

    // 1) Powered on, flat — process should succeed and produce output.
    dsp.powerOn(true);
    std::vector<short> out_on(in.size(), 0);
    dsp.processAudio(in.data(), out_on.data(), frames, 0);

    // 2) Powered off — output should differ from powered-on output.
    dsp.powerOn(false);
    std::vector<short> out_off(in.size(), 0);
    dsp.processAudio(in.data(), out_off.data(), frames, 0);

    double diff_power = rms(out_on, out_off);
    std::printf("RMS(out_on vs out_off) = %.2f\n", diff_power);

    // 3) Powered on with a big bass EQ boost — output should change again.
    dsp.powerOn(true);
    dsp.setEqBandBoostCut(1, 12.0f);
    std::vector<short> out_eq(in.size(), 0);
    for (int k = 0; k < 8; ++k)               // let the EQ settle across blocks
        dsp.processAudio(in.data(), out_eq.data(), frames, 0);
    double diff_eq = rms(out_eq, out_on);
    std::printf("RMS(out_eq vs out_on)  = %.2f\n", diff_eq);

    bool out_nonzero = false;
    for (short s : out_on) if (s != 0) { out_nonzero = true; break; }

    bool pass = out_nonzero && diff_power > 1.0 && diff_eq > 1.0;
    std::printf("%s\n", pass ? "DSP SELFTEST: PASS" : "DSP SELFTEST: FAIL");
    return pass ? 0 : 1;
}
