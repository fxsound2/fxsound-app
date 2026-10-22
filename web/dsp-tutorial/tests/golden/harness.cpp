// Golden-reference harness: runs float32 stereo audio through the REAL DfxDsp engine
// (built from origin/main) so the JS tutorial ports can be diffed against it.
//
// Usage: harness.exe in.f32 out.f32 key=value ...
//   power=0|1 eq=0|1 gains=g1,...,g10 mg=dB bal=dB q=mult vl=0..4 norm=dB
//   fid=0..10 amb=0..10 sur=0..10 dyn=0..10 bass=0..10 frames=441
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "DfxDsp.h"

static std::map<std::string, std::string> parseArgs(int argc, char** argv)
{
    std::map<std::string, std::string> kv;
    for (int i = 3; i < argc; i++)
    {
        const char* eq = strchr(argv[i], '=');
        if (eq) kv[std::string(argv[i], eq - argv[i])] = std::string(eq + 1);
    }
    return kv;
}

static float getF(std::map<std::string, std::string>& kv, const char* k, float def)
{
    auto it = kv.find(k);
    return it == kv.end() ? def : (float)atof(it->second.c_str());
}

int main(int argc, char** argv)
{
    if (argc < 3) { fprintf(stderr, "usage: harness in.f32 out.f32 key=value...\n"); return 1; }
    auto kv = parseArgs(argc, argv);

    FILE* fi = fopen(argv[1], "rb");
    if (!fi) { fprintf(stderr, "cannot open input\n"); return 1; }
    fseek(fi, 0, SEEK_END); long bytes = ftell(fi); fseek(fi, 0, SEEK_SET);
    std::vector<float> in(bytes / sizeof(float));
    fread(in.data(), sizeof(float), in.size(), fi); fclose(fi);
    std::vector<float> out(in.size(), 0.0f);

    const int channels = 2;
    const int frames = (int)getF(kv, "frames", 441);

    DfxDsp dsp;
    dsp.setSignalFormat(32, channels, 44100, 32);

    // Mirror FxController's order: global EQ settings, band layout, band gains, then effects.
    dsp.powerOn(getF(kv, "power", 1) != 0);
    dsp.eqOn(getF(kv, "eq", 1) != 0);
    dsp.setFilterQ(getF(kv, "q", 1.0f));
    // The engine initialises its EQ from the registry (possibly 31 bands of old values).
    // Zero all 31 sections first so the reference is deterministic, unless stale=1 is
    // passed to reproduce what a real user who switched from 31 -> 10 bands would hear.
    if (getF(kv, "stale", 0) == 0)
    {
        dsp.setNumBands(31);
        for (int b = 0; b < 31; b++) dsp.setEqBandBoostCut(b, 0.0f);
    }
    dsp.setNumBands(10);
    std::vector<float> gains(10, 0.0f);
    if (kv.count("gains"))
    {
        std::string s = kv["gains"]; size_t pos = 0; int b = 0;
        while (b < 10 && pos <= s.size())
        {
            size_t c = s.find(',', pos);
            gains[b++] = (float)atof(s.substr(pos, c == std::string::npos ? std::string::npos : c - pos).c_str());
            if (c == std::string::npos) break;
            pos = c + 1;
        }
    }
    for (int b = 0; b < 10; b++) dsp.setEqBandBoostCut(b, gains[b]);
    dsp.setMasterGain(getF(kv, "mg", 0));
    dsp.setBalance(getF(kv, "bal", 0));
    dsp.setNormalization(getF(kv, "norm", 0));
    dsp.setVolumeLeveling(getF(kv, "vl", 0));
    dsp.setEffectValue(DfxDsp::Fidelity, getF(kv, "fid", 0));
    dsp.setEffectValue(DfxDsp::Ambience, getF(kv, "amb", 0));
    dsp.setEffectValue(DfxDsp::Surround, getF(kv, "sur", 0));
    dsp.setEffectValue(DfxDsp::DynamicBoost, getF(kv, "dyn", 0));
    dsp.setEffectValue(DfxDsp::Bass, getF(kv, "bass", 0));

    const size_t totalFrames = in.size() / channels;
    std::vector<float> bufIn(frames * channels), bufOut(frames * channels);
    for (size_t f0 = 0; f0 < totalFrames; f0 += frames)
    {
        int n = (int)((totalFrames - f0) < (size_t)frames ? (totalFrames - f0) : frames);
        memcpy(bufIn.data(), &in[f0 * channels], n * channels * sizeof(float));
        dsp.processAudio((short int*)bufIn.data(), (short int*)bufOut.data(), n, 0);
        memcpy(&out[f0 * channels], bufOut.data(), n * channels * sizeof(float));
    }

    FILE* fo = fopen(argv[2], "wb");
    fwrite(out.data(), sizeof(float), out.size(), fo); fclose(fo);
    printf("processed %zu frames\n", totalFrames);
    return 0;
}
