/*
FxSound — Linux build: PipeWire passthrough smoke test (M2 verification)

Brings up the AudioPassthru backend, holds it for a few seconds (so the FxSound
node is visible in the graph, e.g. `pw-cli ls Node`), then tears it down.
*/

#include "AudioPassthru.h"
#include <cstdio>
#include <thread>
#include <chrono>

int main(int argc, char** argv)
{
    int seconds = (argc > 1) ? atoi(argv[1]) : 3;
    AudioPassthru ap;
    int rc = ap.init();
    std::printf("AudioPassthru::init() rc=%d, playbackAvailable=%d\n",
                rc, (int)ap.isPlaybackDeviceAvailable());
    if (rc != 0) return rc;
    std::printf("FxSound node should now be present for %d s...\n", seconds);
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    std::printf("done\n");
    return 0;
}
