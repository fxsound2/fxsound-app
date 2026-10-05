#import <Foundation/Foundation.h>
#include "../engine/command-dispatcher.h"
#include <cassert>
#include <iostream>
int main() {
    fxsound::AudioBridge bridge; fxsound::RoutingGuard routing;
    fxsound::DSPController dsp;
    assert(!dsp.available());
    std::string uid; bool activate = false, shutdown = false;
    auto run = [&](const std::string& text) {
        return fxsound::dispatchCommand(text, bridge, routing, dsp, uid, activate, shutdown);
    };
    for (const auto* text : {"{", "[]", "null", "{}", "{\"version\":true,\"command\":\"getState\"}",
             "{\"version\":2,\"command\":\"getState\"}", "{\"version\":1,\"command\":1}",
             "{\"version\":1,\"command\":\"getState\",\"unknown\":0}",
             "{\"version\":1,\"command\":\"setBypass\",\"value\":0}",
             "{\"version\":1,\"command\":\"setBypass\",\"value\":false}",
             "{\"version\":1,\"command\":\"activateRouting\",\"value\":true}",
             "{\"version\":1,\"command\":\"setOutputUID\",\"outputUID\":\"\"}",
             "{\"version\":1,\"command\":\"setOutputUID\",\"outputUID\":\"a\\u0000b\"}",
             "{\"version\":1,\"command\":\"setOutputUID\",\"outputUID\":\"FxSound_Mac_Virtual\"}",
             "{\"version\":1,\"command\":\"setPreset\"}",
             "{\"version\":1,\"command\":\"setParam\"}",
             "{\"version\":1,\"command\":\"savePreset\",\"path\":\"/missing.fac\"}",
             "{\"version\":1,\"command\":\"setParams\",\"params\":{\"eq1\":2}}"}) {
        const auto reply = run(text);
        assert(reply.find("\"ok\":false") != std::string::npos);
        assert(!activate && uid.empty() && !bridge.ready());
        NSData* bytes = [NSData dataWithBytes:reply.data() length:reply.size()];
        assert([NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil]);
    }
    assert(run("{\"version\":1,\"command\":\"setBypass\",\"value\":true}")
           .find("\"ok\":true") != std::string::npos);
    assert(run("{\"version\":1,\"command\":\"getState\"}")
           .find("\"ready\":false") != std::string::npos);
    assert(run("{\"version\":1,\"command\":\"shutdown\"}").find("\"ok\":true") != std::string::npos);
    assert(shutdown && !activate && !routing.owned());
    std::cout << "command schema: malformed/types/unknown fields/version/readiness/feedback/DSP gate passed; read-only device discovery; no routing change\n";
}
