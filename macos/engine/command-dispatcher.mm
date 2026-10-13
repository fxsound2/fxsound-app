#import <Foundation/Foundation.h>
#include "command-dispatcher.h"
#include "ipc-server.h"

namespace fxsound {
static std::string success(const std::string& data = "null") {
    return "{\"version\":1,\"ok\":true,\"data\":" + data + "}";
}
static bool boolean(id value) {
    return value && CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID();
}
static void requireKeys(NSDictionary* object, NSArray<NSString*>* names) {
    for (id key in object) if (![names containsObject:key]) throw std::runtime_error("unknown request field");
}
std::string dispatchCommand(const std::string& request, AudioBridge& bridge, RoutingGuard& routing,
                            DSPController& dsp, std::string& outputUID, bool& activate, bool& shutdown) {
    @autoreleasepool {
        try {
            NSData* bytes = [NSData dataWithBytes:request.data() length:request.size()];
            NSError* error = nil;
            id parsed = [NSJSONSerialization JSONObjectWithData:bytes options:0 error:&error];
            if (error || ![parsed isKindOfClass:[NSDictionary class]]) throw std::runtime_error("request must be a JSON object");
            NSDictionary* object = parsed;
            id version = object[@"version"], command = object[@"command"];
            if (![version isKindOfClass:[NSNumber class]] || boolean(version) || [version doubleValue] != ipcVersion)
                throw std::runtime_error("incompatible IPC version");
            if (![command isKindOfClass:[NSString class]]) throw std::runtime_error("command must be a string");
            if ([command isEqualToString:@"getDevices"]) {
                requireKeys(object, @[@"version", @"command"]); return success(devicesJSON());
            }
            if ([command isEqualToString:@"getMeters"] || [command isEqualToString:@"getState"]) {
                requireKeys(object, @[@"version", @"command"]);
                return success("{\"controls\":" + dsp.stateJSON() + ",\"outputUID\":" + jsonString(outputUID) +
                               ",\"routingActive\":" + (routing.owned() ? "true" : "false") +
                               ",\"parityVerified\":false,\"metrics\":" + bridge.metricsJSON() + "}");
            }
            if ([command isEqualToString:@"setBypass"]) {
                requireKeys(object, @[@"version", @"command", @"value"]);
                if (!boolean(object[@"value"])) throw std::runtime_error("bypass value must be boolean");
                if (![object[@"value"] boolValue] && !dsp.available()) throw std::runtime_error("DSP unavailable in this build");
                dsp.bypass([object[@"value"] boolValue]);
                return success();
            }
            if ([command isEqualToString:@"setOutputUID"]) {
                requireKeys(object, @[@"version", @"command", @"outputUID"]);
                id uid = object[@"outputUID"];
                if (![uid isKindOfClass:[NSString class]] || ![uid length] || [uid length] > 512 ||
                    [uid rangeOfString:@"\0"].location != NSNotFound) throw std::runtime_error("outputUID must be a bounded string");
                std::string next = [uid UTF8String];
                if (next == virtualUID) throw std::runtime_error("virtual device is not a physical output");
                findDevice(next, false);
                const auto previous = outputUID;
                if (!routing.restore()) throw std::runtime_error("restore physical output before switching devices");
                try { dsp.prepare(bridge, next); bridge.start(next); outputUID = next; }
                catch (...) {
                    try { dsp.prepare(bridge, previous); bridge.start(previous); } catch (...) {}
                    throw;
                }
                return success();
            }
            if ([command isEqualToString:@"activateRouting"]) {
                requireKeys(object, @[@"version", @"command", @"value"]);
                if (!boolean(object[@"value"])) throw std::runtime_error("routing value must be boolean");
                const bool requested = [object[@"value"] boolValue];
                if (requested && !bridge.ready()) throw std::runtime_error("engine is not ready; output was not changed");
                if (requested) routing.activate(bridge.inputDevice());
                else if (!routing.restore()) throw std::runtime_error("physical output restoration failed; retry before stopping engine");
                activate = requested;
                return success();
            }
            if ([command isEqualToString:@"shutdown"]) {
                requireKeys(object, @[@"version", @"command"]);
                if (!routing.restore()) throw std::runtime_error("restore output before shutting down engine");
                activate = false; shutdown = true; return success();
            }
            if ([command isEqualToString:@"setNumEqBands"]) {
                requireKeys(object, @[@"version", @"command", @"count"]);
                id count = object[@"count"];
                if (!dsp.available() || ![count isKindOfClass:[NSNumber class]] || boolean(count))
                    throw std::runtime_error("EQ count must be numeric and DSP available");
                double number = [count doubleValue];
                if (number != 5 && number != 10 && number != 15 && number != 20 && number != 31)
                    throw std::runtime_error("EQ count must be 5,10,15,20 or31");
                dsp.edit([&] { dsp.numBands(static_cast<int>(number)); });
                return success();
            }
            if ([command isEqualToString:@"setPreset"] || [command isEqualToString:@"savePreset"] ||
                [command isEqualToString:@"setParam"] || [command isEqualToString:@"setParams"]) {
                if (!dsp.available()) throw std::runtime_error("DSP unavailable in this build");
                bool save = [command isEqualToString:@"savePreset"], batch = [command isEqualToString:@"setParams"];
                bool preset = [command isEqualToString:@"setPreset"] || save;
                requireKeys(object, preset ? @[@"version", @"command", @"path"] :
                    (batch ? @[@"version", @"command", @"params"] : @[@"version", @"command", @"name", @"value"]));
                id text = object[preset ? @"path" : @"name"];
                if (!batch && (![text isKindOfClass:[NSString class]] || ![text length] || [text length] > 4096 ||
                    [text rangeOfString:@"\0"].location != NSNotFound)) throw std::runtime_error("parameter name/preset path must be a bounded string");
                id value = object[@"value"];
                if (!preset && !batch && (![value isKindOfClass:[NSNumber class]] || boolean(value))) throw std::runtime_error("parameter must be numeric");
                NSDictionary* params = batch ? object[@"params"] : nil;
                if (batch && (![params isKindOfClass:[NSDictionary class]] || [params count] > 71 || ![params count]))
                    throw std::runtime_error("params must contain 1..71 controls");
                for (id key in params) {
                    if (![key isKindOfClass:[NSString class]] || [key length] > 32 ||
                        [key rangeOfString:@"\0"].location != NSNotFound ||
                        ![params[key] isKindOfClass:[NSNumber class]] || boolean(params[key]))
                        throw std::runtime_error("parameter batch must contain numeric values");
                    dsp.validateCurrentParameter([key UTF8String], [params[key] doubleValue]);
                }
                if (!preset && !batch) dsp.validateCurrentParameter([text UTF8String], [value doubleValue]);
                dsp.edit([&] {
                    if (save) dsp.savePreset([text UTF8String]);
                    else if (preset) dsp.preset([text UTF8String]);
                    else if (batch) for (NSString* key in params) dsp.parameter([key UTF8String], [params[key] doubleValue]);
                    else dsp.parameter([text UTF8String], [value doubleValue]);
                });
                return success();
            }
            throw std::runtime_error("unknown command");
        } catch (const std::exception& error) {
            return "{\"version\":1,\"ok\":false,\"error\":" + jsonString(error.what()) + "}";
        }
    }
}
}
