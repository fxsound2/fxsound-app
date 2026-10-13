#include "dsp-controller.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#ifdef FXSOUND_HAVE_DSP
#include "float32-engine.h"
#endif
namespace fxsound {
namespace {
const std::array<std::string,5> effects{"fidelity","ambience","surround","dynamicBoost","bass"};
int band(const std::string& name, const std::string& prefix) {
    if (name.rfind(prefix,0) != 0) return -1;
    const auto suffix = name.substr(prefix.size());
    if (suffix.empty() || suffix.size()>2 || suffix[0]=='0' ||
        !std::all_of(suffix.begin(),suffix.end(),[](char c){return c>='0' && c<='9';})) return -1;
    const int number = std::stoi(suffix);
    return number>=1 && number<=31 ? number-1 : -1;
}
void range(double value,double low,double high) {
    if(value<low || value>high) throw std::runtime_error("parameter outside allowed range");
}
}
void DSPController::validateParameter(const std::string& name,double value) {
    if(!std::isfinite(value)) throw std::runtime_error("parameter must be finite");
    if(std::find(effects.begin(),effects.end(),name)!=effects.end()) return range(value,0,10);
    if(band(name,"eq")>=0) return range(value,-12,12);
    if(band(name,"freq")>=0) return range(value,20,20000);
    if(name=="masterGain" || name=="balance") return range(value,-20,20);
    if(name=="volumeLeveling") return range(value,0,4);
    if(name=="filterQ") return range(value,1,3);
    throw std::runtime_error("unknown parameter");
}
void DSPController::validateCurrentParameter(const std::string& name,double value) const {
    validateParameter(name,value);
#ifdef FXSOUND_HAVE_DSP
    auto& c=dsp_->controls();
    int index=band(name,"eq");
    if(index>=c.getNumEqBands()) throw std::runtime_error("EQ band outside current count");
    index=band(name,"freq");
    if(index>=0) {
        if(index>=c.getNumEqBands()) throw std::runtime_error("frequency band outside current count");
        float low=0,high=0;c.getEqBandFrequencyRange(index,&low,&high);
        range(value,low,high);
    }
#else
    throw std::runtime_error("DSP unavailable in this build");
#endif
}
void DSPController::parameter(const std::string& name,double value) {
    validateCurrentParameter(name,value);
#ifdef FXSOUND_HAVE_DSP
    auto& c=dsp_->controls();
    for(size_t i=0;i<effects.size();++i) if(name==effects[i]) {
        c.setEffectValue(static_cast<DfxDsp::Effect>(i),float(value));return;
    }
    int index=band(name,"eq");
    if(index>=0) {c.setEqBandBoostCut(index,float(value));return;}
    index=band(name,"freq");
    if(index>=0) {c.setEqBandFrequency(index,float(value));return;}
    if(name=="masterGain") c.setMasterGain(float(value));
    else if(name=="balance") c.setBalance(float(value));
    else if(name=="volumeLeveling") c.setVolumeLeveling(float(value));
    else if(name=="filterQ") c.setFilterQ(float(value));
#endif
}
void DSPController::numBands(int count) {
    if(count!=5 && count!=10 && count!=15 && count!=20 && count!=31)
        throw std::runtime_error("EQ count must be 5,10,15,20 or31");
#ifdef FXSOUND_HAVE_DSP
    dsp_->controls().setNumBands(count);
    if(dsp_->controls().getNumEqBands()!=count) throw std::runtime_error("EQ resize failed");
#else
    throw std::runtime_error("DSP unavailable in this build");
#endif
}
void DSPController::refreshState() {
    std::ostringstream out;
    out<<"{\"dspAvailable\":"<<(available()?"true":"false")
       <<",\"effects\":[";
#ifdef FXSOUND_HAVE_DSP
    auto& c=dsp_->controls();int count=c.getNumEqBands();
    for(int i=0;i<5;++i) {if(i)out<<',';out<<c.getEffectValue(static_cast<DfxDsp::Effect>(i))*10;}
#else
    int count=0;
#endif
    out<<"],\"numEqBands\":"<<count<<",\"eq\":[";
#ifdef FXSOUND_HAVE_DSP
    for(int i=0;i<count;++i) {if(i)out<<',';out<<c.getEqBandBoostCut(i);}
#endif
    out<<"],\"frequencies\":[";
#ifdef FXSOUND_HAVE_DSP
    for(int i=0;i<count;++i) {if(i)out<<',';out<<c.getEqBandFrequency(i);}
#endif
    out<<"],\"frequencyRanges\":[";
#ifdef FXSOUND_HAVE_DSP
    for(int i=0;i<count;++i) {float low=0,high=0;c.getEqBandFrequencyRange(i,&low,&high);if(i)out<<',';out<<'['<<low<<','<<high<<']';}
#endif
#ifdef FXSOUND_HAVE_DSP
    out<<"],\"masterGain\":"<<c.getMasterGain()<<",\"balance\":"<<c.getBalance()
       <<",\"volumeLeveling\":"<<c.getVolumeLeveling()<<",\"filterQ\":"<<c.getFilterQ();
#else
    out<<"],\"masterGain\":0,\"balance\":0,\"volumeLeveling\":0,\"filterQ\":1";
#endif
    std::lock_guard<std::mutex> lock(stateMutex_);
    controlsJSON_=out.str();
}
std::string DSPController::stateJSON() const {
    std::ostringstream out;
    { std::lock_guard<std::mutex> lock(stateMutex_);out<<controlsJSON_; }
    out<<",\"bypassed\":"<<(bypassed()?"true":"false")<<",\"spectrum\":[";
#ifdef FXSOUND_HAVE_DSP
    float spectrum[10]{};dsp_->controls().getSpectrumBandValues(spectrum,10);
    for(int i=0;i<10;++i) {if(i)out<<',';out<<spectrum[i];}
#endif
    out<<"]}";return out.str();
}
}
