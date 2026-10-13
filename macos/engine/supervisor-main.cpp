#include "audio-devices.h"
#include "recovery-channel.h"
#include <chrono>
#include <csignal>
#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

extern char** environ;
static volatile sig_atomic_t stopping = 0;
static void stopSignal(int) { stopping = 1; }
static void stopChild(pid_t child) noexcept {
    int status = 0;
    auto result = waitpid(child, &status, WNOHANG);
    if (result == child || (result < 0 && errno == ECHILD)) return;
    kill(child, SIGTERM);
    for (int retry = 0; retry < 40; ++retry) {
        result = waitpid(child, &status, WNOHANG);
        if (result == child || (result < 0 && errno == ECHILD)) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    kill(child, SIGKILL);
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
}
static bool restoreOutput(const std::string& previous) {
    try {
        if (fxsound::deviceUID(fxsound::defaultOutput()) != fxsound::virtualUID) return true;
        AudioDeviceID fallback = 0;
        for (const auto& device : fxsound::devices()) if (device.outputs && device.uid != fxsound::virtualUID) {
            if (!fallback || device.uid == previous) fallback = device.id;
            if (device.uid == previous) break;
        }
        if (!fallback) return false;
        fxsound::setDefaultOutput(fallback); return true;
    } catch (...) { return false; }
}
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--restore-output") return restoreOutput(argv[2]) ? 0 : 1;
        if (argc < 5 || std::string(argv[1]) != "--engine" || std::string(argv[3]) != "--output") {
            std::cerr << "Usage: fxsound-supervisor --engine ABSOLUTE_PATH --output UID [--activate] | --restore-output PREVIOUS_UID\n";
            return 2;
        }
        const std::string engine = argv[2], output = argv[4];
        if (engine.empty() || engine[0] != '/' || output == fxsound::virtualUID)
            throw std::runtime_error("engine path must be absolute and output must be physical");
        bool activate = false; std::string preset, fallbackPreset;
        for (int i = 5; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--activate") activate = true;
            else if (option == "--preset" && i + 1 < argc) preset = argv[++i];
            else if (option == "--fallback-preset" && i + 1 < argc) fallbackPreset = argv[++i];
            else throw std::runtime_error("unknown supervisor option");
        }
        auto previous = fxsound::deviceUID(fxsound::defaultOutput());
        std::signal(SIGTERM, stopSignal); std::signal(SIGINT, stopSignal);
        for (int attempt = 0; attempt < 3 && !stopping; ++attempt) {
            fxsound::RecoveryChannel recovery;
            std::vector<std::string> args{engine, "--run", output};
            args.emplace_back("--recovery-fd"); args.push_back(std::to_string(recovery.childFD()));
            if (activate) args.emplace_back("--activate");
            if (!preset.empty()) { args.emplace_back("--preset"); args.push_back(preset); }
            if (!fallbackPreset.empty()) { args.emplace_back("--fallback-preset"); args.push_back(fallbackPreset); }
            std::vector<char*> raw;
            for (auto& arg : args) raw.push_back(arg.data());
            raw.push_back(nullptr);
            pid_t child = 0;
            int status = posix_spawn(&child, engine.c_str(), nullptr, nullptr, raw.data(), environ);
            if (status) throw std::runtime_error("could not launch engine: " + std::to_string(status));
            try {
            recovery.closeWriter();
            auto stopAt = std::chrono::steady_clock::time_point{};
            bool terminated = false;
            for (;;) {
                recovery.latest(previous);
                auto result = waitpid(child, &status, WNOHANG);
                if (result == child) break;
                if (result < 0 && errno != EINTR) throw std::runtime_error("could not observe engine exit");
                if (stopping && !terminated) {
                    restoreOutput(previous);
                    kill(child, SIGTERM); terminated = true; stopAt = std::chrono::steady_clock::now();
                }
                if (terminated && std::chrono::steady_clock::now() - stopAt > std::chrono::seconds(2)) kill(child, SIGKILL);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            recovery.latest(previous);
            bool restored = false;
            for (int retry = 0; retry < 20 && !restored; ++retry) {
                restored = restoreOutput(previous);
                if (!restored) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if (!restored) std::cerr << "fxsound-supervisor: physical output restoration unavailable\n";
            if (stopping || (WIFEXITED(status) && WEXITSTATUS(status) == 0)) return restored ? 0 : 1;
            activate = false;
            std::cerr << "fxsound-supervisor: engine exited; retry " << attempt + 1 << "/3 without automatic routing\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(250 * (1 << attempt)));
            } catch (...) {
                restoreOutput(previous);
                stopChild(child);
                restoreOutput(previous);
                throw;
            }
        }
        return stopping ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "fxsound-supervisor: " << error.what() << '\n'; return 1;
    }
}
