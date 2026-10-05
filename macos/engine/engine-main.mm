#include "audio-bridge.h"
#include "command-dispatcher.h"
#include "ipc-server.h"
#include "routing-guard.h"
#include "recovery-channel.h"
#include <csignal>
#include <iostream>
#include <chrono>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

static volatile sig_atomic_t stopping = 0;
static void stopSignal(int) { stopping = 1; }
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--devices") { std::cout << fxsound::devicesJSON() << '\n'; return 0; }
        if (argc == 4 && std::string(argv[1]) == "--request") {
            std::cout << fxsound::IPCServer::request(argv[2], argv[3]) << '\n'; return 0;
        }
        if (argc == 1 || std::string(argv[1]) != "--run" || argc < 3) {
            std::cerr << "Usage: fxsound-engine --devices | --request SOCKET JSON | --run OUTPUT_UID [--activate] [--socket PATH]\n";
            return 2;
        }
        std::string outputUID = argv[2], socketPath = fxsound::defaultSocketPath(); bool activate = false;
        std::string presetPath, fallbackPreset;
        int recoveryFD = -1;
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--activate") activate = true;
            else if (arg == "--socket" && i + 1 < argc) socketPath = argv[++i];
            else if (arg == "--preset" && i + 1 < argc) presetPath = argv[++i];
            else if (arg == "--fallback-preset" && i + 1 < argc) fallbackPreset = argv[++i];
            else if (arg == "--recovery-fd" && i + 1 < argc) recoveryFD = std::stoi(argv[++i]);
            else throw std::runtime_error("unknown or incomplete engine option");
        }
        if (outputUID == fxsound::virtualUID) throw std::runtime_error("virtual output would cause feedback");
        std::signal(SIGINT, stopSignal); std::signal(SIGTERM, stopSignal);
        std::signal(SIGPIPE, SIG_IGN);
        fxsound::RoutingGuard routing;
        if (recoveryFD >= 0) {
            struct stat channelInfo{};
            int socketType = 0; socklen_t typeSize = sizeof(socketType);
            if (fstat(recoveryFD, &channelInfo) || !S_ISSOCK(channelInfo.st_mode) ||
                getsockopt(recoveryFD, SOL_SOCKET, SO_TYPE, &socketType, &typeSize) || socketType != SOCK_DGRAM ||
                fcntl(recoveryFD, F_SETFD, FD_CLOEXEC))
                throw std::runtime_error("invalid supervisor recovery channel");
            routing.recoverySink([recoveryFD](const std::string& uid) { fxsound::notifyRecovery(recoveryFD, uid); });
        }
        fxsound::DSPController dsp;
        fxsound::AudioBridge bridge;
        dsp.prepare(bridge, outputUID);
        if (!presetPath.empty()) {
            try { dsp.edit([&] { dsp.preset(presetPath); }); }
            catch (...) {
                if (fallbackPreset.empty()) throw;
                std::cerr << "fxsound-engine: saved preset unavailable; loading fallback preset\n";
                dsp.edit([&] { dsp.preset(fallbackPreset); });
            }
        }
        bridge.start(outputUID);
        fxsound::IPCServer server(socketPath);
        auto started = std::chrono::steady_clock::now();
        bool shutdown = false;
        while (!stopping && !shutdown) {
            server.poll([&](const std::string& request) {
                return fxsound::dispatchCommand(request, bridge, routing, dsp, outputUID, activate, shutdown);
            });
            if (routing.owned() && fxsound::deviceUID(fxsound::defaultOutput()) != fxsound::virtualUID) {
                activate = false; routing.restore();
            }
            if (bridge.changed()) {
                routing.restore(); dsp.prepare(bridge, outputUID); bridge.start(outputUID); started = std::chrono::steady_clock::now();
            }
            if (activate && !routing.owned() && bridge.ready()) routing.activate(bridge.inputDevice());
            if (!bridge.ready() && std::chrono::steady_clock::now() - started > std::chrono::seconds(5))
                throw std::runtime_error("capture/render readiness timed out; default output was not activated");
        }
        routing.restore(); bridge.stop();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fxsound-engine: " << error.what() << '\n'; return 1;
    }
}
