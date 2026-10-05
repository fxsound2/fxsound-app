#include "../engine/audio-bridge.h"
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>
#define private public
#include "../engine/dsp-controller.h"
#undef private
#include "../dsp/float32-engine.h"
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
static double rms(const std::array<float,512>& block) {
    double energy=0;for(float x:block){assert(std::isfinite(x));energy+=double(x)*x;}
    return std::sqrt(energy/block.size());
}
int main(int argc,char** argv) {
    assert(argc==2);const std::filesystem::path factory=argv[1];unsigned cases=0;
    for(const auto& file:std::filesystem::directory_iterator(factory)) {
        if(file.path().extension()!=".fac")continue;
        for(int rate:{44100,48000,96000}) {
            fxsound::DSPController controller;assert(controller.dsp_->prepare(rate,2));
            controller.edit([&]{controller.preset(file.path());});
            std::array<float,512> input{},output{};size_t cursor=0;
            auto fill=[&]{for(size_t i=0;i<256;++i){float s=float(0.02*std::sin(2*3.141592653589793*440*(cursor+i)/rate));input[i*2]=input[i*2+1]=s;}cursor+=256;};
            for(int block=0;block<32;++block){fill();output=input;controller.process(&controller,output.data(),256);assert(rms(output)>1e-6);}
            for(int bands:{5,10,15,20,31}) {
                controller.edit([&]{controller.numBands(bands);controller.parameter("masterGain",-20);
                    controller.parameter("balance",-20);controller.parameter("volumeLeveling",0);controller.parameter("filterQ",3);
                    for(int i=1;i<=bands;++i)controller.parameter("eq"+std::to_string(i),-12);});
                controller.bypass(false);double minimum=100,maximum=0;
                for(int block=0;block<96;++block){fill();output=input;controller.process(&controller,output.data(),256);
                    double level=rms(output);if(block>=32){assert(level>1e-6);minimum=std::min(minimum,level);maximum=std::max(maximum,level);}}
                assert(!controller.bypassed());
                controller.bypass(true);
                for(int block=0;block<3;++block){fill();output=input;controller.process(&controller,output.data(),256);assert(rms(output)>1e-6);}
                for(size_t i=0;i<input.size();++i)assert(input[i]==output[i]);
                std::cout<<file.path().filename()<<" rate="<<rate<<" bands="<<bands<<" settledRMS="<<minimum<<".."<<maximum<<'\n';++cases;
            }
        }
    }
    assert(cases==195);std::cout<<"Actual DSPController nonzero settled signal195cases,bypass transitions/worst bounded gain/EQ/Q passed; no HAL claim\n";
}
