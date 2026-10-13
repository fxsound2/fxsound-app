#include "state-validation.h"
#include <cmath>
namespace fxgui {
static bool number(const juce::var& value) {
    return (value.isInt() || value.isInt64() || value.isDouble()) && std::isfinite(double(value));
}
static bool arrayOfNumbers(const juce::var& value,int count,double minimum,double maximum) {
    if(!value.isArray() || value.size()!=count)return false;
    for(const auto& item:*value.getArray())if(!number(item) || double(item)<minimum || double(item)>maximum)return false;
    return true;
}
bool validState(const juce::var& data) {
    auto controls=data["controls"],metrics=data["metrics"],count=controls["numEqBands"];
    if(!(count.isInt() || count.isInt64()))return false;
    int bands=int(count);if(bands!=5 && bands!=10 && bands!=15 && bands!=20 && bands!=31)return false;
    auto ranges=controls["frequencyRanges"];
    if(!ranges.isArray() || ranges.size()!=bands)return false;
    for(auto range:*ranges.getArray())if(!arrayOfNumbers(range,2,1,24000) || double(range[0])>double(range[1]))return false;
    auto scalar=[&](const char* key,double lo,double hi){auto v=controls[key];return number(v) && double(v)>=lo && double(v)<=hi;};
    return data.isObject() && controls.isObject() && metrics.isObject() &&
        controls["dspAvailable"].isBool() && controls["bypassed"].isBool() &&
        data["outputUID"].isString() && data["outputUID"].toString().isNotEmpty() &&
        data["routingActive"].isBool() && data["parityVerified"].isBool() && metrics["ready"].isBool() &&
        number(metrics["peakLeft"]) && double(metrics["peakLeft"])>=0 &&
        number(metrics["peakRight"]) && double(metrics["peakRight"])>=0 &&
        arrayOfNumbers(controls["effects"],5,0,10) && arrayOfNumbers(controls["eq"],bands,-12,12) &&
        arrayOfNumbers(controls["frequencies"],bands,1,24000) && arrayOfNumbers(controls["spectrum"],10,0,1) &&
        scalar("masterGain",-20,20) && scalar("balance",-20,20) && scalar("volumeLeveling",0,4) && scalar("filterQ",1,3);
}
}
