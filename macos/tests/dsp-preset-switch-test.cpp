#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <limits>
#include <string>
#include <vector>
#define private public
#include "../dsp/float32-engine.h"
#include "../../dsp/u_DfxDsp.h"
#undef private
#include "dfxp.h"
#include "DfxSdk.h"

struct Snapshot {
 std::array<float,5> cached{},knobs{};
 std::array<int,5> buttons{};
 std::vector<float> gains,frequencies;
};
static Snapshot run(const std::filesystem::path& initial,const std::filesystem::path& preset,
                    int bands,bool windowsSequence) {
 fxsound::Float32Engine engine;
 assert(engine.prepare(48000,2));assert(engine.loadPreset(initial));
 auto& controls=engine.controls();controls.setNumBands(bands);
 for(int e=0;e<5;++e)controls.setEffectValue(static_cast<DfxDsp::Effect>(e),10);
 for(int band=0;band<bands;++band)controls.setEqBandBoostCut(band,band%2?8:-8);
 if(windowsSequence) {
  assert(controls.loadPreset(preset.wstring())==0);
  for(int e=0;e<5;++e) {
   auto effect=static_cast<DfxDsp::Effect>(e);
   controls.setEffectValue(effect,controls.getEffectValue(effect)*10);
  }
  for(int band=0;band<controls.getNumEqBands();++band) {
   controls.setEqBandFrequency(band,controls.getEqBandFrequency(band));
   controls.setEqBandBoostCut(band,controls.getEqBandBoostCut(band));
  }
 } else assert(engine.loadPreset(preset.string()));
 Snapshot state;
 const int knobs[]={DFX_UI_KNOB_FIDELITY,DFX_UI_KNOB_AMBIENCE,DFX_UI_KNOB_SURROUND,
                    DFX_UI_KNOB_DYNAMIC_BOOST,DFX_UI_KNOB_BASS_BOOST};
 const int buttons[]={DFX_UI_BUTTON_FIDELITY,DFX_UI_BUTTON_AMBIENCE,DFX_UI_BUTTON_SURROUND,
                      DFX_UI_BUTTON_DYNAMIC_BOOST,DFX_UI_BUTTON_BASS_BOOST};
 for(int e=0;e<5;++e) {
  state.cached[e]=controls.getEffectValue(static_cast<DfxDsp::Effect>(e));
  assert(dfxpGetKnobValue(controls.data_->dfxp_handle_,knobs[e],&state.knobs[e])==0);
  assert(dfxpGetButtonValue(controls.data_->dfxp_handle_,buttons[e],&state.buttons[e])==0);
 }
 for(int band=0;band<controls.getNumEqBands();++band) {
  state.gains.push_back(controls.getEqBandBoostCut(band));
  state.frequencies.push_back(controls.getEqBandFrequency(band));
 }
 auto missing=preset.parent_path()/"__fxsound_missing_test_preset__.fac";
 assert(!std::filesystem::exists(missing));assert(!engine.loadPreset(missing.string()));
 for(int e=0;e<5;++e) {
  float knob=0;int button=0;
  assert(dfxpGetKnobValue(controls.data_->dfxp_handle_,knobs[e],&knob)==0);
  assert(dfxpGetButtonValue(controls.data_->dfxp_handle_,buttons[e],&button)==0);
  assert(knob==state.knobs[e] && button==state.buttons[e]);
  assert(controls.getEffectValue(static_cast<DfxDsp::Effect>(e))==state.cached[e]);
 }
 assert(controls.getNumEqBands()==int(state.gains.size()));
 for(size_t band=0;band<state.gains.size();++band) {
  assert(controls.getEqBandBoostCut(int(band))==state.gains[band]);
  assert(controls.getEqBandFrequency(int(band))==state.frequencies[band]);
 }
 return state;
}
int main(int argc,char** argv) {
 assert(argc==2);std::filesystem::path factory(argv[1]);int cases=0,failures=0;
 for(auto& file:std::filesystem::directory_iterator(factory)) {
  if(file.path().extension()!=".fac")continue;
  for(int bands:{5,10,31}) {
   // Original DSP has global state; construct/configure/snapshot instances serially.
   const auto actual=run(factory/"Default.fac",file.path(),bands,false);
   const auto expected=run(factory/"Default.fac",file.path(),bands,true);
   for(int e=0;e<5;++e)
    assert(std::abs(actual.cached[e]-expected.cached[e])<=2*std::numeric_limits<float>::epsilon());
   bool match=actual.knobs==expected.knobs && actual.buttons==expected.buttons
       && actual.gains==expected.gains && actual.frequencies==expected.frequencies;
   std::cout<<file.path().filename()<<" bands="<<bands<<" cached B float-roundtrip match=1 DSP knobs/buttons/EQ match="<<match<<'\n';
   if(!match) {
    ++failures;
    for(int e=0;e<5;++e)std::cout<<" effect"<<e<<" cached="<<actual.cached[e]
      <<" active knob="<<actual.knobs[e]<<" reference="<<expected.knobs[e]
      <<" active button="<<actual.buttons[e]<<" reference="<<expected.buttons[e]<<'\n';
   }
   ++cases;
  }
 }
 assert(cases==39);
 std::cout<<"Actual preset replacement after edited A: "<<cases<<" cases, "<<failures
          <<" failures; missing preset rejected without mutation; native original Windows controller sequence reference, not Windows compiler parity\n";
 return failures?1:0;
}
