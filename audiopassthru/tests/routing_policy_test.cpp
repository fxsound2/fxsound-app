// Compile with clang++/g++ -std=c++14, or cl /EHsc /std:c++14.
#include "../include/AudioRoutingPolicy.h"
#include <cstdlib>
#include <iostream>

using namespace AudioRouting;

static int passed = 0;
static void expect(const char* name, const std::vector<Endpoint>& devices, const Preferences& preferences,
    int input, int output, Error error = Error::none)
{
    const auto route = select(devices, preferences);
    if (route.input != input || route.output != output || route.error != error)
    {
        std::cerr << "FAIL: " << name << " (input=" << route.input << ", output=" << route.output << ")\n";
        std::exit(1);
    }
    if (route.error == Error::none && (route.input == route.output || devices[route.output].recording))
        std::exit(2);
    ++passed;
}

int main()
{
    const std::vector<Endpoint> devices = {
        { L"fx", false, true, true, 2 },
        { L"speakers", false, false, true, 2 },
        { L"headphones", false, false, true, 2 },
        { L"microphone", true, false, true, 1 },
        { L"cable-recording", true, false, true, 2 },
        { L"cable-playback", false, false, true, 2 }
    };
    Preferences p;
    p.systemDefault = L"speakers";
    expect("first run uses virtual input and physical default", devices, p, 0, 1);
    p.requestedOutput = L"headphones";
    expect("explicit output beats Windows default", devices, p, 0, 2);
    p.pinnedOutput = L"speakers";
    expect("active pin beats automatic output requests", devices, p, 0, 1);
    p.newOutput = L"cable-playback";
    expect("newly connected device cannot steal a pinned output", devices, p, 0, 1);
    p.pinnedOutput = L"missing-output";
    expect("disconnected pin uses requested fallback", devices, p, 0, 2);
    p.requestedOutput.clear();
    expect("new output provides fallback when pin missing", devices, p, 0, 5);
    p.newOutput.clear();
    expect("missing pin preserves a physical default fallback", devices, p, 0, 1);
    p.pinnedOutput = L"headphones";
    auto disconnected = devices;
    disconnected[2].active = false;
    expect("unplugged pin falls back", disconnected, p, 0, 1);
    expect("reconnected pin takes priority again", devices, p, 0, 2);
    p.input = L"microphone";
    expect("mono recording input is selectable", devices, p, 3, 2);
    p.input = L"cable-recording";
    expect("virtual cable recording endpoint is selectable", devices, p, 4, 2);
    p.input = L"cable-playback";
    expect("virtual cable playback endpoint is a loopback source", devices, p, 5, 2);
    p.input = L"speakers";
    p.systemDefault = L"speakers";
    p.pinnedOutput = L"speakers";
    expect("pin and default cannot cause a loopback feedback route", devices, p, 1, 2);
    p.input = L"missing-microphone";
    expect("missing input never falls back to another microphone", devices, p, -1, -1, Error::inputUnavailable);
    p.input = L"microphone";
    disconnected = devices;
    disconnected[3].active = false;
    expect("disconnected recording input pauses routing", disconnected, p, -1, -1, Error::inputUnavailable);
    p.input.clear();
    disconnected = devices;
    disconnected[0].active = false;
    expect("missing FxSound input is not silently replaced", disconnected, p, -1, -1, Error::inputUnavailable);
    p = {};
    p.systemDefault = L"microphone";
    p.pinnedOutput = L"cable-recording";
    expect("recording devices cannot become playback outputs", devices, p, 0, 1);
    p.systemDefault = L"fx";
    p.pinnedOutput = L"fx";
    expect("FxSound virtual device cannot become playback output", devices, p, 0, 1);
    p = {};
    p.recentPlayback = L"headphones";
    expect("recent physical playback is remembered", devices, p, 0, 2);
    p.recentPlayback = L"missing";
    p.originalDefault = L"headphones";
    expect("original default remains a usable fallback", devices, p, 0, 2);
    std::vector<Endpoint> onlySource = { devices[1] };
    p.input = L"speakers";
    expect("same render device as sole input and output is rejected", onlySource, p, 0, -1, Error::feedback);
    std::vector<Endpoint> noOutput = { devices[0], devices[3] };
    p.input = L"microphone";
    expect("microphone without physical output cannot start", noOutput, p, 1, -1, Error::outputUnavailable);
    p.input.clear();
    expect("virtual endpoint alone reports unavailable output", { devices[0] }, p, 0, -1, Error::outputUnavailable);
    auto unsupported = devices;
    unsupported[1].channels = 1;
    unsupported[2].channels = 3;
    p.input.clear();
    p.systemDefault = L"speakers";
    expect("unsupported output layouts are excluded", unsupported, p, 0, 5);
    for (const int channels : { 2, 4, 6, 8 })
    {
        auto layout = devices;
        layout[1].channels = channels;
        p = {};
        p.systemDefault = L"speakers";
        expect("supported output layouts remain selectable", layout, p, 0, 1);
    }
    std::cout << passed << " audio routing scenarios passed\n";
}
