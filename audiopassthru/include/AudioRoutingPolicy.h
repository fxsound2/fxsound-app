/* FxSound audio routing policy. Licensed under AGPL-3.0-or-later. */
#pragma once

#include <string>
#include <vector>

namespace AudioRouting
{
struct Endpoint
{
    std::wstring id;
    bool recording = false;
    bool virtualDevice = false;
    bool active = false;
    int channels = 0;
};

struct Preferences
{
    std::wstring input;
    std::wstring requestedOutput;
    std::wstring pinnedOutput;
    std::wstring newOutput;
    std::wstring systemDefault;
    std::wstring recentPlayback;
    std::wstring recentDefault;
    std::wstring priorDefault;
    std::wstring originalDefault;
};

enum class Error
{
    none,
    inputUnavailable,
    outputUnavailable,
    feedback
};

struct Route
{
    int input = -1;
    int output = -1;
    Error error = Error::none;
};

inline bool supportsOutput(int channels)
{
    return channels == 2 || channels == 4 || channels == 6 || channels == 8;
}

inline Route select(const std::vector<Endpoint>& endpoints,
                    const Preferences& preferences)
{
    Route route;
    for (int i = 0; i < static_cast<int>(endpoints.size()); ++i)
    {
        const auto& endpoint = endpoints[i];
        if (endpoint.active &&
            (preferences.input.empty() ? endpoint.virtualDevice
                                       : endpoint.id == preferences.input))
        {
            route.input = i;
            break;
        }
    }
    if (route.input < 0)
    {
        route.error = Error::inputUnavailable;
        return route;
    }

    const auto eligible = [&](int i)
    {
        const auto& endpoint = endpoints[i];
        return endpoint.active && !endpoint.recording &&
               !endpoint.virtualDevice && supportsOutput(endpoint.channels) &&
               i != route.input;
    };
    // A disconnected pin remains saved, while an available fallback keeps audio
    // usable.
    const std::wstring candidates[] = {
        preferences.pinnedOutput,   preferences.requestedOutput,
        preferences.newOutput,      preferences.systemDefault,
        preferences.recentPlayback, preferences.recentDefault,
        preferences.priorDefault,   preferences.originalDefault};
    for (const auto& id : candidates)
    {
        if (id.empty())
        {
            continue;
        }
        for (int i = 0; i < static_cast<int>(endpoints.size()); ++i)
        {
            if (eligible(i) && endpoints[i].id == id)
            {
                route.output = i;
                return route;
            }
        }
    }
    for (int i = 0; i < static_cast<int>(endpoints.size()); ++i)
    {
        if (eligible(i))
        {
            route.output = i;
            return route;
        }
    }
    route.error = (endpoints[route.input].recording ||
                   endpoints[route.input].virtualDevice)
                      ? Error::outputUnavailable
                      : Error::feedback;
    return route;
}
}  // namespace AudioRouting
