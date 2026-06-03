/*
FxSound — Linux build (M0 bootstrap stub)

Stub implementation of the DfxDsp public API (dsp/include/DfxDsp.h) so the JUCE
GUI compiles and launches on Linux before the real DSP is ported (M1).
Every method returns a neutral/no-op value. processAudio() copies input to
output (bit-perfect passthrough) so audio is never corrupted while stubbed.
*/

#include "DfxDsp.h"
#include <cstring>

class DfxDspPrivate {}; // opaque, unused in the stub

DfxDsp::DfxDsp() : data_(nullptr) {}
DfxDsp::~DfxDsp() {}

int DfxDsp::setSignalFormat(int, int, int, int) { return 0; }

int DfxDsp::processAudio(short int* in, short int* out, int num_sample_sets, int)
{
    if (in && out && num_sample_sets > 0)
        std::memcpy(out, in, static_cast<size_t>(num_sample_sets) * 2 * sizeof(short int));
    return num_sample_sets;
}

int   DfxDsp::loadPreset(std::wstring)                             { return 0; }
int   DfxDsp::savePreset(std::wstring, std::wstring)              { return 0; }
int   DfxDsp::exportPreset(std::wstring, std::wstring, std::wstring) { return 0; }
void  DfxDsp::eqOn(bool)                                          {}
int   DfxDsp::getNumEqBands()                                     { return 10; }
float DfxDsp::getBalance()                                        { return 0.0f; }
void  DfxDsp::setBalance(float)                                   {}
float DfxDsp::getNormalization()                                  { return 0.0f; }
void  DfxDsp::setNormalization(float)                             {}
float DfxDsp::getMasterGain()                                     { return 0.0f; }
void  DfxDsp::setMasterGain(float)                                {}
float DfxDsp::getFilterQ()                                        { return 1.0f; }
void  DfxDsp::setFilterQ(float)                                   {}
void  DfxDsp::setNumBands(int)                                    {}
float DfxDsp::getEqBandFrequency(int band_num)                    { return 31.25f * (1 << band_num); }
void  DfxDsp::setEqBandFrequency(int, float)                      {}
void  DfxDsp::getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq)
{
    // Return a valid (non-empty) range so the GUI's NormalisableRange is sane.
    const float center = 31.25f * (1 << band_num);
    if (min_freq) *min_freq = center * 0.5f;
    if (max_freq) *max_freq = center * 2.0f;
}
float DfxDsp::getEqBandBoostCut(int)                              { return 0.0f; }
void  DfxDsp::setEqBandBoostCut(int, float)                       {}
void  DfxDsp::powerOn(bool)                                       {}
bool  DfxDsp::isPowerOn()                                         { return true; }
float DfxDsp::getEffectValue(Effect)                              { return 0.0f; }
void  DfxDsp::setEffectValue(Effect, float)                       {}
DfxPreset DfxDsp::getPresetInfo(std::wstring)                     { return DfxPreset{}; }
unsigned long DfxDsp::getTotalAudioProcessedTime()               { return 0; }
void  DfxDsp::resetTotalAudioProcessedTime()                     {}
void  DfxDsp::getSpectrumBandValues(float* values, int size)
{
    if (values)
        for (int i = 0; i < size; ++i) values[i] = 0.0f;
}
