#import <Foundation/Foundation.h>
#include "../engine/command-dispatcher.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <filesystem>
static NSDictionary* parse(const std::string& text) {
    NSData* bytes = [NSData dataWithBytes:text.data() length:text.size()];
    return [NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil];
}
int main() {
    fxsound::AudioBridge bridge; fxsound::RoutingGuard routing; fxsound::DSPController dsp;
    assert(dsp.available()); std::string uid; bool active = false, shutdown = false;
    auto run = [&](const std::string& text) {
        return fxsound::dispatchCommand(text, bridge, routing, dsp, uid, active, shutdown);
    };
    dsp.edit([&] { dsp.numBands(10); });
    const auto before = dsp.stateJSON();
    for (const auto* command : {
         "{\"version\":1,\"command\":\"setParams\",\"params\":{\"fidelity\":2,\"eq1\":99}}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":{\"fidelity\":2,\"unknown\":0}}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":{\"eq1\\u0000bad\":2}}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":{\"eq1\":true}}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":{}}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":[]}",
         "{\"version\":1,\"command\":\"setParam\",\"name\":\"eq0\",\"value\":2}",
         "{\"version\":1,\"command\":\"setParam\",\"name\":\"eq1\",\"value\":13}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":{\"fidelity\":2,\"eq11\":1}}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":{\"fidelity\":2,\"freq1\":20000}}",
         "{\"version\":1,\"command\":\"setParams\",\"params\":{\"fidelity\":2,\"masterGain\":21}}",
         "{\"version\":1,\"command\":\"setNumEqBands\",\"count\":9}",
         "{\"version\":1,\"command\":\"setNumEqBands\",\"count\":true}",
         "{\"version\":1,\"command\":\"setNumEqBands\",\"count\":10.5}"}) {
        assert(![parse(run(command))[@"ok"] boolValue]);
        assert(before == dsp.stateJSON());
        assert(!bridge.ready() && !active && !shutdown);
    }
    dsp.edit([&] { dsp.parameter("eq1", 2.5); dsp.parameter("eq9", -3.25); });
    NSDictionary* state = parse(dsp.stateJSON()); NSArray* eq = state[@"eq"];
    assert([eq count] == 10 && [eq[0] doubleValue] == 2.5 && [eq[8] doubleValue] == -3.25);
    dsp.edit([&] { dsp.parameter("masterGain", 2); dsp.parameter("balance", -4);
        dsp.parameter("volumeLeveling", 2); dsp.parameter("filterQ", 1.5); });
    state = parse(dsp.stateJSON());
    assert([state[@"masterGain"] doubleValue] == 2 && [state[@"balance"] doubleValue] == -4);
    assert([state[@"volumeLeveling"] doubleValue] == 2 && [state[@"filterQ"] doubleValue] == 1.5);
    NSArray* limits = state[@"frequencyRanges"][0];
    double frequency = ([limits[0] doubleValue] + [limits[1] doubleValue]) / 2;
    dsp.edit([&] { dsp.parameter("freq1", frequency); });
    assert(std::abs([parse(dsp.stateJSON())[@"frequencies"][0] doubleValue] - frequency) < 0.01);
    auto success = [&](NSDictionary* command) {
        NSData* encoded = [NSJSONSerialization dataWithJSONObject:command options:0 error:nil];
        assert([parse(run(std::string((const char*)[encoded bytes], [encoded length])))[@"ok"] boolValue]);
        assert(!bridge.ready() && !active);
    };
    success(@{@"version":@1,@"command":@"setNumEqBands",@"count":@31});
    NSMutableDictionary* values = [NSMutableDictionary dictionary];
    state = parse(dsp.stateJSON());
    for (int band = 1; band <= 31; ++band) {
        values[[NSString stringWithFormat:@"eq%d",band]] = @(band % 7 - 3);
        NSArray* range = state[@"frequencyRanges"][band-1];
        values[[NSString stringWithFormat:@"freq%d",band]] = @(([range[0] doubleValue] + [range[1] doubleValue]) / 2);
    }
    for (NSString* name in @[@"fidelity",@"ambience",@"surround",@"dynamicBoost",@"bass"]) values[name] = @3;
    values[@"masterGain"] = @2; values[@"balance"] = @-4;
    values[@"volumeLeveling"] = @2; values[@"filterQ"] = @1.5;
    assert(values.count == 71);
    success(@{@"version":@1,@"command":@"setParams",@"params":values});
    assert([parse(dsp.stateJSON())[@"eq"][30] doubleValue] == 0);
    const auto source = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    NSString* preset = [NSString stringWithUTF8String:(source / "Installer/Resources/Factsoft/Default.fac").c_str()];
    success(@{@"version":@1,@"command":@"setPreset",@"path":preset});
    NSString* saved = @"/private/tmp/fxsound-engine-unit-tests/schema-saved.fac";
    success(@{@"version":@1,@"command":@"savePreset",@"path":saved});
    success(@{@"version":@1,@"command":@"setPreset",@"path":saved});
    std::filesystem::remove([saved UTF8String]);
    assert([parse(run("{\"version\":1,\"command\":\"shutdown\"}"))[@"ok"] boolValue] && shutdown);
    std::cout << "real DSP schema: full-batch prevalidation/no partial mutation,NUL rejection,EQ1/EQ9 indexing,71-control batch/count/preset/save without HAL restart,shutdown passed\n";
}
