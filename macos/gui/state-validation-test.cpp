#include "state-validation.h"
#include <limits>
#include <iostream>
using namespace juce;
static var object(){return var(new DynamicObject());}
static void put(var value,const char* name,var property){value.getDynamicObject()->setProperty(name,property);}
static var array(int count,double value){Array<var> result;for(int i=0;i<count;++i)result.add(value);return var(result);}
static var state(int count){
 auto data=object(),controls=object(),metrics=object();
 put(data,"controls",controls);put(data,"metrics",metrics);put(data,"outputUID","physical-device");put(data,"routingActive",false);put(data,"parityVerified",false);
 put(metrics,"ready",true);put(metrics,"peakLeft",0.25);put(metrics,"peakRight",0.5);
 put(controls,"dspAvailable",true);put(controls,"bypassed",true);put(controls,"numEqBands",count);
 put(controls,"effects",array(5,0));put(controls,"eq",array(count,0));put(controls,"frequencies",array(count,1000));put(controls,"spectrum",array(10,0));
 Array<var> ranges;for(int i=0;i<count;++i){Array<var> pair;pair.add(20);pair.add(20000);ranges.add(var(pair));}put(controls,"frequencyRanges",var(ranges));
 put(controls,"masterGain",0);put(controls,"balance",0);put(controls,"volumeLeveling",0);put(controls,"filterQ",1);return data;
}
int main(){
 int checked=0,failed=0;auto expect=[&](var value,bool valid){++checked;if(fxgui::validState(value)!=valid)++failed;};
 for(int count:{5,10,15,20,31})expect(state(count),true);
 for(int count:{0,9,11,32})expect(state(count),false);
 auto mutate=[&](const char* field,var value){auto s=state(10);put(s["controls"],field,value);expect(s,false);};
 mutate("numEqBands",true);mutate("numEqBands","10");mutate("eq",array(9,0));mutate("effects",array(4,0));mutate("frequencies",array(10,-1));
 mutate("eq",array(10,13));mutate("masterGain",21);mutate("balance",-21);mutate("filterQ",0);mutate("volumeLeveling",5);
 mutate("spectrum",array(10,std::numeric_limits<double>::quiet_NaN()));mutate("effects",array(5,std::numeric_limits<double>::infinity()));
 mutate("frequencyRanges",array(10,20));mutate("dspAvailable","true");
 auto s=state(10);put(s["metrics"],"ready",1);expect(s,false);s=state(10);put(s,"outputUID","");expect(s,false);
 expect(var(),false);expect(var(Array<var>()),false);
 std::cout<<checked<<" schema checks, "<<failed<<" failures\n";return failed?1:0;
}
